#include "utils/crypto.hpp"

#include <openssl/rand.h>
#include <cstdint>
#include <argon2.h>
#include <stdexcept>

static constexpr uint32_t kTimeCost = 2;
static constexpr uint32_t kMemoryCost = 1 << 16; // 64 MB
static constexpr uint32_t kParallelism = 1;
static constexpr size_t kHashLen = 32;
static constexpr size_t kSaltLen = 16;

namespace user_service::crypto {

std::string HashEncode(std::string_view password) {
    uint8_t salt[kSaltLen];
    RAND_bytes(salt, kSaltLen);

    const size_t encoded_len = argon2_encodedlen(
        kTimeCost, kMemoryCost, kParallelism, kSaltLen, kHashLen, Argon2_id
    );
    std::string encoded(encoded_len, '\0');

    int rc = argon2id_hash_encoded(
        kTimeCost, kMemoryCost, kParallelism,
        password.data(), password.size(),
        salt, kSaltLen,
        kHashLen,
        encoded.data(), encoded_len
    );

    if (rc != ARGON2_OK) {
        throw std::runtime_error(argon2_error_message(rc));
    }

    return encoded; // "$argon2id$v=19$m=65536,t=2,p=1$<salt>$<hash>"
}

bool VerifyHash(std::string_view password, std::string_view encoded) {
    int rc = argon2id_verify(encoded.data(), password.data(), password.size());
    if (rc == ARGON2_OK) return true;
    if (rc == ARGON2_VERIFY_MISMATCH) return false;
    throw std::runtime_error(argon2_error_message(rc));
}

}