#include "marlin_printer.hpp"
#include <module/prusa/tool_mapper.hpp>
#include "printer_common.hpp"
#include "hostname.hpp"

#include <ini.h>
#include <otp.hpp>
#include <buddy/filename_defs.hpp>
#include <odometer.hpp>
#include <netdev.h>
#include <print_utils.hpp>
#include <wui_api.h>
#include <wui.h>
#include <filament.hpp>
#include <feature/filament_sensor/filament_sensors_handler.hpp>
#include <feature/filament_sensor/filament_sensor_states.hpp>
#include <state/printer_state.hpp>
#include <transfers/transfer_file_check.hpp>
#include <common/sys.hpp>
#include <common/unique_file_ptr.hpp>

#include <option/has_cancel_object.h>
#if HAS_CANCEL_OBJECT()
    #include <feature/cancel_object/cancel_object.hpp>
#endif

#include <option/has_side_leds.h>
#if HAS_SIDE_LEDS()
    #include <leds/side_strip_handler.hpp>
#endif

#include <option/has_spool_join.h>
#if HAS_SPOOL_JOIN()
    #include <module/prusa/spool_join.hpp>
#endif

#include <option/has_esp.h>

#if XL_ENCLOSURE_SUPPORT()
    #include <xl_enclosure.hpp>
    #include <fanctl.hpp>

    #include <option/has_chamber_filtration_api.h>
static_assert(HAS_CHAMBER_FILTRATION_API());
    #include <feature/chamber_filtration/chamber_filtration.hpp>
#endif

#if PRINTER_IS_PRUSA_COREONE() || PRINTER_IS_PRUSA_COREONEL()
    #include <feature/chamber/chamber.hpp>
    #include <feature/xbuddy_extension/xbuddy_extension.hpp>
#endif
#include <client_response.hpp>

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <crc32.h>

#include <config_store/store_instance.hpp>

#include <option/has_mmu2.h>
#include <bsod/bsod.h>
#if HAS_MMU2()
    #include <Marlin/src/feature/prusa/MMU2/mmu2_mk4.h>
    #include <mmu2/mmu2_fsm.hpp>
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
        } else if (ini_string_match(section, INI_SECTION, name, "proxy_hostname")) {
            if (len <= config_store_ns::connect_proxy_size) {
                strlcpy(config->proxy_host, value, sizeof config->proxy_host);
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
        } else if (ini_string_match(section, INI_SECTION, name, "proxy_port")) {
            char *endptr;
            long tmp = strtol(value, &endptr, 10);
            if (*endptr == '\0' && tmp >= 0 && tmp <= 65535) {
                config->proxy_port = (uint16_t)tmp;
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
        } else if (ini_string_match(section, INI_SECTION, name, "custom_cert")) {
            if (strcmp(value, "1") == 0 || strcasecmp(value, "true") == 0) {
                config->custom_cert = true;
            } else if (strcmp(value, "0") == 0 || strcasecmp(value, "false") == 0) {
                config->custom_cert = false;
            } else {
                return 0;
            }
        }
        return 1;
    }

    bool copy(const char *src, const char *dst) {
        unique_file_ptr s(fopen(src, "rb"));
        if (!s) {
            return false;
        }
        unique_file_ptr d(fopen(dst, "wb"));
        if (!d) {
            return false;
        }

        while (!feof(s.get()) && !ferror(s.get()) && !ferror(d.get())) {
            constexpr size_t block = 128;
            uint8_t buffer[block];
            size_t read = fread(buffer, 1, block, s.get());
            size_t written = fwrite(buffer, 1, read, d.get());
            if (read != written) {
                return false;
            }
        }

        return !ferror(s.get()) && !ferror(d.get());
    }
} // namespace

atomic<bool> MarlinPrinter::ready = false;

MarlinPrinter::MarlinPrinter() {
    marlin_client::init();

    init_info(info);
}

