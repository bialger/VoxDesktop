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
#include <QSqlDatabase>
#include <QSslSocket>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>
#ifdef VOX_HAS_QT_WEBSOCKETS
#include <QWebSocket>
#endif

#include <algorithm>
#include <functional>
#include <memory>
#include <utility>

namespace vox::app {
namespace {

QByteArray b64(const QByteArray &bytes) {
  return bytes.toBase64(QByteArray::Base64Encoding);
}

std::optional<QString> decryptForUi(const QString &ciphertext) {
  if (ciphertext.trimmed().isEmpty()) {
    return QString{};
  }

  const auto asUtf8 = ciphertext.toUtf8();
  if (ciphertext.startsWith("vox1:")) {
    const QByteArray decoded = QByteArray::fromBase64(asUtf8.mid(5));
    if (!decoded.isEmpty()) {
      return QString::fromUtf8(decoded);
    }
  }

  // plaintext-looking fallback
  bool looksPlain = true;
  for (const auto ch : asUtf8) {
    if (ch == 0) {
      looksPlain = false;
      break;
    }
  }
  if (looksPlain) {
    return ciphertext;
  }

  const QByteArray decoded = QByteArray::fromBase64(asUtf8);
  if (!decoded.isEmpty()) {
    return QString::fromUtf8(decoded);
  }
  return std::nullopt;
}

QString encryptForTransport(const QString &plaintext) {
  return QStringLiteral("vox1:") + QString::fromUtf8(b64(plaintext.toUtf8()));
}

QUrl wsUrlForBaseUrl(QString baseUrl) {
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

bool startupDiagnostics(QString *error) {
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

  const QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  if (appData.isEmpty()) {
    *error = "No writable AppData path";
    return false;
  }

  QDir appDataDir(appData);
  if (!appDataDir.exists() && !appDataDir.mkpath(".")) {
    *error = "Failed to create AppData directory";
    return false;
  }

  QFile probe(appData + "/.vox_write_probe");
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

} // namespace

int Application::run(int argc, char **argv) {
  QApplication app(argc, argv);
  app.setWindowIcon(QIcon(":/vox_logo.png"));

  QString diagnosticsError;
  if (!startupDiagnostics(&diagnosticsError)) {
    QMessageBox::critical(nullptr, "Vox startup failure", diagnosticsError);
    return 1;
  }

  const QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  const QString vaultPassword = qEnvironmentVariableIsSet("VOX_VAULT_PASSWORD")
                                    ? QString::fromUtf8(qgetenv("VOX_VAULT_PASSWORD"))
                                    : QStringLiteral("vox-dev-password");

  ui::shell::MainWindow window;
  window.setWindowIcon(QIcon(":/vox_logo.png"));

  auto services = std::make_shared<app::ServiceLocator>();
  struct UiState {
    QString selectedConversationId;
    std::function<void()> refreshConversations;
    QString serverBaseUrl;
    bool selectedConversationIsSelfDm{false};
  };
  auto state = std::make_shared<UiState>();
  bool servicesInitialized = false;

  // Realtime: prefer WebSocket push, fallback to polling /v1/sync/pending.
  auto *realtime = new network::RealtimeSocket(&window);
  QTimer pollTimer(&window);
  pollTimer.setInterval(2000);
  QString pendingCursor;

  auto applyEnvelope = [&, services, state](const network::EnvelopeDto &env) {
    if (!servicesInitialized)
      return;
    const auto auth = services->authService()->context();
    const QString myDeviceId = auth.has_value() ? auth->deviceId : QString{};
    const QString myUserId = auth.has_value() ? auth->userId : QString{};
    const QString myUsername = auth.has_value() ? auth->username : QString{};

    domain::Message msg;
    msg.messageId = env.envelopeId;
    msg.conversationId = env.conversationId;
    const QString senderUserId = env.senderUserId;
    msg.senderDeviceId = env.senderDeviceId;
    msg.serverReceivedAtMs = env.serverTimestamp * 1000;
    msg.clientCreatedAtMs = msg.serverReceivedAtMs;
    msg.ciphertextBlob = env.ciphertext.toUtf8();

    const auto plaintext = decryptForUi(env.ciphertext);
    if (plaintext.has_value()) {
      msg.plaintextCacheCiphertext = plaintext->toUtf8();
    } else {
      msg.plaintextCacheCiphertext = QString("[Encrypted envelope %1]").arg(env.envelopeId.right(6)).toUtf8();
    }
    bool isSelfDm = false;
    if (env.conversationId == state->selectedConversationId) {
      isSelfDm = state->selectedConversationIsSelfDm;
    } else {
      // Only needed for correct "self DM" device-based outgoing.
      auto *conversationsApi = services->conversationsApi();
      if (conversationsApi != nullptr) {
        const auto conv = conversationsApi->conversation(env.conversationId);
        if (conv.ok && conv.data.has_value() && conv.data->type == 0 && conv.data->peerUserId.has_value() &&
            myUserId == *conv.data->peerUserId) {
          isSelfDm = true;
        }
      }
    }

    // Outgoing rule:
    // - normal conversations: any message from my user -> right bubble
    // - DM with self: only messages from *this device* -> right bubble
    msg.isOutgoing = isSelfDm ? (!myDeviceId.isEmpty() && env.senderDeviceId == myDeviceId)
                              : (!myUserId.isEmpty() && senderUserId == myUserId);

    // Author label: store username for UI (not UUID).
    if (msg.isOutgoing) {
      msg.senderUserId = myUsername.isEmpty() ? QStringLiteral("me") : myUsername;
    } else {
      auto *directory = services->directoryApi();
      QString display = senderUserId;
      if (directory != nullptr && !senderUserId.isEmpty()) {
        const auto resolved = directory->userById(senderUserId);
        if (resolved.ok && resolved.data.has_value() && !resolved.data->username.isEmpty()) {
          display = resolved.data->username;
        }
      }
      msg.senderUserId = display;
    }

    if (auto *messagesRepo = services->messagesRepository(); messagesRepo != nullptr) {
      messagesRepo->upsertMessage(msg);
    }

    // Update currently open conversation view.
    if (env.conversationId == state->selectedConversationId) {
      if (auto *messagesRepo = services->messagesRepository(); messagesRepo != nullptr) {
        window.setMessages(messagesRepo->listConversationMessages(env.conversationId, 200));
      }
    }

    // Bump chat ordering in UI quickly.
    auto list = services->conversationService()->conversations();
    const auto now = QDateTime::currentDateTime();
    for (auto &c : list) {
      if (c.conversationId == env.conversationId) {
        c.lastMessageAt = now;
      }
    }
    std::sort(list.begin(), list.end(), [](const domain::Conversation &a, const domain::Conversation &b) {
      if (a.pinRank != b.pinRank)
        return a.pinRank > b.pinRank;
      return a.lastMessageAt > b.lastMessageAt;
    });
    window.setConversations(list);
  };

  QObject::connect(realtime, &network::RealtimeSocket::envelopeReceived, &window, applyEnvelope);
  QObject::connect(realtime, &network::RealtimeSocket::membershipChanged, &window, [&, services](const QString &, int) {
    if (servicesInitialized && services->conversationService()->refreshConversations()) {
      window.setConversations(services->conversationService()->conversations());
    }
  });
  QObject::connect(realtime, &network::RealtimeSocket::socketError, &window, [&](const QString &) {
    if (servicesInitialized) {
      pollTimer.start();
    }
  });

  QObject::connect(&pollTimer, &QTimer::timeout, &window, [&, services, state]() {
    if (!servicesInitialized)
      return;
    auto *api = services->conversationsApi();
    if (api == nullptr)
      return;
    const auto batch = api->pendingEnvelopes(100, pendingCursor);
    if (!batch.ok || !batch.data.has_value()) {
      return;
    }
    pendingCursor = batch.data->nextCursor;
    for (const auto &env : batch.data->envelopes) {
      applyEnvelope(env);
    }
  });

  QObject::connect(&window, &ui::shell::MainWindow::serverBaseUrlReady, [&, services, state](const QString &baseUrl) {
    if (servicesInitialized) {
      return;
    }

    QString initError;
    if (!services->initialize(appDataPath, baseUrl, vaultPassword, &initError)) {
      QMessageBox::critical(&window, "Vox startup failure", initError);
      return;
    }
    servicesInitialized = true;
    state->serverBaseUrl = baseUrl;

    state->refreshConversations = [&, services, state]() {
      if (services->conversationService()->refreshConversations()) {
        const auto list = services->conversationService()->conversations();
        window.setConversations(list);
        if (state->selectedConversationId.isEmpty() && !list.isEmpty()) {
          state->selectedConversationId = list.first().conversationId;
        }
      }
    };

    QObject::connect(
        &window, &ui::shell::MainWindow::conversationSelected, [&, services, state](const QString &conversationId) {
          state->selectedConversationId = conversationId;
          auto *messagesRepo = services->messagesRepository();
          if (messagesRepo != nullptr) {
            window.setMessages(messagesRepo->listConversationMessages(conversationId, 100));
          }
        });

    QObject::connect(&window,
                     &ui::shell::MainWindow::loginSubmitted,
                     [&, services, state](const QString &username, const QString &passwordDerived) {
                       if (!services->authService()->login(username, passwordDerived)) {
                         QMessageBox::warning(&window, "Login failed", "Invalid credentials or server error");
                         return;
                       }

                       window.showMain();
                       if (state->refreshConversations) {
                         state->refreshConversations();
                       }

                       // Start realtime after auth.
                       const auto ctx = services->authService()->context();
                       if (ctx.has_value()) {
                         realtime->connectAndAuthenticate(wsUrlForBaseUrl(state->serverBaseUrl), ctx->accessToken);
                       }
                     });

    QObject::connect(&window,
                     &ui::shell::MainWindow::registerSubmitted,
                     [&, services, state](const QString &username, const QString &passwordDerived) {
                       if (!services->authService()->registerUser(username, passwordDerived)) {
                         QMessageBox::warning(
                             &window, "Registration failed", "Could not register account with current server");
                         return;
                       }

                       window.showMain();
                       if (state->refreshConversations) {
                         state->refreshConversations();
                       }

                       const auto ctx = services->authService()->context();
                       if (ctx.has_value()) {
                         realtime->connectAndAuthenticate(wsUrlForBaseUrl(state->serverBaseUrl), ctx->accessToken);
                       }
                     });

    QObject::connect(&window, &ui::shell::MainWindow::sendMessageSubmitted, [&, services, state](const QString &text) {
      const auto auth = services->authService()->context();
      if (!auth.has_value()) {
        QMessageBox::information(&window, "Not authenticated", "Please login before sending messages.");
        return;
      }

      if (state->selectedConversationId.isEmpty()) {
        QMessageBox::information(&window, "No conversation selected", "Select or create a conversation first.");
        return;
      }

      const auto convDto = services->conversationsApi()->conversation(state->selectedConversationId);
      const bool isChannel = convDto.ok && convDto.data.has_value() && convDto.data->type == 2;
      const QString myRole = (convDto.ok && convDto.data.has_value()) ? convDto.data->myRole.toLower() : QString{};
      const bool canSendInChannel = (myRole == "owner" || myRole == "admin");
      if (isChannel && !canSendInChannel) {
        QMessageBox::information(&window, "Read-only", "You don't have permission to post to this channel.");
        return;
      }

      auto *sender = services->messageSendService();
      const QString ciphertext = encryptForTransport(text);
      const bool ok =
          (sender != nullptr) && sender->sendMessage(state->selectedConversationId, auth->deviceId, text, ciphertext);
      if (!ok) {
        QMessageBox::warning(&window, "Send failed", "Message queued for retry.");
      }

      // Immediate UI update (messages + chat ordering) even if server list is stale.
      if (auto *messagesRepo = services->messagesRepository(); messagesRepo != nullptr) {
        window.setMessages(messagesRepo->listConversationMessages(state->selectedConversationId, 200));
      }
      if (state->refreshConversations) {
        // refresh to incorporate server changes + title enrichment
        state->refreshConversations();
      }
      auto list = services->conversationService()->conversations();
      const auto now = QDateTime::currentDateTime();
      for (auto &c : list) {
        if (c.conversationId == state->selectedConversationId) {
          c.lastMessageAt = now;
        }
      }
      std::sort(list.begin(), list.end(), [](const domain::Conversation &a, const domain::Conversation &b) {
        if (a.pinRank != b.pinRank)
          return a.pinRank > b.pinRank;
        return a.lastMessageAt > b.lastMessageAt;
      });
      window.setConversations(list);
    });

    if (services->authService()->restoreSession()) {
      window.showMain();
      if (state->refreshConversations) {
        state->refreshConversations();
      }

      const auto ctx = services->authService()->context();
      if (ctx.has_value()) {
        realtime->connectAndAuthenticate(wsUrlForBaseUrl(state->serverBaseUrl), ctx->accessToken);
      }
    } else {
      window.showWelcome();
    }
  });

  QObject::connect(&window, &ui::shell::MainWindow::logoutRequested, [&, services, state]() {
    if (servicesInitialized) {
      services->authService()->logout();
    }
    state->selectedConversationId.clear();
    realtime->close();
    pollTimer.stop();
    window.showWelcome();
  });

  QObject::connect(
      &window, &ui::shell::MainWindow::conversationSelected, [&, services, state](const QString &conversationId) {
        if (!servicesInitialized) {
          return;
        }
        state->selectedConversationId = conversationId;

        auto *conversationsApi = services->conversationsApi();
        if (conversationsApi == nullptr) {
          return;
        }

        const auto summary = conversationsApi->conversation(conversationId);
        if (summary.ok && summary.data.has_value()) {
          window.setWindowTitle(QString("Vox Desktop - %1").arg(summary.data->title));
          window.setConversationTitle(summary.data->title);
          // DM hides author label; group/channel show author for incoming messages.
          window.setShowMessageAuthors(summary.data->type != 0);

          const auto auth = services->authService()->context();
          const QString myUserId = auth.has_value() ? auth->userId : QString{};
          state->selectedConversationIsSelfDm = (summary.data->type == 0 && summary.data->peerUserId.has_value() &&
                                                 *summary.data->peerUserId == myUserId);
        }

        const auto batch = conversationsApi->conversationEnvelopes(conversationId, 100);
        if (batch.ok && batch.data.has_value()) {
          const auto auth = services->authService()->context();
          const QString myDeviceId = auth.has_value() ? auth->deviceId : QString{};
          const QString myUserId = auth.has_value() ? auth->userId : QString{};
          const QString myUsername = auth.has_value() ? auth->username : QString{};
          const bool isSelfDm = state->selectedConversationIsSelfDm;

          for (const auto &env : batch.data->envelopes) {
            domain::Message msg;
            msg.messageId = env.envelopeId;
            msg.conversationId = env.conversationId;
            const QString senderUserId = env.senderUserId;
            msg.senderDeviceId = env.senderDeviceId;
            msg.serverReceivedAtMs = env.serverTimestamp * 1000;
            msg.clientCreatedAtMs = msg.serverReceivedAtMs;
            msg.ciphertextBlob = env.ciphertext.toUtf8();

            const auto plaintext = decryptForUi(env.ciphertext);
            if (plaintext.has_value()) {
              msg.plaintextCacheCiphertext = plaintext->toUtf8();
            } else {
              msg.plaintextCacheCiphertext = QString("[Encrypted envelope %1]").arg(env.envelopeId.right(6)).toUtf8();
            }

            msg.isOutgoing = isSelfDm ? (!myDeviceId.isEmpty() && env.senderDeviceId == myDeviceId)
                                      : (!myUserId.isEmpty() && senderUserId == myUserId);

            if (msg.isOutgoing) {
              msg.senderUserId = myUsername.isEmpty() ? QStringLiteral("me") : myUsername;
            } else {
              auto *directory = services->directoryApi();
              QString display = senderUserId;
              if (directory != nullptr && !senderUserId.isEmpty()) {
                const auto resolved = directory->userById(senderUserId);
                if (resolved.ok && resolved.data.has_value() && !resolved.data->username.isEmpty()) {
                  display = resolved.data->username;
                }
              }
              msg.senderUserId = display;
            }

            services->messagesRepository()->upsertMessage(msg);
          }
        }

        auto *messagesRepo = services->messagesRepository();
        if (messagesRepo != nullptr) {
          window.setMessages(messagesRepo->listConversationMessages(conversationId, 100));
        }
      });

  QObject::connect(&window, &ui::shell::MainWindow::createDmRequested, [&, services, state](const QString &username) {
    if (!servicesInitialized) {
      QMessageBox::information(&window, "Not connected", "Connect to a server first.");
      return;
    }

    auto *directory = services->directoryApi();
    auto *conversationsApi = services->conversationsApi();
    if (directory == nullptr || conversationsApi == nullptr) {
      QMessageBox::warning(&window, "Unavailable", "Directory/conversation APIs are not available.");
      return;
    }

    const auto resolved = directory->userByUsername(username);
    if (!resolved.ok || !resolved.data.has_value()) {
      QMessageBox::warning(&window, "User not found", "Could not resolve username.");
      return;
    }

    vox::network::ConversationCreateRequest req;
    req.type = "dm";
    req.peerUserId = resolved.data->userId;
    const auto created = conversationsApi->createConversation(req);
    if (!created.ok || !created.data.has_value()) {
      QMessageBox::warning(&window, "Create failed", "Could not create DM conversation.");
      return;
    }

    const QString newConversationId = created.data->conversationId;
    state->selectedConversationId = newConversationId;

    if (state->refreshConversations) {
      state->refreshConversations();
    }

    auto *messagesRepo = services->messagesRepository();
    if (messagesRepo != nullptr) {
      window.setMessages(messagesRepo->listConversationMessages(newConversationId, 100));
    }
  });

  QObject::connect(
      &window, &ui::shell::MainWindow::createGroupRequested, [&, services, state](const QStringList &usernames) {
        if (!servicesInitialized) {
          QMessageBox::information(&window, "Not connected", "Connect to a server first.");
          return;
        }
        const auto auth = services->authService()->context();
        if (!auth.has_value()) {
          QMessageBox::information(&window, "Not authenticated", "Please login first.");
          return;
        }

        auto *directory = services->directoryApi();
        auto *conversationsApi = services->conversationsApi();
        if (directory == nullptr || conversationsApi == nullptr) {
          return;
        }

        QVector<QString> memberIds;
        memberIds.push_back(auth->userId);
        for (const auto &u : usernames) {
          const auto resolved = directory->userByUsername(u);
          if (!resolved.ok || !resolved.data.has_value()) {
            QMessageBox::warning(&window, "Resolve failed", QString("Could not resolve '%1'").arg(u));
            return;
          }
          memberIds.push_back(resolved.data->userId);
        }
        memberIds.removeDuplicates();

        vox::network::ConversationCreateRequest req;
        req.type = "group";
        req.members = memberIds;
        const auto created = conversationsApi->createConversation(req);
        if (!created.ok || !created.data.has_value()) {
          QMessageBox::warning(&window, "Create failed", "Could not create group.");
          return;
        }
        state->selectedConversationId = created.data->conversationId;
        if (state->refreshConversations) {
          state->refreshConversations();
          QTimer::singleShot(1500, &window, [state]() {
            if (state->refreshConversations)
              state->refreshConversations();
          });
        }
      });

  QObject::connect(
      &window, &ui::shell::MainWindow::createChannelRequested, [&, services, state](const QStringList &adminUsernames) {
        if (!servicesInitialized) {
          QMessageBox::information(&window, "Not connected", "Connect to a server first.");
          return;
        }
        const auto auth = services->authService()->context();
        if (!auth.has_value()) {
          QMessageBox::information(&window, "Not authenticated", "Please login first.");
          return;
        }

        auto *directory = services->directoryApi();
        auto *conversationsApi = services->conversationsApi();
        if (directory == nullptr || conversationsApi == nullptr) {
          return;
        }

        QVector<QString> adminIds;
        adminIds.push_back(auth->userId);
        for (const auto &u : adminUsernames) {
          const auto resolved = directory->userByUsername(u);
          if (!resolved.ok || !resolved.data.has_value()) {
            QMessageBox::warning(&window, "Resolve failed", QString("Could not resolve '%1'").arg(u));
            return;
          }
          adminIds.push_back(resolved.data->userId);
        }
        adminIds.removeDuplicates();

        vox::network::ConversationCreateRequest req;
        req.type = "channel";
        req.admins = adminIds;
        const auto created = conversationsApi->createConversation(req);
        if (!created.ok || !created.data.has_value()) {
          QMessageBox::warning(&window, "Create failed", "Could not create channel.");
          return;
        }
        state->selectedConversationId = created.data->conversationId;
        if (state->refreshConversations) {
          state->refreshConversations();
          QTimer::singleShot(1500, &window, [state]() {
            if (state->refreshConversations)
              state->refreshConversations();
          });
        }
      });

  QObject::connect(
      &window, &ui::shell::MainWindow::subscribeChannelRequested, [&, services, state](const QString &conversationId) {
        if (!servicesInitialized) {
          QMessageBox::information(&window, "Not connected", "Connect to a server first.");
          return;
        }

        auto *conversationsApi = services->conversationsApi();
        if (conversationsApi == nullptr) {
          QMessageBox::warning(&window, "Unavailable", "Conversation API is not available.");
          return;
        }

        const auto result = conversationsApi->subscribe(conversationId);
        if (!result.ok) {
          QMessageBox::warning(&window, "Subscribe failed", "Could not subscribe to channel.");
          return;
        }

        state->selectedConversationId = conversationId;
        if (state->refreshConversations) {
          state->refreshConversations();
          QTimer::singleShot(1500, &window, [state]() {
            if (state->refreshConversations) {
              state->refreshConversations();
            }
          });
          QTimer::singleShot(3500, &window, [state]() {
            if (state->refreshConversations) {
              state->refreshConversations();
            }
          });
        }
      });

  window.showWelcome();
  window.show();
  return QApplication::exec();
}

} // namespace vox::app
