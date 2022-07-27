#include "core_interface.hpp"
#include <cstring>
#include <otp.h>
#include <ini.h>
#include "version.h"
#include "support_utils.h"
#include <ini.h>
#include <eeprom.h>
#include <printers.h>

#include <cstdlib>
#include <cctype>
#include <cassert>
#include <string_view>
#include <mbedtls/sha256.h>

using std::string_view;
using std::variant;

namespace con {

namespace {

    const constexpr char *const INI_SECTION = "service::connect";

    bool ini_string_match(const char *section, const char *section_var,
        const char *name, const char *name_var) {
        return strcmp(section_var, section) == 0 && strcmp(name_var, name) == 0;
    }

    // TODO: How do we extract some user-friendly error indicator what exactly is wrong?
    int connect_ini_handler(void *user, const char *section, const char *name,
        const char *value) {
        // TODO: Can this even happen? How?
        if (user == nullptr || section == nullptr || name == nullptr || value == nullptr) {
            return 0;
        }

        configuration_t *config = reinterpret_cast<configuration_t *>(user);
        size_t len = strlen(value);

        if (ini_string_match(section, INI_SECTION, name, "hostname")) {
            if (len <= CONNECT_HOST_SIZE) {
                strlcpy(config->host, value, sizeof config->host);
            } else {
                return 0;
            }
        } else if (ini_string_match(section, INI_SECTION, name, "token")) {
            if (len <= CONNECT_TOKEN_SIZE) {
                strlcpy(config->token, value, sizeof config->token);
            } else {
                return 0;
            }
        } else if (ini_string_match(section, INI_SECTION, name, "port")) {
            char *endptr;
            long tmp = strtol(value, &endptr, 10);
            if (*endptr == '\0' && tmp >= 0 && tmp <= 65535) {
                config->port = (uint16_t)tmp;
            } else {
                return 0;
            }
        } else if (ini_string_match(section, INI_SECTION, name, "tls")) {
            if (strcmp(value, "1") == 0 || strcasecmp(value, "true") == 0) {
                config->tls = true;
            } else if (strcmp(value, "0") == 0 || strcasecmp(value, "false") == 0) {
                config->tls = false;
            } else {
                return 0;
            }
        }
        return 1;
    }

    // Some of the dev boards are not properly flashed and have garbage in there.
    // We try to guess that by looking for "invalid" characters in the serial
    // number. We err on the side of accepting something that's not valid SN, we
    // just want to make sure to have something somewhat usable come out of the dev
    // board.
    static bool serial_valid(const char *sn) {
        for (const char *c = sn; *c; c++) {
            if (!isalnum(*c)) {
                return false;
            }
        }
        return true;
    }

    // "Make up" some semi-unique, semi-stable serial number.
    static uint8_t synthetic_serial(char sn[SER_NUM_BUFR_LEN]) {
        memset(sn, 0, SER_NUM_BUFR_LEN);
        strlcpy(sn, "DEVX", SER_NUM_BUFR_LEN);
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
            // With 25 letters in the alphabet, this should provide us with nice
            // readable characters.
            sn[i + offset] = 'a' + (hash[i] & 0x0f);
        }
        return 20;
    }
}

device_params_t core_interface::get_data() {
    device_params_t params;
    memset(&params, 0, sizeof params);

    if (marlin_vars) {
        marlin_update_vars(MARLIN_VAR_MSK_DEF | MARLIN_VAR_MSK_WUI);
        switch (marlin_vars->print_state) {
        case mpsIdle:
        case mpsAborted:
            if (DEVICE_STATE_PREPARED != params.state)
                params.state = DEVICE_STATE_READY;
            break;
        case mpsPrinting:
        case mpsAborting_Begin:
        case mpsAborting_WaitIdle:
        case mpsAborting_ParkHead:
        case mpsFinishing_WaitIdle:
        case mpsFinishing_ParkHead:
            params.state = DEVICE_STATE_PRINTING;
            break;
        case mpsPausing_Begin:
        case mpsPausing_WaitIdle:
        case mpsPausing_ParkHead:
        case mpsPaused:
        case mpsResuming_Begin:
        case mpsResuming_Reheating:
            //    case mpsResuming_UnparkHead:
            params.state = DEVICE_STATE_PAUSED;
            break;
        case mpsFinished:
            if (DEVICE_STATE_PREPARED != params.state)
                params.state = DEVICE_STATE_FINISHED;
            break;
        default:
            params.state = DEVICE_STATE_UNKNOWN;
            break;
        }
        params.temp_bed = marlin_vars->temp_bed;
        params.target_bed = marlin_vars->target_bed;
        params.temp_nozzle = marlin_vars->temp_nozzle;
        params.target_nozzle = marlin_vars->target_nozzle;
        params.pos[X_AXIS_POS] = marlin_vars->pos[X_AXIS_POS];
        params.pos[Y_AXIS_POS] = marlin_vars->pos[Y_AXIS_POS];
        params.pos[Z_AXIS_POS] = marlin_vars->pos[Z_AXIS_POS];
        params.print_speed = marlin_vars->print_speed;
        params.flow_factor = marlin_vars->flow_factor;
        params.job_id = marlin_vars->job_id;
        params.job_path = marlin_vars->media_SFN_path;
    }

    return params;
}