void MarlinPrinter::renew(std::optional<SharedBuffer::Borrow> new_borrow) {
    if (new_borrow.has_value()) {
        static_assert(SharedBuffer::SIZE >= filename_defs::filename_buffer_size + filename_defs::path_buffer_size);
        borrow = BorrowPaths(move(*new_borrow));
        // update variables from marlin server, sample LFN+SFN atomically
        auto lock = MarlinVarsLockGuard();
        marlin_vars().media_SFN_path.copy_to(borrow->path(), filename_defs::path_buffer_size, lock);
        marlin_vars().media_LFN.copy_to(borrow->name(), filename_defs::filename_buffer_size, lock);
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
        params.mmu_version = MMU2::mmu2.GetMMUFWVersion();
#endif

        params.active_slot = VirtualToolIndex::currently_selected();

        params.slot_mask = 0;
        for (VirtualToolIndex vt : VirtualToolIndex::all().skip_all_disabled()) {
            const PhysicalToolIndex pt = vt.to_physical();
            const auto &hotend = marlin_vars().hotend(pt);
            params.slot_mask |= (1 << vt.to_raw());

            auto &slot = params.slots[vt];
            slot.material = config_store().get_filament_type(vt).parameters().name;
            slot.temp_nozzle = hotend.temp_nozzle;
#if PRINTER_IS_PRUSA_iX()
            slot.temp_heatbreak = hotend.temp_heatbreak;

            if (IFSensor *sensor = FSensors_instance().sensor(LogicalFilamentSensor::extruder)) {
                slot.extruder_fs_state = sensor->get_state();
            } else {
                slot.extruder_fs_state.reset();
            }
            if (IFSensor *sensor = FSensors_instance().sensor(LogicalFilamentSensor::side)) {
                slot.remote_fs_state = sensor->get_state();
            } else {
                slot.remote_fs_state.reset();
            }
#endif
            slot.print_fan_rpm = hotend.print_fan_rpm;
            slot.heatbreak_fan_rpm = hotend.heatbreak_fan_rpm;
            slot.nozzle_diameter = config_store().get_nozzle_diameter(pt);
            slot.hardened = config_store().get_nozzle_is_hardened(pt.to_raw());
            slot.high_flow = config_store().get_nozzle_is_high_flow(pt.to_raw());
        }
    }
} // namespace

Printer::Params MarlinPrinter::params() const {

    Params params(borrow);
    params.state = get_state_with_dialog(ready);
    params.has_job = has_job();
    params.temp_bed = marlin_vars().temp_bed;
#if PRINTER_IS_PRUSA_iX()
    params.temp_psu = thermalManager.deg_psu();
    params.temp_ambient = thermalManager.deg_ambient();
#endif
    params.target_bed = marlin_vars().target_bed;
    params.target_nozzle = marlin_vars().active_hotend().target_nozzle;
    params.pos[X_AXIS_POS] = marlin_vars().logical_pos[X_AXIS_POS];
    params.pos[Y_AXIS_POS] = marlin_vars().logical_pos[Y_AXIS_POS];
    params.pos[Z_AXIS_POS] = marlin_vars().logical_pos[Z_AXIS_POS];
    params.print_speed = marlin_vars().print_speed;

    if (auto virtual_tool = stdext::get_optional<VirtualToolIndex>(VirtualToolIndex::currently_selected())) {
        params.flow_factor = marlin_vars().virtual_tools[*virtual_tool].flow_factor;
    }

    params.job_id = marlin_vars().job_id;
    params.version = PrinterModelInfo::current().version;
    get_slot_info(params);

#if XL_ENCLOSURE_SUPPORT()
    params.enclosure_info = {
        .present = xl_enclosure.isActive(),
        .enabled = xl_enclosure.isEnabled(),
        .printing_filtration = config_store().chamber_print_filtration_enable.get(),
        .post_print = config_store().chamber_post_print_filtration_enable.get(),
        // it is stored is minutes, but we want seconds, so that it is consistent with the rest
        .post_print_filtration_time = static_cast<uint16_t>(config_store().chamber_post_print_filtration_duration_min.get() * 60),
        .temp = static_cast<int>(xl_enclosure.getEnclosureTemperature().value_or(0)),
        .fan_rpm = Fans::enclosure().get_actual_rpm(),
        .time_in_use = config_store().chamber_filter_time_used_s.get()
    };
#endif
#if PRINTER_IS_PRUSA_COREONE() || PRINTER_IS_PRUSA_COREONEL()
    {
        auto xbe = buddy::xbuddy_extension().get_fan12_state(); // avoid locking 2 mutexes just to read a single value (and we are reading 4 values)
        params.chamber_info = {
            .target_temp = (uint32_t)buddy::chamber().target_temperature().value_or(connect_client::Printer::ChamberInfo::target_temp_unset),
            .current_temp = buddy::chamber().current_temperature().value_or(0), /* Missing that would be rare, so we just always render something for simplicity */
            .fan_1_rpm = xbe.fan1rpm,
            .fan_2_rpm = xbe.fan2rpm,
            .fan_pwm_target = xbe.fan1_fan2_target_pwm.transform(buddy::XBuddyExtension::FanPWM::to_percent_static).value_or(connect_client::Printer::ChamberInfo::fan_pwm_target_unset),
            .led_intensity = static_cast<int8_t>(static_cast<uint16_t>(leds::SideStripHandler::instance().get_max_brightness()) * 100 / 255),
        };
        params.addon_power = buddy::xbuddy_extension().usb_power();
    }
#endif
    params.print_duration = marlin_vars().print_duration;
    params.time_to_end = marlin_vars().time_to_end;
    params.time_to_pause = marlin_vars().time_to_pause;
    params.progress_percent = marlin_vars().sd_percent_done;
    params.filament_used = Odometer_s::instance().get_extruded_all();
    params.has_usb = marlin_vars().media_inserted;
    params.can_start_download = can_start_download;

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
#if HAS_CANCEL_OBJECT()
    const auto &parameters = params();
    crc = crc32_calc_ex(crc, reinterpret_cast<const uint8_t *>(&parameters.job_id), sizeof(parameters.job_id));

    const auto revision = buddy::cancel_object().objects_revision();
    crc = crc32_calc_ex(crc, reinterpret_cast<const uint8_t *>(&revision), sizeof(revision));
#endif
    return crc;
}

