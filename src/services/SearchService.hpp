#ifndef VOX_SERVICES_SEARCHSERVICE_HPP
#define VOX_SERVICES_SEARCHSERVICE_HPP

#include "services/Interfaces.hpp"
#include "storage/Repositories.hpp"

namespace vox::services {

class SearchService final : public ISearchService {
public:
  SearchService(storage::ISearchRepository &repository, QByteArray searchIndexKey);

  bool indexMessage(const QString &conversationId, const QString &messageId, const QString &plaintext) override;

  QVector<QString> findMessageIds(const QString &query) const override;

private:
  QVector<QString> tokenize(const QString &text) const;

  storage::ISearchRepository &m_repository;
  QByteArray m_searchIndexKey;
};

} // namespace vox::services

#endif // VOX_SERVICES_SEARCHSERVICE_HPP
