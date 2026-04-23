#ifndef VOX_SERVICES_NOTIFICATIONSERVICE_HPP
#define VOX_SERVICES_NOTIFICATIONSERVICE_HPP

#include "services/Interfaces.hpp"

namespace vox::services {

class NotificationService final : public INotificationService {
public:
  explicit NotificationService(bool showPreviews);

  QString notificationPreview(const QString &sender, const QString &message) const override;

  void setShowPreviews(bool enabled);

private:
  bool m_showPreviews{false};
};

} // namespace vox::services

#endif // VOX_SERVICES_NOTIFICATIONSERVICE_HPP
