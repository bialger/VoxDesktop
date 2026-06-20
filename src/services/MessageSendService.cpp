#include "services/MessageSendService.hpp"

#include <QDateTime>
#include <QUuid>

namespace vox::services {
namespace {

constexpr qint64 kMillisPerSecond = 1000;
constexpr qint64 kMessageRetryDelayMs = 1000;

} // namespace

MessageSendService::MessageSendService(network::ConversationsApi &api,
                                       storage::IMessagesRepository &messagesRepository,
                                       storage::IJobsRepository &jobsRepository) :
    m_api(api), m_messagesRepository(messagesRepository), m_jobsRepository(jobsRepository) {
}

bool MessageSendService::sendCiphertext(const QString &conversationId,
                                        const QString &deviceId,
                                        const QString &ciphertext,
                                        int envelopeType) {
  if (conversationId.isEmpty() || deviceId.isEmpty() || ciphertext.isEmpty()) {
    return false;
  }

  const QString envelope_id = QUuid::createUuid().toString(QUuid::WithoutBraces);

  network::SendEnvelopeRequest request;
  request.deviceId = deviceId;
  request.conversationId = conversationId;
  request.ciphertext = ciphertext;
  request.envelopeId = envelope_id;
  request.envelopeType = envelopeType;

  const auto sent = m_api.sendEnvelope(request);

  domain::Message message;
  message.messageId = envelope_id;
  message.conversationId = conversationId;
  message.senderDeviceId = deviceId;
  message.senderUserId = "me";
  message.clientCreatedAtMs = QDateTime::currentMSecsSinceEpoch();
  message.serverReceivedAtMs = sent.data.has_value() ? (sent.data->serverTimestamp * kMillisPerSecond) : 0;
  message.ciphertextBlob = ciphertext.toUtf8();
  message.isOutgoing = true;

  if (sent.ok) {
    message.deliveryState = domain::DeliveryState::SentToServer;
    return m_messagesRepository.upsertMessage(message);
  }

  message.deliveryState = domain::DeliveryState::Queued;
  const bool stored = m_messagesRepository.upsertMessage(message);
  const bool queued = m_jobsRepository.enqueueJob(
      "send_message", ciphertext.toUtf8(), QDateTime::currentMSecsSinceEpoch() + kMessageRetryDelayMs);
  return stored && queued;
}

bool MessageSendService::sendMessage(const QString &conversationId,
                                     const QString &deviceId,
                                     const QString &plaintext,
                                     const QString &ciphertext,
                                     int envelopeType) {
  if (plaintext.isEmpty()) {
    return false;
  }

  // Reuse existing path, but ensure plaintext is cached immediately for UI.
  if (conversationId.isEmpty() || deviceId.isEmpty() || ciphertext.isEmpty()) {
    return false;
  }

  const QString envelope_id = QUuid::createUuid().toString(QUuid::WithoutBraces);

  network::SendEnvelopeRequest request;
  request.deviceId = deviceId;
  request.conversationId = conversationId;
  request.ciphertext = ciphertext;
  request.envelopeId = envelope_id;
  request.envelopeType = envelopeType;

  const auto sent = m_api.sendEnvelope(request);

  domain::Message message;
  message.messageId = envelope_id;
  message.conversationId = conversationId;
  message.senderDeviceId = deviceId;
  message.senderUserId = "me";
  message.clientCreatedAtMs = QDateTime::currentMSecsSinceEpoch();
  message.serverReceivedAtMs = sent.data.has_value() ? (sent.data->serverTimestamp * kMillisPerSecond) : 0;
  message.ciphertextBlob = ciphertext.toUtf8();
  message.plaintextCacheCiphertext = plaintext.toUtf8();
  message.isOutgoing = true;

  if (sent.ok) {
    message.deliveryState = domain::DeliveryState::SentToServer;
    return m_messagesRepository.upsertMessage(message);
  }

  message.deliveryState = domain::DeliveryState::Queued;
  const bool stored = m_messagesRepository.upsertMessage(message);
  const bool queued = m_jobsRepository.enqueueJob(
      "send_message", ciphertext.toUtf8(), QDateTime::currentMSecsSinceEpoch() + kMessageRetryDelayMs);
  return stored && queued;
}

} // namespace vox::services
