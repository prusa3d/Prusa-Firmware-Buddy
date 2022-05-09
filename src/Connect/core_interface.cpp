#include "core_interface.hpp"
#include <cstring>
#include <otp.h>
#include <ini.h>
#include "version.h"
#include "support_utils.h"
#include <ini.h>
#include <eeprom.h>
#include <printers.h>

#include <cctype>
#include <cassert>
#include <string_view>
#include <mbedtls/sha256.h>

using std::string_view;

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

}

void core_interface::get_data(device_params_t *params) {
    if (NULL == params) {
        assert(0);
        return;
    }

    memset(params, 0, sizeof(device_params_t));

    if (marlin_vars) {
        marlin_client_loop();
        switch (marlin_vars->print_state) {
        case mpsIdle:
        case mpsAborted:
            if (DEVICE_STATE_PREPARED != params->state)
                params->state = DEVICE_STATE_READY;
            break;
        case mpsPrinting:
        case mpsAborting_Begin:
        case mpsAborting_WaitIdle:
        case mpsAborting_ParkHead:
        case mpsFinishing_WaitIdle:
        case mpsFinishing_ParkHead:
            params->state = DEVICE_STATE_PRINTING;
            break;
        case mpsPausing_Begin:
        case mpsPausing_WaitIdle:
        case mpsPausing_ParkHead:
        case mpsPaused:
        case mpsResuming_Begin:
        case mpsResuming_Reheating:
            //    case mpsResuming_UnparkHead:
            params->state = DEVICE_STATE_PAUSED;
            break;
        case mpsFinished:
            if (DEVICE_STATE_PREPARED != params->state)
                params->state = DEVICE_STATE_FINISHED;
            break;
        default:
            params->state = DEVICE_STATE_UNKNOWN;
            break;
        }
        params->temp_bed = marlin_vars->temp_bed;
        params->target_bed = marlin_vars->target_bed;
        params->temp_nozzle = marlin_vars->temp_nozzle;
        params->target_nozzle = marlin_vars->target_nozzle;
        params->pos[X_AXIS_POS] = marlin_vars->pos[X_AXIS_POS];
        params->pos[Y_AXIS_POS] = marlin_vars->pos[Y_AXIS_POS];
        params->pos[Z_AXIS_POS] = marlin_vars->pos[Z_AXIS_POS];
        params->print_speed = marlin_vars->print_speed;
        params->flow_factor = marlin_vars->flow_factor;
    }
}

core_interface::core_interface() {
    marlin_vars = marlin_client_init();
    marlin_client_set_change_notify(MARLIN_VAR_MSK_DEF | MARLIN_VAR_MSK_WUI, NULL);
}

std::optional<Error> core_interface::get_printer_info(printer_info_t *printer_info) {
    if (NULL == printer_info)
        return Error::ERROR;

    size_t len = strlen(project_version_full);
    if (len < FW_VER_BUFR_LEN)
        strlcpy(printer_info->firmware_version, project_version_full, FW_VER_BUFR_LEN);
    else
        return Error::ERROR;

    printer_info->printer_type = PRINTER_TYPE;
    //FIXME!
    // The serial number is a temporary hack. This must be finalized. Currently
    // the Connect server expects CZPXddddXdddx#ddddd where 'd' is a digit (0 - 9)
    // and often is not the case on the hardware.
    memcpy(printer_info->serial_number, "CZPX", 4);

    for (int i = 0; i < OTP_SERIAL_NUMBER_SIZE; i++) {
        printer_info->serial_number[i + 4] = *(volatile char *)(OTP_SERIAL_NUMBER_ADDR + i);
    }

    printer_info->serial_number[8] = 'X';
    printer_info->serial_number[12] = 'X';

    for (int i = 4; i < 8; i++) {
        if (!isdigit(printer_info->serial_number[i]))
            printer_info->serial_number[i] = '0';
    }

    for (int i = 9; i < 12; i++) {
        if (!isdigit(printer_info->serial_number[i]))
            printer_info->serial_number[i] = '0';
    }

    for (int i = 14; i < 19; i++) {
        if (!isdigit(printer_info->serial_number[i]))
            printer_info->serial_number[i] = '0';
    }
    printer_info->serial_number[SER_NUM_STR_LEN] = 0;

    // Prusa connect requires 16 long fingerprint. Printer code is 8 characters.
    // Copy printer code behind itself to make it 16 characters long.
    //FIXME! This is just a temporary solution!
    printerCode(printer_info->fingerprint); //Compute hash(8 bytes) from uid, serial and mac
    if (!(8 == strlen(printer_info->fingerprint)))
        return Error::ERROR;
    memcpy(printer_info->fingerprint + FINGERPRINT_SIZE / 2, printer_info->fingerprint, FINGERPRINT_SIZE / 2);
    printer_info->fingerprint[FINGERPRINT_SIZE] = 0;

    printer_info->appendix = appendix_exist();
    return std::nullopt;
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
