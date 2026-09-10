#include "support_utils.h"

#include "otp.hpp"
#include "mbedtls/sha256.h"
#include "support_utils_lib.hpp"
#include <bsod/bsod.h>

void printerHash(char *str, size_t size) {
    serial_nr_t serial_nr;
    const uint8_t serial_nr_len = otp_get_serial_nr(serial_nr);

    uint8_t hash[32];
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts_ret(&ctx, false);

    /// CPU ID
    mbedtls_sha256_update_ret(&ctx, otp_get_STM32_UUID()->uuid, sizeof(otp_get_STM32_UUID()->uuid));

    /// MAC
    mbedtls_sha256_update_ret(&ctx, otp_get_mac_address()->mac, sizeof(otp_get_mac_address()->mac));

    /// SN
    mbedtls_sha256_update_ret(&ctx, reinterpret_cast<const unsigned char *>(serial_nr.begin()), serial_nr_len);

    mbedtls_sha256_finish_ret(&ctx, hash);
    mbedtls_sha256_free(&ctx);

    /// convert number by 5-bit chunks (32 symbol alphabet)
    debug_assert(sizeof(hash) * 8 >= size * 5);
    for (uint8_t i = 0; i < size; ++i) {
        str[i] = to32(hash, i * 5);
    }
}
