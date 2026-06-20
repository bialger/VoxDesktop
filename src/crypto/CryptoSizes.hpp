#ifndef VOX_CRYPTO_CRYPTOSIZES_HPP
#define VOX_CRYPTO_CRYPTOSIZES_HPP

#include <sodium.h>

namespace vox::crypto {

inline constexpr int kChaCha20KeyBytes = crypto_aead_chacha20poly1305_ietf_KEYBYTES;
inline constexpr int kChaCha20NonceBytes = crypto_aead_chacha20poly1305_ietf_NPUBBYTES;
inline constexpr int kChaCha20TagBytes = crypto_aead_chacha20poly1305_ietf_ABYTES;
inline constexpr int kXChaCha20NonceBytes = crypto_aead_xchacha20poly1305_ietf_NPUBBYTES;
inline constexpr int kSha256Bytes = crypto_hash_sha256_BYTES;
inline constexpr int kHkdfDefaultSaltBytes = kSha256Bytes;

inline constexpr int kRatchetCounterBytes = 4;
inline constexpr int kRatchetHeaderBytes = kRatchetCounterBytes + kChaCha20NonceBytes;
inline constexpr int kRatchetMinSerializedBytes = kRatchetHeaderBytes + kChaCha20TagBytes;

} // namespace vox::crypto

#endif // VOX_CRYPTO_CRYPTOSIZES_HPP
