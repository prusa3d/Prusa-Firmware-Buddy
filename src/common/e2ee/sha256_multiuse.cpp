#include "sha256_multiuse.hpp"
#include <bsod/bsod.h>

namespace e2ee {

SHA256MultiuseHash::SHA256MultiuseHash() {
    mbedtls_sha256_init(&sha256_ctx);
    mbedtls_sha256_starts_ret(&sha256_ctx, false);
}

SHA256MultiuseHash::~SHA256MultiuseHash() {
    mbedtls_sha256_free(&sha256_ctx);
}

void SHA256MultiuseHash::get_hash(uint8_t *buffer, [[maybe_unused]] size_t buffer_size) {
    debug_assert(buffer_size == e2ee::HASH_SIZE);
    mbedtls_sha256_finish_ret(&sha256_ctx, buffer);
    reset();
}

void SHA256MultiuseHash::update(const uint8_t *data, size_t size) {
    mbedtls_sha256_update_ret(&sha256_ctx, data, size);
}

void SHA256MultiuseHash::reset() {
    mbedtls_sha256_free(&sha256_ctx);
    mbedtls_sha256_init(&sha256_ctx);
    mbedtls_sha256_starts_ret(&sha256_ctx, false);
}

} // namespace e2ee
