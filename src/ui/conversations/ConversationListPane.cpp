#include "ui/conversations/ConversationListPane.hpp"

#include <QHBoxLayout>
#include <QInputDialog>
#include <QMenu>
#include <QRegularExpression>
#include <QStringList>
#include <QVBoxLayout>

namespace vox::ui::conversations {
namespace {

class ChatListProxyModel final : public QSortFilterProxyModel {
public:
  void setTypeFilter(std::optional<domain::ConversationType> type) {
    m_typeFilter = type;
    invalidateFilter();
  }

protected:
  bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override {
    const QModelIndex idx = sourceModel()->index(sourceRow, 0, sourceParent);
    const QString typeStr = idx.data(ConversationListModel::TypeRole).toString();

    if (m_typeFilter.has_value()) {
      const QString expected = domain::toString(*m_typeFilter);
      if (typeStr != expected) {
        return false;
      }
    }

    const QString needle = filterRegularExpression().pattern().trimmed();
    if (needle.isEmpty()) {
      return true;
    }

    const QString title = idx.data(ConversationListModel::TitleRole).toString();
    const QString typeLabel = idx.data(ConversationListModel::TypeLabelRole).toString();

    return title.contains(needle, Qt::CaseInsensitive) || typeLabel.contains(needle, Qt::CaseInsensitive);
  }

private:
  std::optional<domain::ConversationType> m_typeFilter;
};

QStringList splitUsernames(QString raw) {
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

ConversationListPane::ConversationListPane(QWidget *parent) : QWidget(parent) {
  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(8, 8, 8, 8);

  m_tabBar = new QTabBar(this);
  m_tabBar->addTab("DMs");
  m_tabBar->addTab("Groups");
  m_tabBar->addTab("Channels");
  m_tabBar->setExpanding(false);

  auto *topRow = new QHBoxLayout;
  m_searchEdit = new QLineEdit(this);
  m_searchEdit->setPlaceholderText("Search conversations");

  m_addButton = new QToolButton(this);
  m_addButton->setText("+");
  m_addButton->setToolTip("Add conversation");
  m_addButton->setPopupMode(QToolButton::InstantPopup);

  auto *menu = new QMenu(m_addButton);
  auto *dmAction = menu->addAction("New DM (by username)");
  auto *groupAction = menu->addAction("New Group (usernames)");
  auto *channelAction = menu->addAction("New Channel (admins)");
  auto *subscribeAction = menu->addAction("Subscribe to channel (by UUID)");
  m_addButton->setMenu(menu);

  topRow->addWidget(m_searchEdit, 1);
  topRow->addWidget(m_addButton);

  m_model = new ConversationListModel(this);
  auto *proxy = new ChatListProxyModel;
  proxy->setSourceModel(m_model);
  proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
  proxy->setFilterRegularExpression(QRegularExpression{});
  proxy->setTypeFilter(domain::ConversationType::Dm);
  m_proxyModel = proxy;

  m_listView = new QListView(this);
  m_listView->setModel(m_proxyModel);

  layout->addWidget(m_tabBar);
  layout->addLayout(topRow);
  layout->addWidget(m_listView, 1);

  connect(m_searchEdit, &QLineEdit::textChanged, this, [proxy](const QString &text) {
    proxy->setFilterRegularExpression(
        QRegularExpression(QRegularExpression::escape(text), QRegularExpression::CaseInsensitiveOption));
  });

  connect(m_tabBar, &QTabBar::currentChanged, this, [proxy](int idx) {
    if (idx == 0) {
      proxy->setTypeFilter(domain::ConversationType::Dm);
    } else if (idx == 1) {
      proxy->setTypeFilter(domain::ConversationType::Group);
    } else {
      proxy->setTypeFilter(domain::ConversationType::Channel);
    }
  });

  connect(dmAction, &QAction::triggered, this, [this]() {
    bool ok = false;
    const QString username = QInputDialog::getText(this, "New DM", "Username:", QLineEdit::Normal, {}, &ok).trimmed();
    if (!ok || username.isEmpty()) {
      return;
    }
    emit createDmRequested(username);
  });

  connect(groupAction, &QAction::triggered, this, [this]() {
    bool ok = false;
    const QString raw =
        QInputDialog::getMultiLineText(this, "New Group", "Usernames (comma/newline separated):", {}, &ok).trimmed();
    if (!ok || raw.isEmpty()) {
      return;
    }
    const auto usernames = splitUsernames(raw);
    if (usernames.isEmpty()) {
      return;
    }
    emit createGroupRequested(usernames);
  });

  connect(channelAction, &QAction::triggered, this, [this]() {
    bool ok = false;
    const QString raw = QInputDialog::getMultiLineText(
                            this, "New Channel", "Admin usernames (optional; comma/newline separated):", {}, &ok)
                            .trimmed();
    if (!ok) {
      return;
    }
    const auto usernames = splitUsernames(raw);
    emit createChannelRequested(usernames);
  });

  connect(subscribeAction, &QAction::triggered, this, [this]() {
    bool ok = false;
    const QString raw =
        QInputDialog::getText(this, "Subscribe to channel", "Channel UUID (conv_...):", QLineEdit::Normal, {}, &ok)
            .trimmed();
    if (!ok || raw.isEmpty()) {
      return;
    }
    const QRegularExpression re(R"(conv_[A-Za-z0-9_-]+)");
    const auto match = re.match(raw);
    const QString conversationId = match.hasMatch() ? match.captured(0) : raw;
    emit subscribeChannelRequested(conversationId);
  });

  connect(m_listView, &QListView::clicked, this, [this](const QModelIndex &proxyIndex) {
    const QModelIndex sourceIndex = m_proxyModel->mapToSource(proxyIndex);
    const QString conversationId = sourceIndex.data(ConversationListModel::ConversationIdRole).toString();
    emit conversationSelected(conversationId);
  });
}

ConversationListModel *ConversationListPane::model() {
  return m_model;
}

} // namespace vox::ui::conversations
