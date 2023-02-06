#include "marlin_printer.hpp"
#include "hostname.hpp"

#include <eeprom.h>
#include <ini.h>
#include <version.h>
#include <support_utils.h>
#include <otp.h>
#include <odometer.hpp>
#include <netdev.h>
#include <print_utils.hpp>
#include <wui_api.h>
#include <filament.h> //get_selected_filament_name

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <mbedtls/sha256.h>

using std::nullopt;

namespace connect_client {

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

        auto *config = reinterpret_cast<Printer::Config *>(user);
        size_t len = strlen(value);

        if (ini_string_match(section, INI_SECTION, name, "hostname")) {
            char buffer[sizeof config->host];
            if (compress_host(value, buffer, sizeof buffer)) {
                strlcpy(config->host, buffer, sizeof config->host);
                config->loaded = true;
            } else {
                return 0;
            }
        } else if (ini_string_match(section, INI_SECTION, name, "token")) {
            if (len <= CONNECT_TOKEN_SIZE) {
                strlcpy(config->token, value, sizeof config->token);
                config->loaded = true;
            } else {
                return 0;
            }
        } else if (ini_string_match(section, INI_SECTION, name, "port")) {
            char *endptr;
            long tmp = strtol(value, &endptr, 10);
            if (*endptr == '\0' && tmp >= 0 && tmp <= 65535) {
                config->port = (uint16_t)tmp;
                config->loaded = true;
            } else {
                return 0;
            }
        } else if (ini_string_match(section, INI_SECTION, name, "tls")) {
            if (strcmp(value, "1") == 0 || strcasecmp(value, "true") == 0) {
                config->tls = true;
                config->loaded = true;
            } else if (strcmp(value, "0") == 0 || strcasecmp(value, "false") == 0) {
                config->tls = false;
                config->loaded = true;
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
    bool serial_valid(const char *sn) {
        for (const char *c = sn; *c; c++) {
            if (!isalnum(*c)) {
                return false;
            }
        }
        return true;
    }

    // "Make up" some semi-unique, semi-stable serial number.
    uint8_t synthetic_serial(char sn[Printer::PrinterInfo::SER_NUM_BUFR_LEN]) {
        memset(sn, 0, Printer::PrinterInfo::SER_NUM_BUFR_LEN);
        strlcpy(sn, "DEVX", Printer::PrinterInfo::SER_NUM_BUFR_LEN);
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

    Printer::DeviceState to_device_state(marlin_print_state_t state, bool ready) {
        switch (state) {
        case mpsPrintPreviewQuestions:
            return Printer::DeviceState::Attention;
        case mpsIdle:
        case mpsWaitGui:
        case mpsPrintPreviewInit:
        case mpsPrintPreviewImage:
        case mpsPrintInit:
        case mpsAborted:
        case mpsExit:
            if (ready) {
                return Printer::DeviceState::Ready;
            } else {
                return Printer::DeviceState::Idle;
            }
        case mpsPrinting:
        case mpsAborting_Begin:
        case mpsAborting_WaitIdle:
        case mpsAborting_ParkHead:
        case mpsFinishing_WaitIdle:
        case mpsFinishing_ParkHead:
            return Printer::DeviceState::Printing;

        case mpsPowerPanic_acFault:
        case mpsPowerPanic_Resume:
        case mpsPowerPanic_AwaitingResume:
        case mpsCrashRecovery_Begin:
        case mpsCrashRecovery_Axis_NOK:
        case mpsCrashRecovery_Retracting:
        case mpsCrashRecovery_Lifting:
        case mpsCrashRecovery_XY_Measure:
        case mpsCrashRecovery_XY_HOME:
        case mpsCrashRecovery_Repeated_Crash:
            return Printer::DeviceState::Busy;

        case mpsPausing_Begin:
        case mpsPausing_WaitIdle:
        case mpsPausing_ParkHead:
        case mpsPaused:

        case mpsResuming_Begin:
        case mpsResuming_Reheating:
        case mpsPausing_Failed_Code:
        case mpsResuming_UnparkHead_XY:
        case mpsResuming_UnparkHead_ZE:
            return Printer::DeviceState::Paused;
        case mpsFinished:
            if (ready) {
                return Printer::DeviceState::Ready;
            } else {
                return Printer::DeviceState::Finished;
            }
        }
        return Printer::DeviceState::Unknown;
    }

    // Extract a fixed-sized string from EEPROM to provided buffer.
    //
    // maxlen is the length of the buffer, including the byte for \0.
    //
    // FIXME: Unify with the one in wui_api.c
    void strextract(char *into, size_t maxlen, enum eevar_id var) {
        variant8_t tmp = eeprom_get_var(var);
        strlcpy(into, variant8_get_pch(tmp), maxlen);
        variant8_t *ptmp = &tmp;
        variant8_done(&ptmp);
    }
}

MarlinPrinter::MarlinPrinter(SharedBuffer &buffer)
    : buffer(buffer) {
    marlin_vars = marlin_client_init();
    assert(marlin_vars != nullptr);
    marlin_client_set_change_notify(MARLIN_VAR_MSK_DEF | MARLIN_VAR_MSK_WUI, NULL);

    info.firmware_version = project_version_full;
    info.appendix = appendix_exist();

    memcpy(info.serial_number, "CZPX", 4);
    for (int i = 0; i < OTP_SERIAL_NUMBER_SIZE; i++) {
        info.serial_number[i + 4] = *(volatile char *)(OTP_SERIAL_NUMBER_ADDR + i);
    }
    info.serial_number[sizeof(info.serial_number) - 1] = 0;

    if (!serial_valid(info.serial_number)) {
        synthetic_serial(info.serial_number);
    }

    printerHash(info.fingerprint, sizeof(info.fingerprint) - 1, false);
    info.fingerprint[sizeof(info.fingerprint) - 1] = '\0';

    info.appendix = appendix_exist();
}

void MarlinPrinter::renew() {
    if (auto b = buffer.borrow(); b.has_value()) {
        marlin_vars->media_LFN = reinterpret_cast<char *>(b->data());
        marlin_vars->media_SFN_path = reinterpret_cast<char *>(b->data() + FILE_NAME_BUFFER_LEN);
    } else {
        marlin_vars->media_LFN = nullptr;
        marlin_vars->media_SFN_path = nullptr;
    }
    marlin_update_vars(MARLIN_VAR_MSK_DEF | MARLIN_VAR_MSK_WUI);
    // Any suspicious state, like Busy or Printing will cancel the printer-ready state.
    //
    // (We kind of assume there's no chance of renew not being called between a
    // print starts and ends and that we'll see it.).
    if (to_device_state(marlin_vars->print_state, ready) != DeviceState::Ready) {
        ready = false;
    }
}

Printer::Params MarlinPrinter::params() const {
    Params params = {};
    params.material = get_selected_filament_name();
    params.state = to_device_state(marlin_vars->print_state, ready);
    params.temp_bed = marlin_vars->temp_bed;
    params.target_bed = marlin_vars->target_bed;
    params.temp_nozzle = marlin_vars->temp_nozzle;
    params.target_nozzle = marlin_vars->target_nozzle;
    params.pos[X_AXIS_POS] = marlin_vars->curr_pos[X_AXIS_POS];
    params.pos[Y_AXIS_POS] = marlin_vars->curr_pos[Y_AXIS_POS];
    params.pos[Z_AXIS_POS] = marlin_vars->curr_pos[Z_AXIS_POS];
    params.print_speed = marlin_vars->print_speed;
    params.flow_factor = marlin_vars->flow_factor;
    params.job_id = marlin_vars->job_id;
    params.job_path = marlin_vars->media_SFN_path;
    params.job_lfn = marlin_vars->media_LFN;
    params.print_fan_rpm = marlin_vars->print_fan_rpm;
    params.heatbreak_fan_rpm = marlin_vars->heatbreak_fan_rpm;
    params.print_duration = marlin_vars->print_duration;
    params.time_to_end = marlin_vars->time_to_end;
    params.progress_percent = marlin_vars->sd_percent_done;
    params.filament_used = Odometer_s::instance().get(Odometer_s::axis_t::E);
    params.has_usb = marlin_vars->media_inserted;

    return params;
}

Printer::Config MarlinPrinter::load_config() {
    Config configuration = {};
    configuration.enabled = eeprom_get_bool(EEVAR_CONNECT_ENABLED);
    if (configuration.enabled) {
        // Just avoiding to read it when disabled, only to save some CPU
        strextract(configuration.host, sizeof configuration.host, EEVAR_CONNECT_HOST);
        decompress_host(configuration.host, sizeof configuration.host);
        strextract(configuration.token, sizeof configuration.token, EEVAR_CONNECT_TOKEN);
        configuration.tls = eeprom_get_bool(EEVAR_CONNECT_TLS);
        configuration.port = eeprom_get_ui16(EEVAR_CONNECT_PORT);
    }

    return configuration;
}

bool MarlinPrinter::load_cfg_from_ini() {
    Config config;
    bool ok = ini_parse("/usb/prusa_printer_settings.ini", connect_ini_handler, &config) == 0;
    ok = ok && config.loaded;
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

std::optional<Printer::NetInfo> MarlinPrinter::net_info(Printer::Iface iface) const {
    uint32_t id;
    switch (iface) {
    case Iface::Ethernet:
        id = NETDEV_ETH_ID;
        break;
    case Iface::Wifi:
        id = NETDEV_ESP_ID;
        break;
    default:
        assert(0);
        return nullopt;
    }
    if (netdev_get_status(id) != NETDEV_NETIF_UP) {
        return nullopt;
    }
    NetInfo result = {};
    if (!netdev_get_MAC_address(id, result.mac)) {
        return nullopt;
    }
    lan_t addrs;
    netdev_get_ipv4_addresses(id, &addrs);
    static_assert(sizeof(addrs.addr_ip4) == sizeof(result.ip));
    memcpy(result.ip, &addrs.addr_ip4, sizeof addrs.addr_ip4);
    return result;
}

Printer::NetCreds MarlinPrinter::net_creds() const {
    NetCreds result = {};
    strextract(result.pl_password, sizeof result.pl_password, EEVAR_PL_PASSWORD);
    strextract(result.ssid, sizeof result.ssid, EEVAR_WIFI_AP_SSID);
    return result;
}

bool MarlinPrinter::job_control(JobControl control) {
    // Renew was presumably called before short.
    DeviceState state = to_device_state(marlin_vars->print_state, false);

    switch (control) {
    case JobControl::Pause:
        if (state == DeviceState::Printing) {
            marlin_print_pause();
            return true;
        } else {
            return false;
        }
    case JobControl::Resume:
        if (state == DeviceState::Paused) {
            marlin_print_resume();
            return true;
        } else {
            return false;
        }
    case JobControl::Stop:
        if (state == DeviceState::Paused || state == DeviceState::Printing || state == DeviceState::Attention) {
            marlin_print_abort();
            return true;
        } else {
            return false;
        }
    }
    assert(0);
    return false;
}

bool MarlinPrinter::start_print(const char *path) {
    if (!marlin_remote_print_ready(false)) {
        return false;
    }

    print_begin(path, true);
    return marlin_print_started();
}

void MarlinPrinter::submit_gcode(const char *code) {
    marlin_gcode(code);
}

bool MarlinPrinter::set_ready(bool ready) {
    if (ready && marlin_is_printing()) {
        return false;
    }

    this->ready = ready;
    return true;
}

bool MarlinPrinter::is_printing() const {
    return marlin_is_printing();
}

uint32_t MarlinPrinter::files_hash() const {
    return wui_gcodes_mods();
}

}