core_interface::core_interface() {
    marlin_vars = marlin_client_init();
    marlin_client_set_change_notify(MARLIN_VAR_MSK_DEF | MARLIN_VAR_MSK_WUI, NULL);
    if (marlin_vars) {
        /*
         * Note: We currently have only a single marlin client for
         * connect, we can use a single buffer.
         */
        static char SFN_path[FILE_PATH_BUFFER_LEN];
        marlin_vars->media_SFN_path = SFN_path;
    }
}

printer_info_t core_interface::get_printer_info() {
    printer_info_t info = {};
    size_t len = strlen(project_version_full);
    (void)len;
    assert(len < FW_VER_BUFR_LEN);
    strlcpy(info.firmware_version, project_version_full, FW_VER_BUFR_LEN);

    info.printer_type = PRINTER_TYPE;

    memcpy(info.serial_number, "CZPX", 4);
    for (int i = 0; i < OTP_SERIAL_NUMBER_SIZE; i++) {
        info.serial_number[i + 4] = *(volatile char *)(OTP_SERIAL_NUMBER_ADDR + i);
    }
    info.serial_number[SER_NUM_STR_LEN] = 0;

    if (!serial_valid(info.serial_number)) {
        synthetic_serial(info.serial_number);
    }

    // Prusa connect requires 16 long fingerprint. Printer code is 8 characters.
    // Copy printer code behind itself to make it 16 characters long.
    //FIXME! This is just a temporary solution!
    printerCode(info.fingerprint); //Compute hash(8 bytes) from uid, serial and mac
    assert(strlen(info.fingerprint) == 8);
    memcpy(info.fingerprint + FINGERPRINT_SIZE / 2, info.fingerprint, FINGERPRINT_SIZE / 2);
    info.fingerprint[FINGERPRINT_SIZE] = 0;

    info.appendix = appendix_exist();
    return info;
}

// Extract a fixed-sized string from EEPROM to provided buffer.
//
// maxlen is the length of the buffer, including the byte for \0.
//
// FIXME: Unify with the one in wui_api.c
static void strextract(char *into, size_t maxlen, enum eevar_id var) {
    variant8_t tmp = eeprom_get_var(var);
    strlcpy(into, variant8_get_pch(tmp), maxlen);
    variant8_t *ptmp = &tmp;
    variant8_done(&ptmp);
}

configuration_t core_interface::get_connect_config() {
    configuration_t configuration = {};
    configuration.enabled = eeprom_get_bool(EEVAR_CONNECT_ENABLED);
    if (configuration.enabled) {
        // Just avoiding to read it when disabled, only to save some CPU
        strextract(configuration.host, sizeof configuration.host, EEVAR_CONNECT_HOST);
        strextract(configuration.token, sizeof configuration.token, EEVAR_CONNECT_TOKEN);
        configuration.tls = eeprom_get_bool(EEVAR_CONNECT_TLS);
        configuration.port = eeprom_get_ui16(EEVAR_CONNECT_PORT);
    }

    if (configuration.host[0] == '\0' || configuration.token[0] == '\0') {
        // It's turned on, but no configuration. Just don't try anything in the following code.
        configuration.enabled = false;
    }

    return configuration;
}

bool load_config_from_ini() {
    configuration_t config = {};
    bool ok = ini_parse("/usb/prusa_printer_settings.ini", connect_ini_handler, &config) == 0;
    if (ok) {
        if (config.port == 0) {
            config.port = config.tls ? 443 : 80;
        }

        eeprom_set_pchar(EEVAR_CONNECT_HOST, config.host, 0, 1);
        eeprom_set_pchar(EEVAR_CONNECT_TOKEN, config.token, 0, 1);
        eeprom_set_ui16(EEVAR_CONNECT_PORT, config.port);
        eeprom_set_bool(EEVAR_CONNECT_TLS, config.tls);
        // Note: enabled is controlled in the GUI
    }
    return ok;
}

}
