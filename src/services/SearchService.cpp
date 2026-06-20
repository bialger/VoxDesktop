#include "services/SearchService.hpp"

#include <QRegularExpression>
#include "crypto/CryptoHelpers.hpp"

namespace vox::services {

SearchService::SearchService(storage::ISearchRepository &repository, QByteArray searchIndexKey) :
    m_repository(repository), m_searchIndexKey(std::move(searchIndexKey)) {
}

bool SearchService::indexMessage(const QString &conversationId, const QString &messageId, const QString &plaintext) {
  const auto tokens = tokenize(plaintext);
  int position = 0;

  for (const auto &token : tokens) {
    const auto digest = crypto::CryptoHelpers::hmacSha256(m_searchIndexKey, token.toUtf8());
    if (digest.isEmpty() || !m_repository.insertToken(conversationId, messageId, digest, position++)) {
      return false;
    }
  }

  return true;
}

QVector<QString> SearchService::findMessageIds(const QString &query) const {
  const auto tokens = tokenize(query);
  if (tokens.isEmpty()) {
    return {};
  }

  const auto digest = crypto::CryptoHelpers::hmacSha256(m_searchIndexKey, tokens.first().toUtf8());
  return m_repository.findMessagesByToken(digest);
}

QVector<QString> SearchService::tokenize(const QString &text) const {
  const QString normalized = text.trimmed().toLower();
  const auto parts = normalized.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
  QVector<QString> tokens;
  tokens.reserve(parts.size());
  for (const auto &part : parts) {
    tokens.push_back(part);
  }
  return tokens;
}

} // namespace vox::services
