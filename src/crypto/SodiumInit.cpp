#include "crypto/SodiumInit.hpp"

#include <sodium.h>

namespace vox::crypto {

bool ensureSodiumInitialized() {
    return sodium_init() >= 0;
}

} // namespace vox::crypto
