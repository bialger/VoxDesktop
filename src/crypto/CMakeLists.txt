target_sources(vox_core PRIVATE
    SodiumInit.cpp
    CryptoHelpers.cpp
    HkdfSha256.cpp
    VaultService.cpp
    IdentityKeyStore.cpp
    PrekeyManager.cpp
    X3dhSessionBuilder.cpp
    DoubleRatchetSession.cpp
    SenderKeyManager.cpp
    AttachmentCrypto.cpp
    SyncCrypto.cpp
    FingerprintService.cpp
)

