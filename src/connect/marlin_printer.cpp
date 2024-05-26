#include "marlin_printer.hpp"
#include "printer_common.hpp"
#include "hostname.hpp"

#include <ini.h>
#include <otp.hpp>
#include <odometer.hpp>
#include <netdev.h>
#include <print_utils.hpp>
#include <wui_api.h>
#include <filament.hpp>
#include <filament_sensors_handler.hpp>
#include <filament_sensor_states.hpp>
#include <state/printer_state.hpp>

#if XL_ENCLOSURE_SUPPORT()
    #include <xl_enclosure.hpp>
    #include <fanctl.hpp>
#endif

#include <client_response.hpp>

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <sys/statvfs.h>
#include <crc32.h>

#include <config_store/store_instance.hpp>
#include <option/has_mmu2.h>

#if HAS_MMU2()
    #include <Marlin/src/feature/prusa/MMU2/mmu2_mk4.h>
    #include <mmu2_fsm.hpp>
#endif

using marlin_client::GcodeTryResult;
using printer_state::DeviceState;
using printer_state::get_state;
using printer_state::get_state_with_dialog;
using printer_state::has_job;
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
} // namespace

atomic<bool> MarlinPrinter::ready = false;

MarlinPrinter::MarlinPrinter() {
    marlin_client::init();

    init_info(info);
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

namespace {
    void get_slot_info(Printer::Params &params) {
#if HAS_MMU2()
        params.progress_code = MMU2::Fsm::Instance().reporter.GetProgressCode();
        params.command_code = MMU2::Fsm::Instance().reporter.GetCommandInProgress();
        const bool mmu_enabled = config_store().mmu2_enabled.get() && marlin_vars()->mmu2_state == ftrstd::to_underlying(MMU2::xState::Active);
        params.slot_mask = mmu_enabled ? 0b00011111 : 1;
        params.mmu_version = MMU2::mmu2.GetMMUFWVersion();
        // Note: 0 means no active tool, indexing from 1
        params.active_slot = MMU2::mmu2.get_current_tool() == MMU2::FILAMENT_UNKNOWN ? 0 : MMU2::mmu2.get_current_tool() + 1;
#elif HAS_TOOLCHANGER()
        params.active_slot = prusa_toolchanger.is_any_tool_active() ? prusa_toolchanger.get_active_tool_nr() + 1 : 0;
        // For XL, it is possible to have "holes" in the enabled tools.
        // Therefore, we need to enable all slots and skip particular ones, not
        // do 1..n.
        params.slot_mask = prusa_toolchanger.get_enabled_mask();
#else
        // Anything else - all slots (usually 1) are enabled.
        // 1 << NUMBER_OF_SLOTS -> 1000 (as many zeroes as NUMBER_OF_SLOTS)
        // 1000 - 1 -> 0111 (turn the trailing zeroes to ones)
        params.slot_mask = (1 << Printer::NUMBER_OF_SLOTS) - 1;
#endif
        for (size_t i = 0; i < params.slots.size(); i++) {
            if (params.slot_mask & (1 << i)) {
#if HAS_TOOLCHANGER()
                auto &hotend = marlin_vars()->hotend(i);
#else
                // only one hotend in any other situation
                auto &hotend = marlin_vars()->active_hotend();
#endif
                params.slots[i].material = filament::get_name(config_store().get_filament_type(i));
                params.slots[i].temp_nozzle = hotend.temp_nozzle;
                params.slots[i].print_fan_rpm = hotend.print_fan_rpm;
                params.slots[i].heatbreak_fan_rpm = hotend.heatbreak_fan_rpm;
            }
        }
    }
} // namespace

Printer::Params MarlinPrinter::params() const {

    Params params(borrow);
    params.state = get_state_with_dialog(ready);
    params.has_job = has_job();
    params.temp_bed = marlin_vars()->temp_bed;
    params.target_bed = marlin_vars()->target_bed;
    params.target_nozzle = marlin_vars()->active_hotend().target_nozzle;
    params.pos[X_AXIS_POS] = marlin_vars()->logical_pos[X_AXIS_POS];
    params.pos[Y_AXIS_POS] = marlin_vars()->logical_pos[Y_AXIS_POS];
    params.pos[Z_AXIS_POS] = marlin_vars()->logical_pos[Z_AXIS_POS];
    params.print_speed = marlin_vars()->print_speed;
    params.flow_factor = marlin_vars()->active_hotend().flow_factor;
    params.job_id = marlin_vars()->job_id;
    // Version can change between MK4 and MK3.9 in runtime
    params.version = get_printer_version();
    get_slot_info(params);
#if ENABLED(CANCEL_OBJECTS)
    params.cancel_object_count = marlin_vars()->cancel_object_count;
    params.cancel_object_mask = marlin_vars()->get_cancel_object_mask();
#endif
#if XL_ENCLOSURE_SUPPORT()
    params.enclosure_info = {
        .present = xl_enclosure.isActive(),
        .enabled = xl_enclosure.isEnabled(),
        .printing_filtration = xl_enclosure.isPrintFiltrationEnabled(),
        .post_print = xl_enclosure.isPostPrintFiltrationEnabled(),
        // it is stored is minutes, but we want seconds, so that it is consistent with the rest
        .post_print_filtration_time = static_cast<uint16_t>(config_store().xl_enclosure_post_print_duration.get() * 60),
        .temp = xl_enclosure.isTemperatureValid() ? xl_enclosure.getEnclosureTemperature() : 0,
        .fan_rpm = Fans::enclosure().getActualRPM(),
        .time_in_use = std::min(config_store().xl_enclosure_filter_timer.get(), Enclosure::expiration_deadline_sec)
    };
#endif
    params.print_duration = marlin_vars()->print_duration;
    params.time_to_end = marlin_vars()->time_to_end;
    params.time_to_pause = marlin_vars()->time_to_pause;
    params.progress_percent = marlin_vars()->sd_percent_done;
    params.filament_used = Odometer_s::instance().get_extruded_all();
    params.nozzle_diameter = config_store().get_nozzle_diameter(params.preferred_slot());
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
    return load_eeprom_config();
}

uint32_t MarlinPrinter::cancelable_fingerprint() const {
    uint32_t crc = 0;
#if ENABLED(CANCEL_OBJECTS)
    const auto &parameters = params();
    auto calc_crc = [&](const char *s) {
        crc = crc32_calc_ex(crc, reinterpret_cast<const uint8_t *>(*s), strlen(s));
    };
    for (size_t i = 0; i < marlin_vars_t::CANCEL_OBJECTS_NAME_COUNT; i++) {
        marlin_vars()->cancel_object_names[i].execute_with(calc_crc);
    }
    crc = crc32_calc_ex(crc, reinterpret_cast<const uint8_t *>(&parameters.job_id), sizeof(parameters.job_id));
    crc = crc32_calc_ex(crc, reinterpret_cast<const uint8_t *>(&parameters.cancel_object_count), sizeof(parameters.cancel_object_count));
    crc = crc32_calc_ex(crc, reinterpret_cast<const uint8_t *>(&parameters.cancel_object_mask), sizeof(parameters.cancel_object_mask));
#endif
    return crc;
}

#if ENABLED(CANCEL_OBJECTS)
const char *MarlinPrinter::get_cancel_object_name(char *buffer, size_t size, size_t index) const {
    marlin_vars()->cancel_object_names[index].copy_to(buffer, size);
    return buffer;
}
#endif

void MarlinPrinter::init_connect(const char *token) {
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

Printer::GcodeResult MarlinPrinter::submit_gcode(const char *code) {
    switch (marlin_client::gcode_try(code)) {
    case GcodeTryResult::Submitted:
        return GcodeResult::Submitted;
    case GcodeTryResult::QueueFull:
        return GcodeResult::Later;
    case GcodeTryResult::GcodeTooLong:
        return GcodeResult::Failed;
    }

    bsod("Invalid gcode_try result");
}

bool MarlinPrinter::set_ready(bool ready) {
    // Just wrapping the static method into the virtual one...
    return set_printer_ready(ready);
}

bool MarlinPrinter::is_printing() const {
    return marlin_client::is_printing();
}

bool MarlinPrinter::is_in_error() const {
    // This is true in redscreens, bluescreens and similar. These don't even
    // initialize a MarlinPrinter but ErrorPrinter.
    return false;
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

void MarlinPrinter::reset_printer() {
    NVIC_SystemReset();
}

namespace {
    bool validate_response(const PhaseResponses &responses, Response to_check) {
        for (auto resp : responses) {
            if (to_check == resp) {
                return true;
            }
        }

        return false;
    }
    template <class Phase>
    const char *send_click(uint8_t phase, Response response) {
        auto phase_enum = GetEnumFromPhaseIndex<Phase>(phase);
        if (!validate_response(ClientResponses::GetResponses(phase_enum), response)) {
            return "Invalid button for dialog";
        }
        marlin_client::FSM_response(phase_enum, response);
        return nullptr;
    }
} // namespace

const char *MarlinPrinter::dialog_action(uint32_t dialog_id, Response response) {
    const fsm::States fsm_states = marlin_vars()->get_fsm_states();
    const std::optional<fsm::States::Top> top = fsm_states.get_top();

    // We always send dialog from the top FSM, so we can
    // just check the dialog_id and if it is the same
    // we know it is for the top one
    if (!top) {
        return "No buttons";
    }

    if (fsm_states.generation != dialog_id) {
        return "Invalid dialog id";
    }

    const uint8_t phase = top->data.GetPhase();
    switch (top->fsm_type) {
    case ClientFSM::Load_unload:
        return send_click<PhasesLoadUnload>(phase, response);
    case ClientFSM::Preheat:
        return send_click<PhasesPreheat>(phase, response);
    case ClientFSM::Selftest:
        return send_click<PhasesSelftest>(phase, response);
    case ClientFSM::ESP:
        return send_click<PhasesESP>(phase, response);
    case ClientFSM::CrashRecovery:
        return send_click<PhasesCrashRecovery>(phase, response);
    case ClientFSM::QuickPause:
        return send_click<PhasesQuickPause>(phase, response);
    case ClientFSM::Warning:
        return send_click<PhasesWarning>(phase, response);
    case ClientFSM::ColdPull:
        return send_click<PhasesColdPull>(phase, response);
    case ClientFSM::PrintPreview:
        return send_click<PhasesPrintPreview>(phase, response);
        // NOTE: These have no Phases and no buttons
#if HAS_PHASE_STEPPING()
    case ClientFSM::PhaseStepping:
        return send_click<PhasesPhaseStepping>(phase, response);
#endif
    case ClientFSM::Printing:
    case ClientFSM::Serial_printing:
    case ClientFSM::_none:
        return "No buttons";
    }
    return nullptr;
}

} // namespace connect_client
