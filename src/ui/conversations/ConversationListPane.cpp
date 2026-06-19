#include "ui/conversations/ConversationListPane.hpp"

#include <QHBoxLayout>
#include <QInputDialog>
#include <QMenu>
#include <QRegularExpression>
#include <QStringList>
#include <QVBoxLayout>

#include <memory>

namespace vox::ui::conversations {
namespace {

constexpr int kLayoutMarginPx = 8;

class ChatListProxyModel final : public QSortFilterProxyModel {
public:
  void SetTypeFilter(std::optional<domain::ConversationType> type) {
    m_typeFilter_ = type;
    invalidateFilter();
  }

protected:
  [[nodiscard]] bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override {
    const QModelIndex idx = sourceModel()->index(sourceRow, 0, sourceParent);
    const QString type_str = idx.data(ConversationListModel::TypeRole).toString();

    if (m_typeFilter_.has_value()) {
      const QString expected = domain::toString(*m_typeFilter_);
      if (type_str != expected) {
        return false;
      }
    }

    const QString needle = filterRegularExpression().pattern().trimmed();
    if (needle.isEmpty()) {
      return true;
    }

    const QString title = idx.data(ConversationListModel::TitleRole).toString();
    const QString type_label = idx.data(ConversationListModel::TypeLabelRole).toString();

    return title.contains(needle, Qt::CaseInsensitive) || type_label.contains(needle, Qt::CaseInsensitive);
  }

private:
  std::optional<domain::ConversationType> m_typeFilter_;
};

QStringList SplitUsernames(QString raw) {
  raw.replace('\n', ',');
  raw.replace('\r', ',');
  const auto parts = raw.split(',', Qt::SkipEmptyParts);
  QStringList out;
  out.reserve(parts.size());
  for (auto p : parts) {
    p = p.trimmed();
    if (!p.isEmpty()) {
      out.push_back(p);
    }
  }
  out.removeDuplicates();
  return out;
}

} // namespace

ConversationListPane::ConversationListPane(QWidget *parent) :
    QWidget(parent), m_tabBar(new QTabBar(this)), m_searchEdit(new QLineEdit(this)), m_addButton(new QToolButton(this)),
    m_model(new ConversationListModel(this)), m_listView(new QListView(this)) {
  auto layout = std::make_unique<QVBoxLayout>();
  layout->setContentsMargins(kLayoutMarginPx, kLayoutMarginPx, kLayoutMarginPx, kLayoutMarginPx);

  m_tabBar->addTab("DMs");
  m_tabBar->addTab("Groups");
  m_tabBar->addTab("Channels");
  m_tabBar->setExpanding(false);

  auto top_row = std::make_unique<QHBoxLayout>();

  m_searchEdit->setPlaceholderText("Search conversations");

  m_addButton->setText("+");
  m_addButton->setToolTip("Add conversation");
  m_addButton->setPopupMode(QToolButton::InstantPopup);

  auto menu = std::make_unique<QMenu>(m_addButton);
  auto *dm_action = menu->addAction("New DM (by username)");
  auto *group_action = menu->addAction("New Group (usernames)");
  auto *channel_action = menu->addAction("New Channel (admins)");
  auto *subscribe_action = menu->addAction("Subscribe to channel (by UUID)");
  m_addButton->setMenu(menu.release());

  top_row->addWidget(m_searchEdit, 1);
  top_row->addWidget(m_addButton);

  auto proxy = std::make_unique<ChatListProxyModel>();
  proxy->setSourceModel(m_model);
  proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
  proxy->setFilterRegularExpression(QRegularExpression{});
  proxy->SetTypeFilter(domain::ConversationType::Dm);
  ChatListProxyModel *const proxy_model = proxy.get();
  m_proxyModel = proxy.release();
  m_proxyModel->setParent(this);

  m_listView->setModel(m_proxyModel);

  layout->addWidget(m_tabBar);
  layout->addLayout(top_row.release());
  layout->addWidget(m_listView, 1);

  connect(m_searchEdit, &QLineEdit::textChanged, this, [proxy_model](const QString &text) {
    proxy_model->setFilterRegularExpression(
        QRegularExpression(QRegularExpression::escape(text), QRegularExpression::CaseInsensitiveOption));
  });

  connect(m_tabBar, &QTabBar::currentChanged, this, [proxy_model](int idx) {
    if (idx == 0) {
      proxy_model->SetTypeFilter(domain::ConversationType::Dm);
    } else if (idx == 1) {
      proxy_model->SetTypeFilter(domain::ConversationType::Group);
    } else {
      proxy_model->SetTypeFilter(domain::ConversationType::Channel);
    }
  });

  connect(dm_action, &QAction::triggered, this, [this]() {
    bool ok = false;
    const QString username = QInputDialog::getText(this, "New DM", "Username:", QLineEdit::Normal, {}, &ok).trimmed();
    if (!ok || username.isEmpty()) {
      return;
    }
    emit createDmRequested(username);
  });

  connect(group_action, &QAction::triggered, this, [this]() {
    bool ok = false;
    const QString raw =
        QInputDialog::getMultiLineText(this, "New Group", "Usernames (comma/newline separated):", {}, &ok).trimmed();
    if (!ok || raw.isEmpty()) {
      return;
    }
    const auto usernames = SplitUsernames(raw);
    if (usernames.isEmpty()) {
      return;
    }
    emit createGroupRequested(usernames);
  });

  connect(channel_action, &QAction::triggered, this, [this]() {
    bool ok = false;
    const QString raw = QInputDialog::getMultiLineText(
                            this, "New Channel", "Admin usernames (optional; comma/newline separated):", {}, &ok)
                            .trimmed();
    if (!ok) {
      return;
    }
    const auto usernames = SplitUsernames(raw);
    emit createChannelRequested(usernames);
  });

  connect(subscribe_action, &QAction::triggered, this, [this]() {
    bool ok = false;
    const QString raw =
        QInputDialog::getText(this, "Subscribe to channel", "Channel UUID (conv_...):", QLineEdit::Normal, {}, &ok)
            .trimmed();
    if (!ok || raw.isEmpty()) {
      return;
    }
    const QRegularExpression re(R"(conv_[A-Za-z0-9_-]+)");
    const auto match = re.match(raw);
    const QString conversation_id = match.hasMatch() ? match.captured(0) : raw;
    emit subscribeChannelRequested(conversation_id);
  });

  connect(m_listView, &QListView::clicked, this, [this](const QModelIndex &proxyIndex) {
    const QModelIndex source_index = m_proxyModel->mapToSource(proxyIndex);
    const QString conversation_id = source_index.data(ConversationListModel::ConversationIdRole).toString();
    emit conversationSelected(conversation_id);
  });
  setLayout(layout.release());
}

ConversationListModel *ConversationListPane::model() {
  return m_model;
}

} // namespace vox::ui::conversations
