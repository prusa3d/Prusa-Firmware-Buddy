#include "marlin_printer.hpp"
#include "hostname.hpp"

#include <ini.h>
#include <version.h>
#include <support_utils.h>
#include <otp.hpp>
#include <odometer.hpp>
#include <netdev.h>
#include <print_utils.hpp>
#include <wui_api.h>
#include <filament.hpp>
#include <state/printer_state.hpp>

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <mbedtls/sha256.h>
#include <sys/statvfs.h>

#include <config_store/store_instance.hpp>

using printer_state::DeviceState;
using printer_state::get_state;
using std::atomic;
using std::move;
using std::nullopt;
using namespace marlin_server;

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
            if (len <= config_store_ns::connect_token_size) {
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
            // With 25 letters in the alphabet, this should provide us with nice
            // readable characters.
            (*sn)[i + offset] = 'a' + (hash[i] & 0x0f);
        }
        return 20;
    }
} // namespace

atomic<bool> MarlinPrinter::ready = false;

MarlinPrinter::MarlinPrinter() {
    marlin_client::init();

    info.firmware_version = project_version_full;
    info.appendix = appendix_exist();

    otp_get_serial_nr(info.serial_number);

    if (!serial_valid(info.serial_number.begin())) {
        synthetic_serial(&info.serial_number);
    }

    printerHash(info.fingerprint, sizeof(info.fingerprint) - 1, false);
    info.fingerprint[sizeof(info.fingerprint) - 1] = '\0';

    info.appendix = appendix_exist();
}

void MarlinPrinter::renew(std::optional<SharedBuffer::Borrow> new_borrow) {
    if (new_borrow.has_value()) {
        static_assert(SharedBuffer::SIZE >= FILE_NAME_BUFFER_LEN + FILE_PATH_BUFFER_LEN);
        borrow = BorrowPaths(move(*new_borrow));
        // update variables from marlin server, sample LFN+SFN atomically
        auto lock = MarlinVarsLockGuard();
        marlin_vars()->media_SFN_path.copy_to(borrow->path(), FILE_PATH_BUFFER_LEN, lock);
        marlin_vars()->media_LFN.copy_to(borrow->name(), FILE_NAME_BUFFER_LEN, lock);
    } else {
        borrow.reset();
    }

    // Any suspicious state, like Busy or Printing will cancel the printer-ready state.
    //
    // (We kind of assume there's no chance of renew not being called between a
    // print starts and ends and that we'll see it.).
    if (get_state(ready) != DeviceState::Ready) {
        ready = false;
    }
}

void MarlinPrinter::drop_paths() {
    borrow.reset();
}

Printer::Params MarlinPrinter::params() const {
    auto current_filament = config_store().get_filament_type(marlin_vars()->active_extruder);

    Params params(borrow);
    params.material = filament::get_description(current_filament).name;
    params.state = get_state(ready);
    params.temp_bed = marlin_vars()->temp_bed;
    params.target_bed = marlin_vars()->target_bed;
    params.temp_nozzle = marlin_vars()->active_hotend().temp_nozzle;
    params.target_nozzle = marlin_vars()->active_hotend().target_nozzle;
    params.pos[X_AXIS_POS] = marlin_vars()->logical_curr_pos[X_AXIS_POS];
    params.pos[Y_AXIS_POS] = marlin_vars()->logical_curr_pos[Y_AXIS_POS];
    params.pos[Z_AXIS_POS] = marlin_vars()->logical_curr_pos[Z_AXIS_POS];
    params.print_speed = marlin_vars()->print_speed;
    params.flow_factor = marlin_vars()->active_hotend().flow_factor;
    params.job_id = marlin_vars()->job_id;
    // Version can change between MK4 and MK3.9 in runtime
    params.version = get_printer_version();

    params.print_fan_rpm = marlin_vars()->active_hotend().print_fan_rpm;
    params.heatbreak_fan_rpm = marlin_vars()->active_hotend().heatbreak_fan_rpm;
    params.print_duration = marlin_vars()->print_duration;
    params.time_to_end = marlin_vars()->time_to_end;
    params.progress_percent = marlin_vars()->sd_percent_done;
    params.filament_used = Odometer_s::instance().get_extruded_all();
    params.nozzle_diameter = config_store().get_nozzle_diameter(0);
    params.has_usb = marlin_vars()->media_inserted;

    struct statvfs fsbuf = {};
    if (params.has_usb && statvfs("/usb/", &fsbuf) == 0) {
        // Contrary to the "unix" documentation for statvfs, our FAT implementation stores:
        // * Number of free *clusters*, not blocks in bfree.
        // * Number of blocks per cluster in frsize.
        //
        // Do we dare fix it (should we), or would that potentially break
        // something somewhere else?
        //
        // Do I even interpret the documentation correctly, or is the code right?
        //
        // (Either way, this yields the correct results now).
        params.usb_space_free = static_cast<uint64_t>(fsbuf.f_frsize) * static_cast<uint64_t>(fsbuf.f_bsize) * static_cast<uint64_t>(fsbuf.f_bfree);
    }

    return params;
}

