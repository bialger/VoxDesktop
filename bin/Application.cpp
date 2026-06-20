#include "Application.hpp"

#include "app/ServiceLocator.hpp"
#include "crypto/SodiumInit.hpp"
#include "network/RealtimeSocket.hpp"
#include "ui/shell/MainWindow.hpp"

#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QIcon>
#include <QMessageBox>
#include <QMetaObject>
#include <QSqlDatabase>
#include <QSslSocket>
#include <QStandardPaths>
#include <QThread>
#include <QTimer>
#include <QUrl>
#include <memory>
#ifdef VOX_HAS_QT_WEBSOCKETS
#include <QWebSocket>
#endif

#include <algorithm>
#include <functional>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>

namespace vox::app {
namespace {

constexpr int kPollIntervalMs = 2000;
constexpr qint64 kMillisPerSecond = 1000;
constexpr int kConversationIdSuffixLength = 6;
constexpr int kConversationMessagesPageSize = 200;
constexpr int kPendingEnvelopesPageSize = 100;
constexpr int kUiRefreshDelayMs = 1500;
constexpr int kUiRefreshRetryDelayMs = 3500;

QByteArray B64(const QByteArray &bytes) {
  return bytes.toBase64(QByteArray::Base64Encoding);
}

std::optional<QString> DecryptForUi(const QString &ciphertext) {
  if (ciphertext.trimmed().isEmpty()) {
    return QString{};
  }

  const auto as_utf8 = ciphertext.toUtf8();
  if (ciphertext.startsWith("vox1:")) {
    const QByteArray decoded = QByteArray::fromBase64(as_utf8.mid(5));
    if (!decoded.isEmpty()) {
      return QString::fromUtf8(decoded);
    }
  }

  // plaintext-looking fallback
  bool looks_plain = true;
  for (const auto ch : as_utf8) {
    if (ch == 0) {
      looks_plain = false;
      break;
    }
  }
  if (looks_plain) {
    return ciphertext;
  }

  const QByteArray decoded = QByteArray::fromBase64(as_utf8);
  if (!decoded.isEmpty()) {
    return QString::fromUtf8(decoded);
  }
  return std::nullopt;
}

QString EncryptForTransport(const QString &plaintext) {
  return QStringLiteral("vox1:") + QString::fromUtf8(B64(plaintext.toUtf8()));
}

QUrl WsUrlForBaseUrl(QString baseUrl) {
  baseUrl = baseUrl.trimmed();
  while (baseUrl.endsWith('/'))
    baseUrl.chop(1);
  QUrl url(baseUrl);
  if (url.scheme() == "https") {
    url.setScheme("wss");
  } else {
    url.setScheme("ws");
  }
  url.setPath(url.path() + "/v1/ws");
  return url;
}

bool StartupDiagnostics(QString *error) {
  if (!crypto::ensureSodiumInitialized()) {
    *error = "sodium_init failed";
    return false;
  }

  if (!QSqlDatabase::isDriverAvailable("QSQLITE")) {
    *error = "QSQLITE driver unavailable";
    return false;
  }

  if (!QSslSocket::supportsSsl()) {
    *error = "TLS support unavailable";
    return false;
  }

  const QString app_data = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  if (app_data.isEmpty()) {
    *error = "No writable AppData path";
    return false;
  }

  QDir app_data_dir(app_data);
  if (!app_data_dir.exists() && !app_data_dir.mkpath(".")) {
    *error = "Failed to create AppData directory";
    return false;
  }

  QFile probe(app_data + "/.vox_write_probe");
  if (!probe.open(QIODevice::WriteOnly)) {
    *error = "AppData path is not writable";
    return false;
  }
  probe.write("ok");
  probe.close();
  probe.remove();

#ifdef VOX_HAS_QT_WEBSOCKETS
  QWebSocket socket;
  if (socket.metaObject()->className() == nullptr) {
    *error = "WebSocket module unavailable";
    return false;
  }
#endif

  return true;
}

template<typename Fn>
auto RunOnServiceThread(QObject *context, Fn &&task) -> std::invoke_result_t<Fn> {
  using Result = std::invoke_result_t<Fn>;

  if (QThread::currentThread() == context->thread()) {
    if constexpr (std::is_void_v<Result>) {
      std::forward<Fn>(task)();
      return;
    } else {
      return std::forward<Fn>(task)();
    }
  }

  if constexpr (std::is_void_v<Result>) {
    QMetaObject::invokeMethod(
        context, [task = std::forward<Fn>(task)]() mutable { task(); }, Qt::BlockingQueuedConnection);
    return;
  } else {
    std::optional<Result> result;
    QMetaObject::invokeMethod(
        context, [&result, task = std::forward<Fn>(task)]() mutable { result = task(); }, Qt::BlockingQueuedConnection);
    return std::move(*result);
  }
}

} // namespace

int Application::run(int argc, char **argv) {
  QApplication app(argc, argv);
  QApplication::setWindowIcon(QIcon(":/vox_logo.png"));

  QString diagnostics_error;
  if (!StartupDiagnostics(&diagnostics_error)) {
    QMessageBox::critical(nullptr, "Vox startup failure", diagnostics_error);
    return 1;
  }

  const QString app_data_path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  const QString vault_password = qEnvironmentVariableIsSet("VOX_VAULT_PASSWORD")
                                     ? QString::fromUtf8(qgetenv("VOX_VAULT_PASSWORD"))
                                     : QStringLiteral("vox-dev-password");

  ui::shell::MainWindow window;
  window.setWindowIcon(QIcon(":/vox_logo.png"));

  QThread service_thread;
  service_thread.setObjectName("vox-service");
  service_thread.start();
  auto service_context = std::make_unique<QObject>();
  service_context->moveToThread(&service_thread);
  QObject *const service_context_ptr = service_context.get();

  auto services = std::make_shared<app::ServiceLocator>();
  auto service_call = [service_context_ptr](auto &&task) -> decltype(auto) {
    return RunOnServiceThread(service_context_ptr, std::forward<decltype(task)>(task));
  };
  struct UiState {
    QString selectedConversationId;
    std::function<void()> refreshConversations;
    QString serverBaseUrl;
    bool selectedConversationIsSelfDm{false};
  };
  auto state = std::make_shared<UiState>();
  bool services_initialized = false;

  // Realtime: prefer WebSocket push, fallback to polling /v1/sync/pending.
  network::RealtimeSocket realtime(&window);
  QTimer poll_timer(&window);
  poll_timer.setInterval(kPollIntervalMs);
  QString pending_cursor;

  auto apply_envelope = [&, services, state](const network::EnvelopeDto &env) {
    if (!services_initialized)
      return;
    const auto auth = service_call([&services]() { return services->authService()->context(); });
    const QString my_device_id = auth.has_value() ? auth->deviceId : QString{};
    const QString my_user_id = auth.has_value() ? auth->userId : QString{};
    const QString my_username = auth.has_value() ? auth->username : QString{};

    domain::Message msg;
    msg.messageId = env.envelopeId;
    msg.conversationId = env.conversationId;
    const QString sender_user_id = env.senderUserId;
    msg.senderDeviceId = env.senderDeviceId;
    msg.serverReceivedAtMs = env.serverTimestamp * kMillisPerSecond;
    msg.clientCreatedAtMs = msg.serverReceivedAtMs;
    msg.ciphertextBlob = env.ciphertext.toUtf8();

    const auto plaintext = DecryptForUi(env.ciphertext);
    if (plaintext.has_value()) {
      msg.plaintextCacheCiphertext = plaintext->toUtf8();
    } else {
      msg.plaintextCacheCiphertext =
          QString("[Encrypted envelope %1]").arg(env.envelopeId.right(kConversationIdSuffixLength)).toUtf8();
    }
    bool is_self_dm = false;
    if (env.conversationId == state->selectedConversationId) {
      is_self_dm = state->selectedConversationIsSelfDm;
    } else {
      // Only needed for correct "self DM" device-based outgoing.
      is_self_dm = service_call([&services, &env, &my_user_id]() {
        auto *conversations_api = services->conversationsApi();
        if (conversations_api == nullptr) {
          return false;
        }
        const auto conv = conversations_api->conversation(env.conversationId);
        return conv.ok && conv.data.has_value() && conv.data->type == 0 && conv.data->peerUserId.has_value() &&
               my_user_id == *conv.data->peerUserId;
      });
    }

    // Outgoing rule:
    // - normal conversations: any message from my user -> right bubble
    // - DM with self: only messages from *this device* -> right bubble
    msg.isOutgoing = is_self_dm ? (!my_device_id.isEmpty() && env.senderDeviceId == my_device_id)
                                : (!my_user_id.isEmpty() && sender_user_id == my_user_id);

    // Author label: store username for UI (not UUID).
    if (msg.isOutgoing) {
      msg.senderUserId = my_username.isEmpty() ? QStringLiteral("me") : my_username;
    } else {
      msg.senderUserId = service_call([&services, sender_user_id]() {
        auto *directory = services->directoryApi();
        QString display = sender_user_id;
        if (directory != nullptr && !sender_user_id.isEmpty()) {
          const auto resolved = directory->userById(sender_user_id);
          if (resolved.ok && resolved.data.has_value() && !resolved.data->username.isEmpty()) {
            display = resolved.data->username;
          }
        }
        return display;
      });
    }

    service_call([&services, msg]() {
      if (auto *messages_repo = services->messagesRepository(); messages_repo != nullptr) {
        messages_repo->upsertMessage(msg);
      }
    });

    // Update currently open conversation view.
    if (env.conversationId == state->selectedConversationId) {
      const auto messages = service_call([&services, conversation_id = env.conversationId]() {
        if (auto *messages_repo = services->messagesRepository(); messages_repo != nullptr) {
          return messages_repo->listConversationMessages(conversation_id, kConversationMessagesPageSize);
        }
        return QVector<domain::Message>{};
      });
      window.setMessages(messages);
    }

    // Bump chat ordering in UI quickly.
    auto list = service_call([&services]() { return services->conversationService()->conversations(); });
    const auto now = QDateTime::currentDateTime();
    for (auto &c : list) {
      if (c.conversationId == env.conversationId) {
        c.lastMessageAt = now;
      }
    }
    std::ranges::sort(list, [](const domain::Conversation &a, const domain::Conversation &b) {
      if (a.pinRank != b.pinRank)
        return a.pinRank > b.pinRank;
      return a.lastMessageAt > b.lastMessageAt;
    });
    window.setConversations(list);
  };

  QObject::connect(&realtime, &network::RealtimeSocket::envelopeReceived, &window, apply_envelope);
  QObject::connect(
      &realtime, &network::RealtimeSocket::membershipChanged, &window, [&, services](const QString &, int) {
        if (!services_initialized) {
          return;
        }
        const auto refreshed = service_call([&services]() {
          const bool ok = services->conversationService()->refreshConversations();
          const auto list = ok ? services->conversationService()->conversations() : QVector<domain::Conversation>{};
          return std::pair<bool, QVector<domain::Conversation>>{ok, list};
        });
        if (refreshed.first) {
          window.setConversations(refreshed.second);
        }
      });
  QObject::connect(&realtime, &network::RealtimeSocket::socketError, &window, [&](const QString &) {
    if (services_initialized) {
      poll_timer.start();
    }
  });

  QObject::connect(&poll_timer, &QTimer::timeout, &window, [&, services, state]() {
    if (!services_initialized)
      return;
    const auto batch = service_call([&services, cursor = pending_cursor]() {
      auto *api = services->conversationsApi();
      if (api == nullptr) {
        return network::ApiResult<network::EnvelopeBatchResponse>{};
      }
      return api->pendingEnvelopes(kPendingEnvelopesPageSize, cursor);
    });
    if (!batch.ok || !batch.data.has_value()) {
      return;
    }
    pending_cursor = batch.data->nextCursor;
    for (const auto &env : batch.data->envelopes) {
      apply_envelope(env);
    }
  });

  QObject::connect(&window, &ui::shell::MainWindow::serverBaseUrlReady, [&, services, state](const QString &baseUrl) {
    if (services_initialized) {
      return;
    }

    const auto init = service_call([&services, &app_data_path, &baseUrl, &vault_password]() {
      QString init_error;
      const bool ok = services->initialize(app_data_path, baseUrl, vault_password, &init_error);
      return std::pair<bool, QString>{ok, init_error};
    });
    if (!init.first) {
      QMessageBox::critical(&window, "Vox startup failure", init.second);
      return;
    }
    services_initialized = true;
    state->serverBaseUrl = baseUrl;

    state->refreshConversations = [&, services, state]() {
      const auto refreshed = service_call([&services]() {
        const bool ok = services->conversationService()->refreshConversations();
        const auto list = ok ? services->conversationService()->conversations() : QVector<domain::Conversation>{};
        return std::pair<bool, QVector<domain::Conversation>>{ok, list};
      });
      if (refreshed.first) {
        window.setConversations(refreshed.second);
        if (state->selectedConversationId.isEmpty() && !refreshed.second.isEmpty()) {
          state->selectedConversationId = refreshed.second.first().conversationId;
        }
      }
    };

    QObject::connect(
        &window, &ui::shell::MainWindow::conversationSelected, [&, services, state](const QString &conversationId) {
          state->selectedConversationId = conversationId;
          const auto messages = service_call([&services, &conversationId]() {
            auto *messages_repo = services->messagesRepository();
            if (messages_repo != nullptr) {
              return messages_repo->listConversationMessages(conversationId, kPendingEnvelopesPageSize);
            }
            return QVector<domain::Message>{};
          });
          window.setMessages(messages);
        });

    QObject::connect(&window,
                     &ui::shell::MainWindow::loginSubmitted,
                     [&, services, state](const QString &username, const QString &passwordDerived) {
                       const bool logged_in = service_call([&services, &username, &passwordDerived]() {
                         return services->authService()->login(username, passwordDerived);
                       });
                       if (!logged_in) {
                         QMessageBox::warning(&window, "Login failed", "Invalid credentials or server error");
                         return;
                       }

                       window.showMain();
                       if (state->refreshConversations) {
                         state->refreshConversations();
                       }

                       // Start realtime after auth.
                       const auto ctx = service_call([&services]() { return services->authService()->context(); });
                       if (ctx.has_value()) {
                         realtime.connectAndAuthenticate(WsUrlForBaseUrl(state->serverBaseUrl), ctx->accessToken);
                       }
                     });

    QObject::connect(&window,
                     &ui::shell::MainWindow::registerSubmitted,
                     [&, services, state](const QString &username, const QString &passwordDerived) {
                       const bool registered = service_call([&services, &username, &passwordDerived]() {
                         return services->authService()->registerUser(username, passwordDerived);
                       });
                       if (!registered) {
                         QMessageBox::warning(
                             &window, "Registration failed", "Could not register account with current server");
                         return;
                       }

                       window.showMain();
                       if (state->refreshConversations) {
                         state->refreshConversations();
                       }

                       const auto ctx = service_call([&services]() { return services->authService()->context(); });
                       if (ctx.has_value()) {
                         realtime.connectAndAuthenticate(WsUrlForBaseUrl(state->serverBaseUrl), ctx->accessToken);
                       }
                     });

    QObject::connect(&window, &ui::shell::MainWindow::sendMessageSubmitted, [&, services, state](const QString &text) {
      const auto auth = service_call([&services]() { return services->authService()->context(); });
      if (!auth.has_value()) {
        QMessageBox::information(&window, "Not authenticated", "Please login before sending messages.");
        return;
      }

      if (state->selectedConversationId.isEmpty()) {
        QMessageBox::information(&window, "No conversation selected", "Select or create a conversation first.");
        return;
      }

      const auto conv_dto = service_call([&services, conversation_id = state->selectedConversationId]() {
        auto *conversations_api = services->conversationsApi();
        if (conversations_api == nullptr) {
          return network::ApiResult<network::ConversationSummaryDto>{};
        }
        return conversations_api->conversation(conversation_id);
      });
      const bool is_channel = conv_dto.ok && conv_dto.data.has_value() && conv_dto.data->type == 2;
      const QString my_role = (conv_dto.ok && conv_dto.data.has_value()) ? conv_dto.data->myRole.toLower() : QString{};
      const bool can_send_in_channel = (my_role == "owner" || my_role == "admin");
      if (is_channel && !can_send_in_channel) {
        QMessageBox::information(&window, "Read-only", "You don't have permission to post to this channel.");
        return;
      }

      const QString ciphertext = EncryptForTransport(text);
      const bool ok = service_call(
          [&services, conversation_id = state->selectedConversationId, device_id = auth->deviceId, text, ciphertext]() {
            auto *sender = services->messageSendService();
            return (sender != nullptr) && sender->sendMessage(conversation_id, device_id, text, ciphertext);
          });
      if (!ok) {
        QMessageBox::warning(&window, "Send failed", "Message queued for retry.");
      }

      // Immediate UI update (messages + chat ordering) even if server list is stale.
      const auto messages = service_call([&services, conversation_id = state->selectedConversationId]() {
        if (auto *messages_repo = services->messagesRepository(); messages_repo != nullptr) {
          return messages_repo->listConversationMessages(conversation_id, kConversationMessagesPageSize);
        }
        return QVector<domain::Message>{};
      });
      window.setMessages(messages);
      if (state->refreshConversations) {
        // refresh to incorporate server changes + title enrichment
        state->refreshConversations();
      }
      auto list = service_call([&services]() { return services->conversationService()->conversations(); });
      const auto now = QDateTime::currentDateTime();
      for (auto &c : list) {
        if (c.conversationId == state->selectedConversationId) {
          c.lastMessageAt = now;
        }
      }
      std::ranges::sort(list, [](const domain::Conversation &a, const domain::Conversation &b) {
        if (a.pinRank != b.pinRank)
          return a.pinRank > b.pinRank;
        return a.lastMessageAt > b.lastMessageAt;
      });
      window.setConversations(list);
    });

    const bool restored = service_call([&services]() { return services->authService()->restoreSession(); });
    if (restored) {
      window.showMain();
      if (state->refreshConversations) {
        state->refreshConversations();
      }

      const auto ctx = service_call([&services]() { return services->authService()->context(); });
      if (ctx.has_value()) {
        realtime.connectAndAuthenticate(WsUrlForBaseUrl(state->serverBaseUrl), ctx->accessToken);
      }
    } else {
      window.showWelcome();
    }
  });

  QObject::connect(&window, &ui::shell::MainWindow::logoutRequested, [&, services, state]() {
    if (services_initialized) {
      service_call([&services]() { services->authService()->logout(); });
    }
    state->selectedConversationId.clear();
    realtime.close();
    poll_timer.stop();
    window.showWelcome();
  });

  QObject::connect(
      &window, &ui::shell::MainWindow::conversationSelected, [&, services, state](const QString &conversationId) {
        if (!services_initialized) {
          return;
        }
        state->selectedConversationId = conversationId;

        struct SelectionResult {
          bool hasSummary{false};
          QString title;
          int type{0};
          bool selectedConversationIsSelfDm{false};
          QVector<domain::Message> messages{};
        };

        const auto loaded = service_call([&services, &conversationId]() {
          SelectionResult out;

          auto *conversations_api = services->conversationsApi();
          if (conversations_api == nullptr) {
            return out;
          }

          const auto summary = conversations_api->conversation(conversationId);
          if (summary.ok && summary.data.has_value()) {
            out.hasSummary = true;
            out.title = summary.data->title;
            out.type = summary.data->type;

            const auto auth = services->authService()->context();
            const QString my_user_id = auth.has_value() ? auth->userId : QString{};
            out.selectedConversationIsSelfDm = (summary.data->type == 0 && summary.data->peerUserId.has_value() &&
                                                *summary.data->peerUserId == my_user_id);
          }

          const auto batch = conversations_api->conversationEnvelopes(conversationId, kPendingEnvelopesPageSize);
          if (batch.ok && batch.data.has_value()) {
            const auto auth = services->authService()->context();
            const QString my_device_id = auth.has_value() ? auth->deviceId : QString{};
            const QString my_user_id = auth.has_value() ? auth->userId : QString{};
            const QString my_username = auth.has_value() ? auth->username : QString{};
            const bool is_self_dm = out.selectedConversationIsSelfDm;

            for (const auto &env : batch.data->envelopes) {
              domain::Message msg;
              msg.messageId = env.envelopeId;
              msg.conversationId = env.conversationId;
              const QString sender_user_id = env.senderUserId;
              msg.senderDeviceId = env.senderDeviceId;
              msg.serverReceivedAtMs = env.serverTimestamp * kMillisPerSecond;
              msg.clientCreatedAtMs = msg.serverReceivedAtMs;
              msg.ciphertextBlob = env.ciphertext.toUtf8();

              const auto plaintext = DecryptForUi(env.ciphertext);
              if (plaintext.has_value()) {
                msg.plaintextCacheCiphertext = plaintext->toUtf8();
              } else {
                msg.plaintextCacheCiphertext =
                    QString("[Encrypted envelope %1]").arg(env.envelopeId.right(kConversationIdSuffixLength)).toUtf8();
              }

              msg.isOutgoing = is_self_dm ? (!my_device_id.isEmpty() && env.senderDeviceId == my_device_id)
                                          : (!my_user_id.isEmpty() && sender_user_id == my_user_id);

              if (msg.isOutgoing) {
                msg.senderUserId = my_username.isEmpty() ? QStringLiteral("me") : my_username;
              } else {
                auto *directory = services->directoryApi();
                QString display = sender_user_id;
                if (directory != nullptr && !sender_user_id.isEmpty()) {
                  const auto resolved = directory->userById(sender_user_id);
                  if (resolved.ok && resolved.data.has_value() && !resolved.data->username.isEmpty()) {
                    display = resolved.data->username;
                  }
                }
                msg.senderUserId = display;
              }

              if (auto *messages_repo = services->messagesRepository(); messages_repo != nullptr) {
                messages_repo->upsertMessage(msg);
              }
            }
          }

          if (auto *messages_repo = services->messagesRepository(); messages_repo != nullptr) {
            out.messages = messages_repo->listConversationMessages(conversationId, kPendingEnvelopesPageSize);
          }
          return out;
        });

        if (loaded.hasSummary) {
          window.setWindowTitle(QString("Vox Desktop - %1").arg(loaded.title));
          window.setConversationTitle(loaded.title);
          // DM hides author label; group/channel show author for incoming messages.
          window.setShowMessageAuthors(loaded.type != 0);
          state->selectedConversationIsSelfDm = loaded.selectedConversationIsSelfDm;
        }

        window.setMessages(loaded.messages);
      });

  QObject::connect(&window, &ui::shell::MainWindow::createDmRequested, [&, services, state](const QString &username) {
    if (!services_initialized) {
      QMessageBox::information(&window, "Not connected", "Connect to a server first.");
      return;
    }

    struct CreateDmResult {
      bool apiAvailable{true};
      bool userFound{false};
      bool created{false};
      QString conversationId;
    };

    const auto result = service_call([&services, &username]() {
      CreateDmResult out;
      auto *directory = services->directoryApi();
      auto *conversations_api = services->conversationsApi();
      if (directory == nullptr || conversations_api == nullptr) {
        out.apiAvailable = false;
        return out;
      }

      const auto resolved = directory->userByUsername(username);
      if (!resolved.ok || !resolved.data.has_value()) {
        return out;
      }
      out.userFound = true;

      vox::network::ConversationCreateRequest req;
      req.type = "dm";
      req.peerUserId = resolved.data->userId;
      const auto created = conversations_api->createConversation(req);
      if (!created.ok || !created.data.has_value()) {
        return out;
      }

      out.created = true;
      out.conversationId = created.data->conversationId;
      return out;
    });

    if (!result.apiAvailable) {
      QMessageBox::warning(&window, "Unavailable", "Directory/conversation APIs are not available.");
      return;
    }
    if (!result.userFound) {
      QMessageBox::warning(&window, "User not found", "Could not resolve username.");
      return;
    }
    if (!result.created) {
      QMessageBox::warning(&window, "Create failed", "Could not create DM conversation.");
      return;
    }

    const QString new_conversation_id = result.conversationId;
    state->selectedConversationId = new_conversation_id;

    if (state->refreshConversations) {
      state->refreshConversations();
    }

    const auto messages = service_call([&services, &new_conversation_id]() {
      auto *messages_repo = services->messagesRepository();
      if (messages_repo != nullptr) {
        return messages_repo->listConversationMessages(new_conversation_id, kPendingEnvelopesPageSize);
      }
      return QVector<domain::Message>{};
    });
    window.setMessages(messages);
  });

  QObject::connect(
      &window, &ui::shell::MainWindow::createGroupRequested, [&, services, state](const QStringList &usernames) {
        if (!services_initialized) {
          QMessageBox::information(&window, "Not connected", "Connect to a server first.");
          return;
        }

        struct CreateGroupResult {
          bool authenticated{false};
          bool apiAvailable{true};
          bool created{false};
          QString error;
          QString conversationId;
        };

        const auto result = service_call([&services, usernames]() {
          CreateGroupResult out;
          const auto auth = services->authService()->context();
          if (!auth.has_value()) {
            return out;
          }
          out.authenticated = true;

          auto *directory = services->directoryApi();
          auto *conversations_api = services->conversationsApi();
          if (directory == nullptr || conversations_api == nullptr) {
            out.apiAvailable = false;
            return out;
          }

          QVector<QString> member_ids;
          member_ids.push_back(auth->userId);
          for (const auto &u : usernames) {
            const auto resolved = directory->userByUsername(u);
            if (!resolved.ok || !resolved.data.has_value()) {
              out.error = QString("Could not resolve '%1'").arg(u);
              return out;
            }
            member_ids.push_back(resolved.data->userId);
          }
          member_ids.removeDuplicates();

          vox::network::ConversationCreateRequest req;
          req.type = "group";
          req.members = member_ids;
          const auto created = conversations_api->createConversation(req);
          if (!created.ok || !created.data.has_value()) {
            out.error = "Could not create group.";
            return out;
          }

          out.created = true;
          out.conversationId = created.data->conversationId;
          return out;
        });

        if (!result.authenticated) {
          QMessageBox::information(&window, "Not authenticated", "Please login first.");
          return;
        }
        if (!result.apiAvailable) {
          return;
        }
        if (!result.created) {
          if (!result.error.isEmpty()) {
            const QString title = result.error.startsWith("Could not resolve") ? "Resolve failed" : "Create failed";
            QMessageBox::warning(&window, title, result.error);
          }
          return;
        }

        state->selectedConversationId = result.conversationId;
        if (state->refreshConversations) {
          state->refreshConversations();
          QTimer::singleShot(kUiRefreshDelayMs, &window, [state]() {
            if (state->refreshConversations)
              state->refreshConversations();
          });
        }
      });

  QObject::connect(
      &window, &ui::shell::MainWindow::createChannelRequested, [&, services, state](const QStringList &adminUsernames) {
        if (!services_initialized) {
          QMessageBox::information(&window, "Not connected", "Connect to a server first.");
          return;
        }

        struct CreateChannelResult {
          bool authenticated{false};
          bool apiAvailable{true};
          bool created{false};
          QString error;
          QString conversationId;
        };

        const auto result = service_call([&services, adminUsernames]() {
          CreateChannelResult out;
          const auto auth = services->authService()->context();
          if (!auth.has_value()) {
            return out;
          }
          out.authenticated = true;

          auto *directory = services->directoryApi();
          auto *conversations_api = services->conversationsApi();
          if (directory == nullptr || conversations_api == nullptr) {
            out.apiAvailable = false;
            return out;
          }

          QVector<QString> admin_ids;
          admin_ids.push_back(auth->userId);
          for (const auto &u : adminUsernames) {
            const auto resolved = directory->userByUsername(u);
            if (!resolved.ok || !resolved.data.has_value()) {
              out.error = QString("Could not resolve '%1'").arg(u);
              return out;
            }
            admin_ids.push_back(resolved.data->userId);
          }
          admin_ids.removeDuplicates();

          vox::network::ConversationCreateRequest req;
          req.type = "channel";
          req.admins = admin_ids;
          const auto created = conversations_api->createConversation(req);
          if (!created.ok || !created.data.has_value()) {
            out.error = "Could not create channel.";
            return out;
          }

          out.created = true;
          out.conversationId = created.data->conversationId;
          return out;
        });

        if (!result.authenticated) {
          QMessageBox::information(&window, "Not authenticated", "Please login first.");
          return;
        }
        if (!result.apiAvailable) {
          return;
        }
        if (!result.created) {
          if (!result.error.isEmpty()) {
            const QString title = result.error.startsWith("Could not resolve") ? "Resolve failed" : "Create failed";
            QMessageBox::warning(&window, title, result.error);
          }
          return;
        }

        state->selectedConversationId = result.conversationId;
        if (state->refreshConversations) {
          state->refreshConversations();
          QTimer::singleShot(kUiRefreshDelayMs, &window, [state]() {
            if (state->refreshConversations)
              state->refreshConversations();
          });
        }
      });

  QObject::connect(
      &window, &ui::shell::MainWindow::subscribeChannelRequested, [&, services, state](const QString &conversationId) {
        if (!services_initialized) {
          QMessageBox::information(&window, "Not connected", "Connect to a server first.");
          return;
        }

        struct SubscribeResult {
          bool apiAvailable{true};
          bool subscribed{false};
        };

        const auto result = service_call([&services, &conversationId]() {
          SubscribeResult out;
          auto *conversations_api = services->conversationsApi();
          if (conversations_api == nullptr) {
            out.apiAvailable = false;
            return out;
          }
          out.subscribed = conversations_api->subscribe(conversationId).ok;
          return out;
        });

        if (!result.apiAvailable) {
          QMessageBox::warning(&window, "Unavailable", "Conversation API is not available.");
          return;
        }
        if (!result.subscribed) {
          QMessageBox::warning(&window, "Subscribe failed", "Could not subscribe to channel.");
          return;
        }

        state->selectedConversationId = conversationId;
        if (state->refreshConversations) {
          state->refreshConversations();
          QTimer::singleShot(kUiRefreshDelayMs, &window, [state]() {
            if (state->refreshConversations) {
              state->refreshConversations();
            }
          });
          QTimer::singleShot(kUiRefreshRetryDelayMs, &window, [state]() {
            if (state->refreshConversations) {
              state->refreshConversations();
            }
          });
        }
      });

  window.showWelcome();
  window.show();
  const int exit_code = QApplication::exec();

  service_call([&services]() { services.reset(); });
  QObject *const ctx_ptr = service_context.get();
  std::unique_ptr<QObject> context_to_destroy = std::move(service_context);
  QMetaObject::invokeMethod(
      ctx_ptr, [ctx = std::move(context_to_destroy)]() mutable { ctx.reset(); }, Qt::BlockingQueuedConnection);
  service_thread.quit();
  service_thread.wait();

  return exit_code;
}

} // namespace vox::app
