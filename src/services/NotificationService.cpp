#include "services/NotificationService.hpp"

namespace vox::services {

NotificationService::NotificationService(bool showPreviews) : m_showPreviews(showPreviews) {}

QString NotificationService::notificationPreview(const QString &sender, const QString &message) const {
    if (!m_showPreviews) {
        return QString("%1: New encrypted message").arg(sender);
    }
    return QString("%1: %2").arg(sender, message);
}

void NotificationService::setShowPreviews(bool enabled) {
    m_showPreviews = enabled;
}

} // namespace vox::services
