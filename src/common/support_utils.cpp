#include "support_utils.h"

#include "otp.hpp"
#include "string.h"
#include "mbedtls/sha256.h"
#include "support_utils_lib.hpp"
#include <bsod/bsod.h>

void printerHash(char *str, size_t size) {
    serial_nr_t serial_nr;
    const uint8_t serial_nr_len = otp_get_serial_nr(serial_nr);

    constexpr uint8_t bufferSize = sizeof(STM32_UUID) + sizeof(otp_get_mac_address()->mac) + sizeof(serial_nr);
    uint8_t toHash[bufferSize];
    /// CPU ID
    memcpy(toHash, otp_get_STM32_UUID(), sizeof(STM32_UUID));
    /// MAC
    memcpy(&toHash[sizeof(STM32_UUID)], otp_get_mac_address()->mac, sizeof(otp_get_mac_address()->mac));
    /// SN
    memcpy(&toHash[sizeof(STM32_UUID) + sizeof(otp_get_mac_address()->mac)], serial_nr.begin(), serial_nr_len);

    uint32_t hash[8] = { 0, 0, 0, 0, 0, 0, 0, 0 }; /// 256 bits
    /// get hash;
    const size_t hash_len = sizeof(STM32_UUID) + sizeof(otp_get_mac_address()->mac) + serial_nr_len;
    mbedtls_sha256_ret(toHash, hash_len, (unsigned char *)hash, false);

    /// convert number by 5-bit chunks (32 symbol alphabet)
    debug_assert(sizeof(hash) * 8 >= size * 5);
    for (uint8_t i = 0; i < size; ++i) {
        str[i] = to32((uint8_t *)hash, i * 5);
    }
}
