#include "domain/Models.hpp"

#include <QHash>

namespace vox::domain {
namespace {

template<typename Enum>
std::optional<Enum> fromTable(const QHash<QString, Enum> &table, const QString &key) {
  const auto it = table.find(key);
  if (it == table.end()) {
    return std::nullopt;
  }
  return it.value();
}

} // namespace

QString toString(DeliveryState state) {
  switch (state) {
    case DeliveryState::Draft:
      return "Draft";
    case DeliveryState::Queued:
      return "Queued";
    case DeliveryState::Encrypting:
      return "Encrypting";
    case DeliveryState::UploadingAttachments:
      return "UploadingAttachments";
    case DeliveryState::Sending:
      return "Sending";
    case DeliveryState::SentToServer:
      return "SentToServer";
    case DeliveryState::DeliveredToPeerDevices:
      return "DeliveredToPeerDevices";
    case DeliveryState::ReadLocal:
      return "ReadLocal";
    case DeliveryState::Failed:
      return "Failed";
  }
  return "Failed";
}

QString toString(ConversationType type) {
  switch (type) {
    case ConversationType::Dm:
      return "dm";
    case ConversationType::Group:
      return "group";
    case ConversationType::Channel:
      return "channel";
  }
  return "dm";
}

QString toString(TrustLevel state) {
  switch (state) {
    case TrustLevel::Unknown:
      return "unknown";
    case TrustLevel::Unverified:
      return "unverified";
    case TrustLevel::Verified:
      return "verified";
    case TrustLevel::Blocked:
      return "blocked";
  }
  return "unknown";
}

QString toString(ContentKind kind) {
  switch (kind) {
    case ContentKind::Text:
      return "text";
    case ContentKind::Attachment:
      return "attachment";
    case ContentKind::System:
      return "system";
  }
  return "text";
}

std::optional<DeliveryState> deliveryStateFromString(const QString &state) {
  static const QHash<QString, DeliveryState> kMap{
      {"Draft", DeliveryState::Draft},
      {"Queued", DeliveryState::Queued},
      {"Encrypting", DeliveryState::Encrypting},
      {"UploadingAttachments", DeliveryState::UploadingAttachments},
      {"Sending", DeliveryState::Sending},
      {"SentToServer", DeliveryState::SentToServer},
      {"DeliveredToPeerDevices", DeliveryState::DeliveredToPeerDevices},
      {"ReadLocal", DeliveryState::ReadLocal},
      {"Failed", DeliveryState::Failed},
  };
  return fromTable(kMap, state);
}

std::optional<ConversationType> conversationTypeFromString(const QString &type) {
  static const QHash<QString, ConversationType> kMap{
      {"dm", ConversationType::Dm}, {"group", ConversationType::Group}, {"channel", ConversationType::Channel}};
  return fromTable(kMap, type);
}

std::optional<TrustLevel> trustLevelFromString(const QString &state) {
  static const QHash<QString, TrustLevel> kMap{{"unknown", TrustLevel::Unknown},
                                               {"unverified", TrustLevel::Unverified},
                                               {"verified", TrustLevel::Verified},
                                               {"blocked", TrustLevel::Blocked}};
  return fromTable(kMap, state);
}

std::optional<ContentKind> contentKindFromString(const QString &kind) {
  static const QHash<QString, ContentKind> kMap{
      {"text", ContentKind::Text}, {"attachment", ContentKind::Attachment}, {"system", ContentKind::System}};
  return fromTable(kMap, kind);
}

} // namespace vox::domain
