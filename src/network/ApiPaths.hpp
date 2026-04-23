#ifndef VOX_NETWORK_APIPATHS_HPP
#define VOX_NETWORK_APIPATHS_HPP

#include <QString>

namespace vox::network::ApiPaths {

inline const QString kHealth = "/v1/health";

inline const QString kRegister = "/v1/register";
inline const QString kLogin = "/v1/login";
inline const QString kRefresh = "/v1/refresh";
inline const QString kMe = "/v1/me";
inline const QString kLogout = "/v1/logout";
inline const QString kChangePassword = "/v1/account/change-password";

inline const QString kUsersByUsernamePrefix = "/v1/users/by-username/";
inline const QString kUsersSearch = "/v1/users/search";
inline const QString kUsersPrefix = "/v1/users/";

inline const QString kConversations = "/v1/conversations";
inline const QString kMessagesSend = "/v1/messages/send";
inline const QString kMessagesAck = "/v1/messages/ack";
inline const QString kSyncPending = "/v1/sync/pending";

inline const QString kAttachmentsUploadInit = "/v1/attachments/upload-init";
inline const QString kAttachmentsPrefix = "/v1/attachments/";

inline const QString kSyncKeyBundle = "/v1/sync/key-bundle";
inline const QString kSyncChanges = "/v1/sync/changes";
inline const QString kSyncRecordsPrefix = "/v1/sync/records/";

inline const QString kWebSocket = "/v1/ws";

} // namespace vox::network::ApiPaths

#endif // VOX_NETWORK_APIPATHS_HPP
