#include "printer_common.hpp"
#include "hostname.hpp"

#include <config_store/store_instance.hpp>

#include <support_utils.h>
#include <version.h>

#include <mbedtls/sha256.h>

namespace connect_client {

namespace {

    // Some of the dev boards are not properly flashed and have garbage in there.
    // We try to guess that by looking for "invalid" characters in the serial
    // number. We err on the side of accepting something that's not valid SN, we
    // just want to make sure to have something somewhat usable come out of the dev
    // board.
    bool serial_valid(const char *sn) {
        for (const char *c = sn; *c; c++) {
            if (!isprint(*c)) {
                return false;
            }
        }
        return true;
    }

    // "Make up" some semi-unique, semi-stable serial number.
    uint8_t synthetic_serial(serial_nr_t *sn) {
        memset(sn->begin(), 0, sn->size());
        strlcpy(sn->begin(), "DEVX", sn->size());
        // Make sure different things generated based on these data produce different hashes.
        static const char salt[] = "Nj20je98gje";
        mbedtls_sha256_context ctx;
        mbedtls_sha256_init(&ctx);
        mbedtls_sha256_starts_ret(&ctx, false);
        mbedtls_sha256_update_ret(&ctx, (const uint8_t *)salt, sizeof salt);
        uint32_t timestamp = otp_get_timestamp();
        mbedtls_sha256_update_ret(&ctx, (const uint8_t *)&timestamp, sizeof timestamp);
        mbedtls_sha256_update_ret(&ctx, otp_get_STM32_UUID()->uuid, sizeof(otp_get_STM32_UUID()->uuid));
        mbedtls_sha256_update_ret(&ctx, (const uint8_t *)salt, sizeof salt);
        uint8_t hash[32];
        mbedtls_sha256_finish_ret(&ctx, hash);
        mbedtls_sha256_free(&ctx);
        const size_t offset = 4;
        for (size_t i = 0; i < 15; i++) {
            // With 26 letters in the alphabet, this should provide us with nice
            // readable characters.
            (*sn)[i + offset] = 'a' + (hash[i] & 0x0f);
        }
        return 20;
    }

} // namespace

Printer::Config load_eeprom_config() {
    Printer::Config configuration = {};
    configuration.enabled = config_store().connect_enabled.get();
    // (We need it even if disabled for registration phase)
    strlcpy(configuration.host, config_store().connect_host.get().data(), sizeof(configuration.host));
    decompress_host(configuration.host, sizeof(configuration.host));
    strlcpy(configuration.token, config_store().connect_token.get().data(), sizeof(configuration.token));
    configuration.tls = config_store().connect_tls.get();
    configuration.port = config_store().connect_port.get();

    return configuration;
}

void init_info(Printer::PrinterInfo &info) {
    info.firmware_version = project_version_full;
    info.appendix = appendix_exist();

    otp_get_serial_nr(info.serial_number);

    if (!serial_valid(info.serial_number.begin())) {
        synthetic_serial(&info.serial_number);
    }

    printerHash(info.fingerprint, sizeof(info.fingerprint) - 1, false);
    info.fingerprint[sizeof(info.fingerprint) - 1] = '\0';
}

} // namespace connect_client