#if HAS_CANCEL_OBJECT()
void MarlinPrinter::set_object_cancelled(uint16_t id, bool set) {
    marlin_client::set_object_cancelled(id, set);
}
#endif

void MarlinPrinter::init_connect(const char *token) {
    auto &store = config_store();
    auto transaction = store.get_backend().transaction_guard();
    store.connect_token.set(token);
    store.connect_enabled.set(true);
}

bool MarlinPrinter::load_cfg_from_ini() {
    Config config;
    bool ok = ini_parse("/usb/prusa_printer_settings.ini", connect_ini_handler, &config) == 0;
    ok = ok && config.loaded;

    if (ok && config.custom_cert) {
        // OK to fail (for example if the dir already exists). In any case, we
        // care about the below successes.
        mkdir("/internal/connect", 0777);
        if (!copy("/usb/connect.der", "/internal/connect/connect.der")) {
            ok = false;
        }
    }

    if (ok) {
        if (config.port == 0) {
            config.port = config.tls ? 443 : 80;
        }

        auto &store = config_store();
        auto transaction = store.get_backend().transaction_guard();
        store.connect_host.set(config.host);
        store.connect_token.set(config.token);
        store.connect_port.set(config.port);
        store.connect_tls.set(config.tls);
        store.connect_custom_tls_cert.set(config.custom_cert);
        store.connect_proxy_host.set(config.proxy_host);
        store.connect_proxy_port.set(config.proxy_port);
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
#if HAS_ESP()
    case Iface::Wifi:
        id = NETDEV_ESP_ID;
        break;
#endif
    default:
        debug_assert(0);
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
#if HAS_ESP()
    strlcpy(result.ssid, config_store().wifi_ap_ssid.get_c_str(), sizeof(result.ssid));
#endif
    netdev_get_hostname(netdev_get_active_id(), result.hostname, sizeof(result.hostname));
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
    debug_assert(0);
    return false;
}

#if HAS_TOOL_MAPPING()
namespace {
    const char *handle_tool_mapping(const ToolMapping &tool_mapping) {
    #if HAS_MMU2()
        if (!config_store().mmu2_enabled.get()) {
            return "MMU not enabled, can't use tools mapping";
        }
    #endif

        auto cleanup = []() {
            tool_mapper.reset();
    #if HAS_SPOOL_JOIN()
            spool_join.reset();
    #endif
            tool_mapper.set_enable(false);
        };
        // Wipe defaults (eg mapping 1-1, 2-2, ...) - we want to replace it,
        // not merge and create some kind of weird hydra-mapping.
        tool_mapper.set_all_unassigned();
        tool_mapper.set_enable(true);
        for (size_t i = 0; i < tool_mapping.size(); i++) {
            auto &curr_tool = tool_mapping[i][0];
            if (curr_tool == ToolMapper::NO_TOOL_MAPPED) {
                continue;
            }

            if (!tool_mapper.set_mapping(i, curr_tool)) {
                cleanup();
                return "Invalid tools mapping";
            }
            for (size_t j = 1; j < tool_mapping[i].size(); j++) {
                if (tool_mapping[i][j] == ToolMapper::NO_TOOL_MAPPED) {
                    break;
                }

    #if HAS_SPOOL_JOIN()
                if (!spool_join.add_join(curr_tool, tool_mapping[i][j])) {
                    cleanup();
                    return "Invalid spool join setting";
                }
    #endif
            }
        }
        return nullptr;
    }
} // namespace
#endif

bool MarlinPrinter::is_valid_file_or_transfer(const char *path) const {
    return transfers::is_valid_file_or_transfer(path);
}

Printer::StartPrintResult MarlinPrinter::start_print(const char *path, [[maybe_unused]] const std::optional<ToolMapping> &tools_mapping) {
    if (!printer_state::remote_print_ready(false)) {
        return std::unexpected("Can't print now");
    }

    if (tools_mapping.has_value()) {
#if HAS_TOOL_MAPPING()
        if (const char *error = handle_tool_mapping(tools_mapping.value()); error != nullptr) {
            return std::unexpected(error);
        }
#else
        return std::unexpected("Tools mapping not enabled");
#endif
    }

    marlin_client::print_start(path, marlin_server::PreviewSkipIfAble::all);
    if (!marlin_client::is_print_started()) {
        return std::unexpected("Can't print now");
    }
    // job_id already updated internally in MarkStarted phase
    return static_cast<uint16_t>(marlin_vars().job_id);
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

bool MarlinPrinter::set_idle() {
    const auto state = printer_state::get_state(false);
    if (state == printer_state::DeviceState::Finished || state == printer_state::DeviceState::Stopped) {
        marlin_client::print_exit();
        return true;
    }
    return false;
}

bool MarlinPrinter::is_printing() const {
    return marlin_client::is_printing();
}

bool MarlinPrinter::is_in_error() const {
    // This is true in error screens. These don't even
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
    sys_reset();
}

const char *MarlinPrinter::dialog_action(printer_state::DialogId dialog_id, Response response) {
    fsm::StateId fsm_gen;
    std::optional<fsm::States::Top> top;
    marlin_vars().peek_fsm_states([&](const auto &states) {
        fsm_gen = states.get_state_id();
        top = states.get_top();
    });

    // We always send dialog from the top FSM, so we can
    // just check the dialog_id and if it is the same
    // we know it is for the top one
    if (!top) {
        return "No buttons";
    }

    if (fsm_gen != dialog_id) {
        return "Invalid dialog id";
    }

    const PhaseResponses &valid_responses = ClientResponses::get_fsm_responses(top->fsm_type, top->data.GetPhase());
    if (std::find(valid_responses.begin(), valid_responses.end(), response) == valid_responses.end()) {
        return "Invalid button for dialog";
    }

    marlin_client::FSM_encoded_response(EncodedFSMResponse {
        .response = FSMResponseVariant::make(response),
        .fsm_and_phase = FSMAndPhase(top->fsm_type, top->data.GetPhase()),
    });
    return nullptr;
}

std::optional<MarlinPrinter::FinishedJobResult> MarlinPrinter::get_prior_job_result(uint16_t job_id) const {
    auto result = marlin_vars().get_job_result(job_id);
    if (!result.has_value()) {
        return nullopt;
    }

    switch (result.value()) {
    case marlin_vars_t::JobInfo::JobResult::aborted:
        return FinishedJobResult::FIN_STOPPED;
    case marlin_vars_t::JobInfo::JobResult::finished:
        return FinishedJobResult::FIN_OK;
    }

    return nullopt;
}

void MarlinPrinter::set_slot_info(VirtualToolIndex vt, const SlotInfo &info) {
    const auto pt = vt.to_physical();
    config_store().set_nozzle_diameter(pt, info.nozzle_diameter);
    config_store().set_nozzle_is_hardened(pt, info.hardened);
    config_store().set_nozzle_is_high_flow(pt, info.high_flow);
}

} // namespace connect_client