Printer::Config MarlinPrinter::load_config() {
    Config configuration = {};
    configuration.enabled = config_store().connect_enabled.get();
    // (We need it even if disabled for registration phase)
    strlcpy(configuration.host, config_store().connect_host.get().data(), sizeof(configuration.host));
    decompress_host(configuration.host, sizeof(configuration.host));
    strlcpy(configuration.token, config_store().connect_token.get().data(), sizeof(configuration.token));
    configuration.tls = config_store().connect_tls.get();
    configuration.port = config_store().connect_port.get();

    return configuration;
}

void MarlinPrinter::init_connect(char *token) {
    config_store().connect_token.set(token);
    config_store().connect_enabled.set(true);
}

bool MarlinPrinter::load_cfg_from_ini() {
    Config config;
    bool ok = ini_parse("/usb/prusa_printer_settings.ini", connect_ini_handler, &config) == 0;
    ok = ok && config.loaded;
    if (ok) {
        if (config.port == 0) {
            config.port = config.tls ? 443 : 80;
        }

        config_store().connect_host.set(config.host);
        config_store().connect_token.set(config.token);
        config_store().connect_port.set(config.port);
        config_store().connect_tls.set(config.tls);
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
    strlcpy(result.pl_password, config_store().prusalink_password.get_c_str(), sizeof(result.pl_password));
    strlcpy(result.ssid, config_store().wifi_ap_ssid.get_c_str(), sizeof(result.ssid));
    return result;
}

bool MarlinPrinter::job_control(JobControl control) {
    // Renew was presumably called before short.
    DeviceState state = get_state(false);

    switch (control) {
    case JobControl::Pause:
        if (state == DeviceState::Printing) {
            marlin_client::print_pause();
            return true;
        } else {
            return false;
        }
    case JobControl::Resume:
        if (state == DeviceState::Paused) {
            marlin_client::print_resume();
            return true;
        } else {
            return false;
        }
    case JobControl::Stop:
        if (state == DeviceState::Paused || state == DeviceState::Printing || state == DeviceState::Attention) {
            marlin_client::print_abort();
            return true;
        } else {
            return false;
        }
    }
    assert(0);
    return false;
}

bool MarlinPrinter::start_print(const char *path) {
    if (!printer_state::remote_print_ready(false)) {
        return false;
    }

    print_begin(path, marlin_server::PreviewSkipIfAble::all);
    return marlin_client::is_print_started();
}

const char *MarlinPrinter::delete_file(const char *path) {
    auto result = remove_file(path);
    if (result == DeleteResult::Busy) {
        return "File is busy";
    } else if (result == DeleteResult::ActiveTransfer) {
        return "File is being transferred";
    } else if (result == DeleteResult::GeneralError) {
        return "Error deleting file";
    } else {
        return nullptr;
    }
}

void MarlinPrinter::submit_gcode(const char *code) {
    marlin_client::gcode(code);
}

bool MarlinPrinter::set_ready(bool ready) {
    // Just wrapping the static method into the virtual one...
    return set_printer_ready(ready);
}

bool MarlinPrinter::is_printing() const {
    return marlin_client::is_printing();
}

bool MarlinPrinter::is_idle() const {
    return marlin_client::is_idle();
}

bool MarlinPrinter::is_printer_ready() {
    // The value is brought down (maybe with some delay) when we start printing
    // or something like that. Therefore it is enough to just read the flag.
    return ready;
}

bool MarlinPrinter::set_printer_ready(bool ready) {
    if (ready && !printer_state::remote_print_ready(false)) {
        return false;
    }

    MarlinPrinter::ready = ready;
    return true;
}

} // namespace connect_client
