#include "marlin_server.hpp"

#include <option/has_crash_detection.h>
#include <option/has_pause.h>
#include <common/directory.hpp>
#include <buddy/filename_defs.hpp>
#include <freertos/critical_section.hpp>
#include <marlin_stubs/skippable_gcode.hpp>
#include <mapi/parking.hpp>
#include <mapi/feedrates/standard_feedrates.hpp>
#include "marlin_client_queue.hpp"
#include "marlin_server_request.hpp"
#include <inttypes.h>
#include <stdarg.h>
#include <cstdint>
#include <stdio.h>
#include <string.h> //strncmp
#include <charconv>

#include "adc.hpp"
#include "marlin_events.h"
#include "marlin_print_preview.hpp"
#include "utils/exponential_backoff.hpp"
#include <bsod.h>
#include "module/prusa/tool_mapper.hpp"
#include "print_utils.hpp"
#include <random/random.h>
#include "timing.h"
#include "cmsis_os.h"
#include <logging/log.hpp>
#include <bsod_gui.hpp>
#include <usb_host.h>
#include <usb_host.h>
#include <lfn.h>
#include <media_prefetch/media_prefetch.hpp>
#include <gcode/gcode_reader_restore_info.hpp>
#include <dirent.h>
#include <raii/scope_guard.hpp>
#include <tools_mapping.hpp>
#include <raii/auto_restore.hpp>
#include <inject_queue.hpp>
#include <utils/string_builder.hpp>
#include <utils/mutex_atomic.hpp>
#include <feature/safety_timer/safety_timer.hpp>
#include <feature/stepper_timeout/stepper_timeout.hpp>
#include <mapi/motion.hpp>
#include <feature/filament_sensor/filament_sensors_handler.hpp>

#include "../Marlin/src/lcd/extensible_ui/ui_api.h"
#include "../Marlin/src/gcode/queue.h"
#include "../Marlin/src/gcode/parser.h"
#include "../Marlin/src/module/planner.h"
#include "../Marlin/src/module/stepper.h"
#include "../Marlin/src/module/endstops.h"
#include "../Marlin/src/module/temperature.h"
#include "../Marlin/src/module/probe.h"
#include "../Marlin/src/module/configuration_store.h"
#include "../Marlin/src/module/printcounter.h"
#include "../Marlin/src/feature/babystep.h"
#include "../Marlin/src/feature/bedlevel/bedlevel.h"
#include "../Marlin/src/feature/input_shaper/input_shaper.hpp"
#include "../Marlin/src/feature/pause.h"
#include "../Marlin/src/feature/prusa/measure_axis.h"
#include "../Marlin/src/core/language.h" //GET_TEXT(MSG)
#include "../Marlin/src/gcode/gcode.h"
#include "../Marlin/src/gcode/lcd/M73_PE.h"
#include "../Marlin/src/feature/print_area.h"
#include "../Marlin/src/Marlin.h"
#include "utility_extensions.hpp"
#include "utils/variant_utils.hpp"
#include <common/gcode/gcode_info_scan.hpp>

#include <option/has_mmu2.h>
#if HAS_MMU2()
    #include "../Marlin/src/feature/prusa/MMU2/mmu2_mk4.h"
#endif

#include <option/has_cancel_object.h>
#if HAS_CANCEL_OBJECT()
    #include <feature/cancel_object/cancel_object.hpp>
#endif

#include <option/has_power_panic.h>
#include <option/has_spool_join.h>
#if HAS_SPOOL_JOIN()
    #include "module/prusa/spool_join.hpp"
#endif

#include "hwio.h"
#include "wdt.hpp"
#include "../marlin_stubs/M123.hpp"
#include "fsm_states.hpp"
#include "odometer.hpp"
#include "metric.h"
#include "app_metrics.h"
#include "media_prefetch_instance.hpp"
#include <common/sensor_data.hpp>
#include <utils/algorithm_extensions.hpp>

#include <option/has_leds.h>

#include "fanctl.hpp"
#include <common/printer_model.hpp>
#include <common/extended_printer_type.hpp>
#include "lcd/extensible_ui/ui_api.h"

#include <option/has_gui.h>
#include <option/has_toolchanger.h>
#include <option/has_tool_crash_recovery.h>
#include <option/has_tool_mapping.h>
#include <option/has_selftest.h>
#include <option/has_dwarf.h>
#include <option/has_remote_bed.h>
#include <option/has_xl_can.h>
#include <option/has_modular_bed.h>
#include <option/has_loadcell.h>
#include <option/has_nfc.h>
#include <option/has_ht_hotend.h>
#include <option/has_sheet_profiles.h>
#include <option/has_i2c_expander.h>
#include <option/has_chamber_api.h>
#include <option/xbuddy_extension_variant.h>
#include <option/has_emergency_stop.h>
#include <option/has_ht_hotend.h>
#include <option/has_uneven_bed_prompt.h>
#include <option/has_nextruder.h>
#include <option/has_human_interactions.h>
#include <option/has_chamber_vents.h>
#include <option/has_motor_current_profiles.h>
#if HAS_MOTOR_CURRENT_PROFILES()
    #include <feature/motor_current_profile/motor_current_profile.hpp>
#endif

#include <option/has_indx.h>
#include <option/has_wastebin_fill_tracking.h>
#if HAS_WASTEBIN_FILL_TRACKING()
    #include <feature/wastebin_watcher/wastebin_watcher.hpp>
#endif
#if HAS_INDX()
    #include <tool/hotend/hotend/indx_hotend.hpp>
#endif

#if HAS_DWARF()
    #include <puppies/Dwarf.hpp>
#endif /*HAS_DWARF()*/

#if HAS_REMOTE_BED()
    #include <feature/remote_bed/remote_bed.hpp>
#endif

#if HAS_XL_CAN()
    #include <puppies/xl_can.hpp>
    #include <hw/xl/modular_bed_fan.hpp>
#endif

#if HAS_SELFTEST()
    #include "printer_selftest.hpp"
    #include "i_selftest.hpp"
    #include "selftest_axis.h"
#endif

#if HAS_SHEET_PROFILES()
    #include "SteelSheets.hpp"
#endif

// Provides crash_s stubs when crash detection is not supported
#include "../Marlin/src/feature/prusa/crash_recovery.hpp"

#if HAS_CRASH_DETECTION()
    #include "crash_recovery_type.hpp"
#endif

#if HAS_POWER_PANIC()
    #include "power_panic.hpp"
    #include "power_panic_storage.hpp"
#endif

#if HAS_TOOLCHANGER()
    #include "module/prusa/toolchanger.h"
    #include "module/tool_change.h"
#endif

#if HAS_MMU2()
    #include <mmu2/mmu2_fsm.hpp>
    #include <mmu2/maintenance.hpp>
#endif

#include <config_store/store_instance.hpp>

#if XL_ENCLOSURE_SUPPORT()
    #include "xl_enclosure.hpp"
#endif

#if HAS_NFC()
    #include <nfc.hpp>
    #include <fsm_network_setup.hpp>
#endif

#if HAS_CHAMBER_API()
    #include <feature/chamber/chamber.hpp>
#endif
#if HAS_E2EE_SUPPORT()
    #include <e2ee/key.hpp>
#endif

#include <option/has_chamber_filtration_api.h>
#if HAS_CHAMBER_FILTRATION_API()
    #include <feature/chamber_filtration/chamber_filtration.hpp>
#endif

#if XBUDDY_EXTENSION_VARIANT_IS_STANDARD()
    #include <feature/xbuddy_extension/xbuddy_extension.hpp>
#endif
#if HAS_EMERGENCY_STOP()
    #include <feature/emergency_stop/emergency_stop.hpp>
#endif
#if HAS_HT_HOTEND()
    #include <feature/hotend_burn_risk.hpp>
#endif

#include <option/has_ceiling_clearance.h>
#if HAS_CEILING_CLEARANCE()
    #include <feature/ceiling_clearance/ceiling_clearance.hpp>
#endif

#include <option/has_auto_retract.h>
#if HAS_AUTO_RETRACT()
    #include <feature/auto_retract/auto_retract.hpp>
#endif

#include <option/has_filament_tracker.h>
#if HAS_FILAMENT_TRACKER()
    #include <feature/filament_tracker/filament_tracker.hpp>
#endif

#include <option/buddy_enable_wui.h>
#if BUDDY_ENABLE_WUI()
    #include <wui.h>
#endif

#include <feature/print_status_message/print_status_message_mgr.hpp>

#if HAS_NOZZLE_CLEANER()
    #include <nozzle_cleaner.hpp>
#endif

#include "option/has_bed_fan.h"
#if HAS_BED_FAN()
    #include <feature/bed_fan/bed_fan.hpp>
    #include <feature/bed_fan/controller.hpp>
#endif

#include <option/has_psu_fan.h>
#if HAS_PSU_FAN()
    #include <feature/psu_fan/psu_fan.hpp>
#endif

#include <option/has_cpu_fan.h>
#if HAS_CPU_FAN()
    #include <cpu_fan_controller.hpp>
#endif

#include <option/has_anfc.h>
#if HAS_ANFC()
    #include <feature/openprinttag/filament_usage_tracker/filament_usage_tracker.hpp>
#endif

#if HAS_INDX()
    #include <fsm/nozzle_mismatch_phases.hpp>
#endif

void record_fanctl_metrics();

using namespace ExtUI;

using ClientQueue = marlin_client::ClientQueue;

extern osThreadId defaultTaskHandle;

LOG_COMPONENT_DEF(MarlinServer, logging::Severity::info);

//-----------------------------------------------------------------------------
// external variables from marlin_client

namespace marlin_client {
extern osThreadId marlin_client_task[MARLIN_MAX_CLIENTS]; // task handles
extern ClientQueue marlin_client_queue[MARLIN_MAX_CLIENTS];
} // namespace marlin_client

namespace marlin_server {

Publisher<> idle_publisher;

void media_prefetch_lazy_start();
void media_prefetch_start();

/// Queue for requests with parameters
RequestQueue request_queue;

/// Bitset for requests that don't need parameters
std::atomic<uint32_t> request_flags = 0;
static_assert(std::to_underlying(RequestFlag::_cnt) <= 32, "There are more flags than bits");

/// FSM response to be processed by the server
static MutexAtomic<EncodedFSMResponse, freertos::Mutex> fsm_response = empty_encoded_fsm_response;

namespace {

    struct server_t {
        EventMask notify_events[MARLIN_MAX_CLIENTS]; // event notification mask - message filter
        EventMask notify_changes[MARLIN_MAX_CLIENTS]; // variable change notification mask - message filter
        EventMask client_events[MARLIN_MAX_CLIENTS]; // client event mask - unsent messages
        State print_state; // printing state (printing, paused, ...)
        bool print_is_serial = false; //< When true, current print is not from USB, but sent via gcode commands.
#if HAS_CRASH_DETECTION() //
        bool aborting_did_crash_trigger = false; // To remember crash_s state when aborting
#endif
        resume_state_t resume; // resume data (state before pausing)
        uint32_t last_update; // last update tick count
        uint16_t flags; // server flags (MARLIN_SFLG)
        int32_t knob_position = 0; /// Increased with each knob move up, decreased with each knob move down
#if ENABLED(AXIS_MEASURE)
        /// length of axes measured after crash
        /// negative numbers represent undefined length
        xy_float_t axis_length = { -1, -1 };
#endif // ENABLED(AXIS_MEASURE)

        bool was_print_time_saved = false;
#if HAS_MMU2()
        bool mmu_maintenance_checked = false;
#endif
    };

    server_t server; // server structure - initialize task to zero

    /// State variables that reset with each print
    struct PrintState {

        // In case we were paused due to media error, we schedule an attempt to recover
        // (using the recover_media_error_backoff mechanism).
        //
        // We still allow an earlier attempt if called externally by try_recover_from_media_error.
        std::optional<uint32_t> recover_media_error_at;

        // Tracking exponential backoff for media error recovery retries.
        //
        // In seconds.
        buddy::ExponentialBackoff<uint32_t, 30, 300> recover_media_error_backoff;

        /// Position the media should be resumed to
        GCodeReaderStreamRestoreInfo media_restore_info;

#if HAS_CRASH_DETECTION()
        /// Command to be executed in interrupt mode - see marlin_client::gcode_interrupt
        GCodeLiteral gcode_interrupt_command;
#endif

        /// When print_resume is called during the pausing (or possibly other sequences), we first have to finish the sequence and then start resuming.
        /// This flag stores that we have a resume pending and we should start executing it when we can.
        bool resume_pending : 1 = false;

        /// Denotes whether a single gcode should be skipped
        /// Some pauses should cause (partial) gcode replay on resume - crash, power panic, ..., some shouldn't.
        /// This does that
        bool skip_gcode : 1 = false;

        /// Whether file open was reported on the serial line.
        /// We cannot do this directly when calling media_prefecth start, we need to wait till we have file size estimate
        bool file_open_reported : 1 = false;
    };

    PrintState print_state;

    enum class Pause_Type {
        Pause,
        Crash
    };

    /**
     * @brief Pauses reading from a file, stops watch, saves temperatures, disables fan.
     * Does not change server.print_state. You need to set that manually.
     * @param type pause type used for different media_print pause
     * @param resume_pos position to resume from, used only in Pause_Type::Crash
     */
    void process_pausing_begin_state(Pause_Type type = Pause_Type::Pause);

    fsm::States fsm_states;

    class ErrorChecker {
    public:
        constexpr ErrorChecker() = default;

        constexpr bool isFailed() const { return m_failed; }

        void checkTrue(bool condition, WarningType warning, bool disable_hotend, bool pause_print_on_error) {
            if (condition || m_failed) {
                return;
            }
            set_warning(warning);

            if (pause_print_on_error && server.print_state == State::Printing) {
                // HACK - changing printer state possibly in the middle of a gcode
                // This is done because we need to store current hotend temperatures before disabling hotend
                process_pausing_begin_state();
                server.print_state = State::Pausing_WaitIdle;
            }

            if (disable_hotend) {
                thermalManager.disable_hotend();
            }
            m_failed = true;
        };

        constexpr void reset() { m_failed = false; }

    protected:
        bool m_failed = false;
    };

    class HotendErrorChecker : private ErrorChecker {
    public:
        constexpr HotendErrorChecker() = default;

        void checkTrue(bool condition) {
            if (!condition && !m_failed) {
                if (server.print_state == State::Printing) {
                    m_postponeFullPrintFan = true;
                } else {
                    thermalManager.set_print_fan_speed(255);
                }
            }

            ErrorChecker::checkTrue(condition, WarningType::HotendTempDiscrepancy, true, true);

            if (condition) {
                reset();
            }
        }
        bool runFullFan() {
            const bool retVal = m_postponeFullPrintFan;
            m_postponeFullPrintFan = false;
            return retVal;
        }

        using ErrorChecker::isFailed;

    private:
        bool m_postponeFullPrintFan = false;
    };

    /// Check MCU temperature and trigger warning and redscreen
    class MCUTempErrorChecker : public ErrorChecker {
        static constexpr const int32_t mcu_temp_warning = 85; ///< When to show warning and pause the print
        static constexpr const int32_t mcu_temp_hysteresis = 2; ///< Hysteresis to reset warning
        static constexpr const int32_t mcu_temp_redscreen = 95; ///< When to show redscreen error

        int32_t ewma_buffer = 0; ///< Buffer for EWMA [1/8 degrees Celsius]
        bool warning = false; ///< True during warning state, enables hysteresis

    public:
        constexpr MCUTempErrorChecker() {};

        /**
         * @brief Check one MCU temperature.
         * @param temperature MCU temperature [degrees Celsius]
         */
        void check(int32_t temperature, WarningType warning_type, const char *error_arg) {
            ewma_buffer = (ewma_buffer * 7 / 8) + temperature; // Simple EWMA filter (stays 1 degree below stable value)
            const auto filtered_temperature = ewma_buffer / 8;

            // Trigger reset immediately
            if (filtered_temperature >= mcu_temp_redscreen) {
                fatal_error(ErrCode::ERR_TEMPERATURE_MCU_MAXTEMP_ERR, error_arg);
            }

            // Trigger and reset warning
            if (warning) {
                if (filtered_temperature < mcu_temp_warning - mcu_temp_hysteresis) {
                    warning = false;
                }
            } else {
                if (filtered_temperature >= mcu_temp_warning) {
                    warning = true;
                }
            }

            this->checkTrue(!warning, warning_type, true, true);
        }
    };

    // we keep old array size instead of PhysicalToolIndex::count because of weak indexing (see definition of PhysicalToolIndex::count)
    constinit StrongIndexArray<ErrorChecker, PhysicalToolIndex::count, PhysicalToolIndex, PhysicalToolIndex::to_raw_static, strong_index_array::AllowWeakIndexing::yes> hotendFanErrorChecker;
    constinit ErrorChecker printFanErrorChecker;
#if HAS_INDX()
    constinit ErrorChecker dockFanErrorChecker;
#endif

#if XBUDDY_EXTENSION_VARIANT_IS_STANDARD()
    constinit ErrorChecker xbe_cool_fan_checker; // Handles both cooling fans (we cannot differentiate anyway)
    constinit ErrorChecker xbe_filter_fan_checker;
#endif

#if XL_ENCLOSURE_SUPPORT()
    constinit ErrorChecker enclosure_fan_checker;
#endif

#if HAS_TEMP_HEATBREAK
    // we keep old array size instead of PhysicalToolIndex::count because of weak indexing (see definition of PhysicalToolIndex::count)
    constinit StrongIndexArray<ErrorChecker, PhysicalToolIndex::count, PhysicalToolIndex, PhysicalToolIndex::to_raw_static, strong_index_array::AllowWeakIndexing::yes> heatBreakThermistorErrorChecker;
#endif
    constinit HotendErrorChecker hotendErrorChecker;

    constinit MCUTempErrorChecker mcuMaxTempErrorChecker; ///< Check Buddy MCU temperature
#if HAS_DWARF()
    /// Check Dwarf MCU temperature
    // we keep old array size instead of PhysicalToolIndex::count because of weak indexing (see definition of PhysicalToolIndex::count)
    constinit StrongIndexArray<MCUTempErrorChecker, PhysicalToolIndex::count, PhysicalToolIndex, PhysicalToolIndex::to_raw_static, strong_index_array::AllowWeakIndexing::yes> dwarfMaxTempErrorChecker;
#endif /*HAS_DWARF()*/
#if HAS_REMOTE_BED()
    constinit MCUTempErrorChecker modbedMaxTempErrorChecker; ///< Check ModularBed MCU temperature
#endif

    void process_pausing_begin_state(Pause_Type type) {
        if (!server.print_is_serial) {
            switch (type) {

            case Pause_Type::Crash:
                print_state.skip_gcode = false;
                break;

            case Pause_Type::Pause:
                print_state.skip_gcode = true;
                break;
            }

            media_prefetch.stop();
            queue.clear();
            log_debug(MarlinServer, "Paused at %" PRIu32 ", skip %i", media_position(), print_state.skip_gcode);
        }

#if HAS_SERIAL_PRINT()
        SerialPrinting::pause();
#endif

        print_job_timer.pause();
        server.resume.nozzle_temp = buddy::safety_timer().original_hotend_targets();
        server.resume.fan_speed = marlin_vars().print_fan_speed; // save fan speed
        server.resume.print_speed = marlin_vars().print_speed;
#if HAS_INDX()
        server.resume.active_tool = match(
            PhysicalToolIndex::currently_selected(),
            [](PhysicalToolIndex tool) { return tool.to_raw(); },
            [](NoTool) { return PrusaToolChanger::MARLIN_NO_TOOL_PICKED; });
#endif
        if (hotendErrorChecker.runFullFan()) {
            thermalManager.set_print_fan_speed(255);
        } else {
            thermalManager.set_print_fan_speed(0); // disable print fan
        }
    }
} // end anonymous namespace

/******************************************************************************/
// Warning handling

static std::bitset<std::to_underlying(WarningType::_cnt)> warning_flags;
static uint32_t active_warning_pop_timestamp_sec = 0;

static void handle_warnings() {
    const auto phase_opt = fsm_states[ClientFSM::Warning];
    if (!phase_opt.has_value()) {
        return;
    }

    const auto phase = static_cast<PhasesWarning>(phase_opt->GetPhase());
    const auto warning_type = fsm::deserialize_data<WarningType>(phase_opt->GetData());

    // Timeout
    if (fsm_states.get_top()->fsm_type != ClientFSM::Warning) {
        // Some other FSM is on top of Warning FSM - reset warning lifespan timestamp
        active_warning_pop_timestamp_sec = ticks_s();
    }
    if (ticks_s() - active_warning_pop_timestamp_sec > warning_lifespan_sec(warning_type)) {
        clear_warning(warning_type);
        return;
    }

    // Peek the response, only some of the warnings consume it
    const auto response = get_response_from_phase(phase, false);
    if (response == Response::_none) {
        return;
    }

    switch (phase) {

    case PhasesWarning::Warning:
        // The only response is OK, at which point we just consume the response and hide the warning.
        break;

#if HAS_CHAMBER_VENTS()
    case PhasesWarning::ChamberVents:
        if (response == Response::Disable) {
            config_store().check_chamber_vent_state.set(false);
        }
        break;
#endif

#if HAS_CHAMBER_FILTRATION_API()
    case PhasesWarning::EnclosureFilterExpiration:
        buddy::chamber_filtration().handle_filter_expiration_warning(response);
        break;
#endif

#if HAS_ILI9488_DISPLAY() && HAS_HUMAN_INTERACTIONS()
    case PhasesWarning::DisplayProblemDetected:
        config_store().reduce_display_baudrate.set(response == Response::Yes);
        break;
#endif

    default:
        // Most warnings are handled somewhere else and we shouldn't process the responses here
        // Return to avoid consuming the response
        return;
    }

    // Consume the response now
    get_response_from_phase(phase, true);
    clear_warning(warning_type);
}

static void update_warning_fsm() {
    if (warning_flags.any()) {
        size_t i = 0;
        for (; !warning_flags.test(i); i++)
            ;
        const WarningType type = static_cast<WarningType>(i);
        const fsm::PhaseData data = fsm::serialize_data<WarningType>(type);

        // Avoid reinit of warning timestamp timer if warning is already shown
        if (!fsm_states[ClientFSM::Warning].has_value() || fsm_states[ClientFSM::Warning]->GetData() != data) {
            active_warning_pop_timestamp_sec = ticks_s();

            // Clear any pending responses for this FSM.
            // The displayed warning has changed, we don't want some stray response to be accidentally processed
            clear_fsm_response(ClientFSM::Warning);

            fsm_create(warning_type_phase(type), data);
        }
    } else {
        fsm_destroy(ClientFSM::Warning);
    }
}

void set_warning(WarningType type) {
    if (!warning_flags.test(std::to_underlying(type))) {
        log_info(MarlinServer, "set_warning(%" PRIu32 ")", std::to_underlying(type));

        warning_flags.set(std::to_underlying(type));
        update_warning_fsm();
    }
}

void clear_warning(WarningType type) {
    if (warning_flags.test(std::to_underlying(type))) {
        warning_flags.reset(std::to_underlying(type));
        update_warning_fsm();
    }
}

bool is_warning_active(WarningType type) {
    return warning_flags.test(std::to_underlying(type));
}

Response prompt_warning(WarningType type, uint32_t timeout_ms) {
    set_warning(type);
    const Response r = wait_for_response(warning_type_phase(type), timeout_ms);
    clear_warning(type);
    return r;
}

/******************************************************************************/
// FSM Manipulation

static void commit_fsm_states() {
    fsm_states.increment_state_id();
    marlin_vars().set_fsm_states(fsm_states);
    fsm_states.log();
}

void fsm_create(FSMAndPhase fsm_and_phase, fsm::PhaseData data) {
    fsm_change(fsm_and_phase, data);
}

void fsm_destroy(ClientFSM type) {
    if (fsm_states[type].has_value()) {
        fsm_states[type] = std::nullopt;
        commit_fsm_states();
    }
}

bool is_fsm_active(ClientFSM type) {
    return fsm_states[type].has_value();
}

void fsm_change(FSMAndPhase fsm_and_phase, fsm::PhaseData data) {
    const auto base_data = fsm::BaseData(fsm_and_phase.phase, data);

    auto &fsm_state = fsm_states[fsm_and_phase.fsm];

    if (!fsm_state.has_value() || fsm_state->GetPhase() != fsm_and_phase.phase) {
        // Clear any pending responses for this FSM. They might have been sent a long time ago and we don't want them to affect the behavior.
        marlin_server::clear_fsm_response(fsm_and_phase.fsm);
    }

    if (fsm_state != base_data) {
        fsm_state = base_data;
        commit_fsm_states();
    }
}

static void fsm_destroy_and_create(ClientFSM old_type, ClientFSM new_type, fsm::BaseData data) {
    fsm_states[old_type] = std::nullopt;
    fsm_states[new_type] = data;
    commit_fsm_states();
}

//-----------------------------------------------------------------------------
// variables

osThreadId server_task = 0; // task handle

/// Whether marlin_server::cycle() is currently running - for nesting prevention
static bool is_cycle_running = false;

//-----------------------------------------------------------------------------
// forward declarations of private functions

static void _server_print_loop(void);
static uint64_t _send_notify_events_to_client(int client_id, ClientQueue &queue, uint64_t evt_msk);
static uint8_t _send_notify_event(Event evt_id, uint32_t usr32, uint16_t usr16);
static void _server_update_vars();
static bool _process_server_request(const Request &);
static void _server_set_var(const Request &);
static void process_request_flags();

static void retract();
static void lift_head();
static void retract_and_lift();
static void park_head(bool is_pause);

static void settings_load();

//-----------------------------------------------------------------------------
// server side functions

void init(void) {
    int i;
    server = server_t();
    server.flags = 0;
    for (i = 0; i < MARLIN_MAX_CLIENTS; i++) {
        server.notify_events[i] = make_mask(Event::Acknowledge); // by default only ack
        server.notify_changes[i] = 0; // by default nothing
    }
    server_task = osThreadGetId();

    // Random at boot, to avoid chance of reusing the same (0/1) dialog ID
    // after a reboot.
    fsm_states.init_state_id();

#if HAS_SHEET_PROFILES()
    SteelSheets::CheckIfCurrentValid();
#endif
    settings_load();
#if HAS_INDX()
    prusa_toolchanger.load_tool_info();
    prusa_toolchanger.restore_last_picked_tool();
#endif

#if HAS_FILAMENT_TRACKER() && HAS_AUTO_RETRACT()
    for (auto tool : PhysicalToolIndex::all()) {
        // If we know that some filament is auto-retracted, pass it over to the filament_tracker
        auto retracted_dist = buddy::auto_retract().retracted_distance(tool);
        if (retracted_dist.has_value()) {
            buddy::filament_tracker().assume_retracted_distance(tool, retracted_dist);
        }
    }
#endif

#if PRINTER_IS_PRUSA_XL()
    // Raise any pending variant-detection warnings set by the puppy task at boot
    switch (xl_type_detection_result.load()) {

    case XLTypeDetectionResult::ok:
        break;

    case XLTypeDetectionResult::detected_as_xls: {
        change_extended_printer_type(PrinterModel::xls, ChangeExtendedPrinterTypeMode::standard_with_marlin_client_and_puppies);
        set_warning(WarningType::PrinterDetectedAsXLS);
        break;
    }

    case XLTypeDetectionResult::detected_as_xl: {
        change_extended_printer_type(PrinterModel::xl, ChangeExtendedPrinterTypeMode::standard_with_marlin_client_and_puppies);

        if (config_store().printer_hw_config_done.get()) {
            set_warning(WarningType::PrinterDetectedAsXL);

        } else {
            // User will see the printer type on the printer setup page, no need to display the warning
            // Without this, non-XLS users would get the warning after factory reset
        }
        break;
    }

    case XLTypeDetectionResult::wiring_suspected:
        set_warning(WarningType::XlCanWiringSuspected);
        break;
    }

#endif
}

void print_fan_spd() {
    static uint32_t last_fan_report = 0;
    uint32_t current_time = ticks_s();
    if (M123::fan_auto_report_delay && (current_time - last_fan_report) >= M123::fan_auto_report_delay) {
        M123::print_fan_speed();
        last_fan_report = current_time;
    }
}

#if HAS_NFC()

void handle_nfc() {
    static uint32_t last_check = 0;
    const uint32_t current_time = ticks_ms();
    if (last_check > current_time || (current_time - last_check) >= nfc::OPTIMAL_CHECK_DIFF_MS) {
        last_check = current_time;

        if (nfc::has_activity()) {
            if (const std::optional<WifiCredentials> wifi_credentials = nfc::consume_data()) {
                network_wizard::network_nfc_wizard(*wifi_credentials);
            }
        }
    }
}

#endif

#if HAS_MMU2()
/// Helper function that enqueues gcodes to safely unload filament from nozzle back to mmu
///
/// To safely unload a filament we need to ensure that the nozzle has correct temperature.
/// This can be safely done by using the `M702` gcode with `W2` argument. The gcode unloads
/// the filament back to mmu and with the argument waits  for correct temperature (if the
/// temperature is bigger than nessesary the gcode (with this argument) doesn't wait for
/// cooldown.
///
/// After the filament is unloaded then we need to restore original temperature. Since we
/// are enqueueing gcode, we can't set it directly and we need to enque another gcode. We
/// can do this since this will be only called at the end of the print or when aborting.
/// So it shouldn't overwrite any important gcodes.
void safely_unload_filament_from_nozzle_to_mmu() {
    if (MMU2::WhereIsFilament() == MMU2::FilamentState::NOT_PRESENT) {
        return; // no filament loaded, nothing to do
    }
    const uint16_t original_temp = thermalManager.degTargetHotend(active_extruder);
    enqueue_gcode("M702 W2");
    enqueue_gcode_printf("M104 S%" PRIu16, original_temp);
}
#endif

void server_update_vars() {
    uint32_t tick = ticks_ms();
    if ((tick - server.last_update) > MARLIN_UPDATE_PERIOD) {
        server.last_update = tick;
        _server_update_vars();
    }
}

void send_notifications_to_clients() {
    for (int client_id = 0; client_id < MARLIN_MAX_CLIENTS; client_id++) {
        ClientQueue &queue = marlin_client::marlin_client_queue[client_id];
        if (const uint64_t msk = server.client_events[client_id]) {
            server.client_events[client_id] &= ~_send_notify_events_to_client(client_id, queue, msk);
        }
    }
}

#if HAS_I2C_EXPANDER()

// Used to avoid multiple triggering of pressed buttons.
static uint8_t io_expander_button_trigger_check(uint8_t pin_states, uint8_t pin_mask) {
    static uint8_t prev_pressed_buttons = 0;

    // Pin states are inversed - pin is low on button press
    const auto pressed_buttons = (~pin_states) & pin_mask;
    const auto triggered_buttons = pressed_buttons & ~prev_pressed_buttons;
    prev_pressed_buttons = pressed_buttons;

    return triggered_buttons;
}

void io_expander_read_loop() {
    if (!buddy::hw::io_expander2.is_initialized()) {
        return;
    }
    if (uint8_t pin_mask = config_store().io_expander_config_register.get()) {
        static constexpr int32_t io_expander_read_loop_delay_ms = 500;
        static uint32_t last_tick_ms = ticks_ms();
        uint32_t tick_ms = ticks_ms();
        if (ticks_diff(tick_ms, last_tick_ms) >= io_expander_read_loop_delay_ms) {
            if (const auto value = buddy::hw::io_expander2.read(pin_mask)) {

                // Debouncing mechanism - after pressing a button, there have to be at least one released state before button can be pressed again
                uint8_t pressed_buttons_mask = io_expander_button_trigger_check(*value, pin_mask);

                for (uint8_t pin_number = 0; pin_number < buddy::hw::TCA6408A::pin_count; pin_number++) {
                    // Create a mask and extract the pin from the pressed_buttons_mask
                    const uint8_t single_pin_mask = 0x1 << pin_number;

                    if (pin_mask & single_pin_mask & pressed_buttons_mask) {
                        if (!inject(GCodeMacroButton(pin_number))) {
                            SERIAL_ECHOLIST("Injecting Macro Button failed, pin: ", pin_number);
                        }
                    }
                }
            }
            last_tick_ms = tick_ms;
        }
    }
}
#endif // HAS_I2C_EXPANDER()

static void cycle() {
    // Some things are somewhat time-sensitive and should be updated even in nested loops
#if HAS_CHAMBER_API()
    buddy::chamber().step();
#endif

#if HAS_CHAMBER_FILTRATION_API()
    buddy::chamber_filtration().step();
#endif

#if HAS_EMERGENCY_STOP()
    buddy::emergency_stop().step();

    // During printing, possibly block anytime, with exception of Load Unload sequence
    // In case there would be planned unsafe moves, there is another buddy::emergency_stop().maybe_block() directly in planner
    if (is_printing_state(server.print_state) && !fsm_states.is_active(ClientFSM::Load_unload)) {
        buddy::emergency_stop().maybe_block();
    }
#endif

#if HAS_HT_HOTEND()
    buddy::check_hotend_burn_risk();
#endif

#if XBUDDY_EXTENSION_VARIANT_IS_STANDARD()
    buddy::xbuddy_extension().step();
#endif

#if HAS_ANFC()
    buddy::openprinttag::filament_usage_tracker().step();
#endif

    buddy::safety_timer().step();

    // Although the timeout should never trigger within idle() (= when a gcode is run),
    // We still need to run the step() there to prevent "sampling bias" so that the timer could reset itself during movements and single-injected gcodes
    buddy::stepper_timeout().step();

#if HAS_BED_FAN()
    bed_fan::controller().step();
#endif

#if HAS_INDX()
    IndxHotend::process_pending_thermal_runaway();
#endif

    FSensors_instance().step();

    record_fanctl_metrics();

    idle_publisher.call_all();

    if (is_cycle_running) {
        return;
    }
    AutoRestore _nr(is_cycle_running, true);

#if HAS_MMU2()
    MMU2::Fsm::Instance().Loop();
#endif

    handle_warnings();

#if XL_ENCLOSURE_SUPPORT()
    int16_t dwarf_temp = std::numeric_limits<int16_t>().min();
    #if HAS_TOOLCHANGER()
    dwarf_temp = prusa_toolchanger.getActiveToolOrFirst().get_board_temperature();
    #endif

    xl_enclosure.loop(remote_bed::get_mcu_temperature(), dwarf_temp);
#endif

#if HAS_XL_CAN() && HAS_REMOTE_BED()
    // is_enabled() latches at bootstrap and never clears, so the controller's
    // running state stays coherent across calls. During the M1978 fan selftest
    // these set_fan_pwm() calls are ignored (XlCan fan selftest mode), so the
    // policy can't race the test's PWM commands. A manual M106 P8 override
    // replaces the policy output; update() still runs so the hysteresis state
    // is fresh when control reverts to automatic.
    if (buddy::puppies::xl_can.is_enabled()) {
        buddy::puppies::xl_can.set_fan_pwm(buddy::ModularBedFanControl::instance().update(remote_bed::get_mcu_temperature()));
    }
#endif

#if HAS_SELFTEST()
    if (!SelftestInstance().IsInProgress()) {
#else
    // #error dead code found by automatic analyses (see BFW-5461)
    {
#endif
        _server_print_loop(); // we need call print loop here because it must be processed while blocking commands (M109)
    }

    // Clear temporary print status messages that have timed out -
    // but only if the printer isn't paused.
    // [BFW-6485] People like to use M117 (show message) before M601
    if (!is_extended_paused_state(server.print_state)) {
        print_status_message().clear_timed_out_temporary();
    }

    print_fan_spd();

#if HAS_TOOLCHANGER()
    // Check if tool didn't fall off
    prusa_toolchanger.loop(!printer_idle(), printer_paused());
#endif

#if HAS_I2C_EXPANDER()
    io_expander_read_loop();
#endif // HAS_I2C_EXPANDER()

    process_request_flags();

    if (Request request; request_queue.try_receive(request, 0)) {
        _process_server_request(request);
    }

    // update variables
    send_notifications_to_clients();
    server_update_vars();
}

/// Function that is called just before finalize_print, before the steppers are possibly disabled
/// \retval true the function did its job and we can continue with the state machine
/// \retval false the function is not ready yet, we need to call it later again (loop in the same state)
static bool pre_finalize_print([[maybe_unused]] bool finished) {
#if HAS_ANFC()
    buddy::openprinttag::filament_usage_tracker().flush({
        .tools = AllTools {},
        .warn_on_failure = true,
    });
#endif

#if HAS_AUTO_RETRACT()
    // During multi tool printing, slicer handles retraction/ramming and keeps FW in the dark
    // FilamentTracker keeps track of retracted distances on each hotend
    // This overwrites retracted distances in persistent storage with temporary ones from FilamentTracker
    for (auto tool : PhysicalToolIndex::all()) {
        const auto dist = buddy::filament_tracker().get_retracted_distance(tool);
        // Do not save filament_tracker value before it was validated
        if (dist.has_value()) {
            // update only used hotends
            buddy::auto_retract().set_retracted_distance(tool, dist);
        }
    }
#endif

#if HAS_MMU2()
    if (MMU2::mmu2.Enabled()) {
        // Unloading from nozzle is handled by Slicer, do not use auto_retract (frequent filament changes cause filament_tracker cannot properly hold valid value)
        // When we are running single-filament gcode with MMU, we should unload current filament.
        if (!finished || GCodeInfo::getInstance().is_singletool_gcode()) {
            safely_unload_filament_from_nozzle_to_mmu();
        }
    } else
#endif

#if HAS_AUTO_RETRACT()
        if (true) {
        buddy::auto_retract().maybe_retract_from_nozzle();
    } else
#endif
    {
    }

#if HAS_NOZZLE_CLEANER()
    // Skip nozzle cleaning if no tool is picked (e.g. tool already parked during pause on INDX)
    if (!std::holds_alternative<NoTool>(PhysicalToolIndex::currently_selected())) {
        if (!nozzle_cleaner::load_and_execute(nozzle_cleaner::Sequence::clean)) {
            return false;
        }
        mapi::park(mapi::get_parking_position(mapi::ParkPosition::park).without_z_move());
    }
#endif

#if HAS_INDX()
    // Park the tool to its dock and persist the state to eeprom.
    // Done here (still with axes homed) so tool_change's ensure_safe_move() doesn't trigger another G28.
    tool_change(NoTool {}, tool_return_t::no_return);
    prusa_toolchanger.persist_last_picked_tool(PhysicalToolIndex::currently_selected(), true);
#endif

#if HAS_CHAMBER_VENTS()
    buddy::chamber().close_vents_after_print();
#endif

    disable_e_steppers();
    disable_xy_steppers();

    return true;
}

/// To be called when completely exiting/resetting the server state - when user exits print preview or printing screen
static void cleanup_print(ResetToolMapping reset_tool_mapping = ResetToolMapping::yes) {
    gcode.compatibility = {};

    if (bool(reset_tool_mapping)) {
#if HAS_TOOL_MAPPING()
        tool_mapper.reset();
#endif

#if HAS_SPOOL_JOIN()
        spool_join.reset();
#endif
    }

    media_prefetch.stop();

    PrintPreview::Instance().ChangeState(IPrintPreview::State::inactive);
    fsm_destroy(ClientFSM::PrintPreview);
    fsm_destroy(ClientFSM::Printing);
#if HAS_SERIAL_PRINT()
    fsm_destroy(ClientFSM::Serial_printing);
#endif

    server.print_is_serial = false;
    server.print_state = State::Idle;
}

/// To be called when the printer goes from printing -> not printing end state (finished, aborted, ...)
/// Executes all the print finishing touches
void static finalize_print(bool finished) {
#if HAS_POWER_PANIC()
    power_panic::reset();
#endif

    print_job_timer.stop();

#if HAS_INDX()
    // Stop the dock fan at the end of every print - the slicer is not
    // guaranteed to emit M107 P6, so it could otherwise keep spinning.
    Fans::dock_fan().set_pwm(0);
#endif

    _server_update_vars();
    // Check if the stopwatch was NOT stopped to and add the current printime to the statistics.
    // finalize_print is being called multiple times and we don't want to add the time twice.
    if (!server.was_print_time_saved) {
        Odometer_s::instance().add_time(marlin_vars().print_duration);
        server.was_print_time_saved = true;
    }
    // print_maintenance();
#if HAS_MMU2()
    if (!server.mmu_maintenance_checked) {
        if (auto reason = MMU2::check_maintenance(); reason.has_value()) {
            switch (reason.value()) {
            case MMU2::MaintenanceReason::Changes:
                set_warning(WarningType::MaintenanceWarningChanges);
                break;
            case MMU2::MaintenanceReason::Failures:
                set_warning(WarningType::MaintenanceWarningFails);
                break;
            default:
                bsod_unreachable();
            }
        }
        server.mmu_maintenance_checked = true;
    }
#endif // HAS_MMU2()

#if HAS_MOTOR_CURRENT_PROFILES()
    buddy::set_active_motor_current_profile(buddy::StandardMotorCurrentProfile::fw_default);
#endif
#if !PRINTER_IS_PRUSA_iX()
    // On iX, we're not cooling down the bed after the print.
    // Resetting bounding rect would result in turning all bedlets on, which we don't want.
    // First - it's increasing power consumption; second - it could clear the bed preheat status.
    // BFW-5085
    print_area.reset_bounding_rect();
#endif

#if HAS_CHAMBER_API()
    buddy::chamber().reset();
#endif
    // Reset IS at the end of the print
    input_shaper::init();

    media_prefetch.stop();

    marlin_vars().print_end_time = time(nullptr);
    marlin_vars().add_job_result(job_id, finished ? marlin_vars_t::JobInfo::JobResult::finished : marlin_vars_t::JobInfo::JobResult::aborted);
#if HAS_E2EE_SUPPORT()
    e2ee::remove_temporary_identites();
#endif

#if HAS_CHAMBER_FILTRATION_API()
    buddy::chamber_filtration().check_filter_expiration();

    /// Reset filtration overrides possibly set by M147/148
    buddy::chamber_filtration().set_needs_filtration_override(Tristate::other);
#endif

#if XBUDDY_EXTENSION_VARIANT_IS_STANDARD()
    buddy::xbuddy_extension().set_chamber_regulator_legacy(true); // For compatibility with old gcodes on coreone
#endif

    if (config_store().show_fsensors_disabled_warning_after_print.get()) {
        config_store().show_fsensors_disabled_warning_after_print.set(false);
        set_warning(WarningType::FilamentSensorsDisabled);
    }

    if (server.print_is_serial) {
        // Clean up immediately - serial print is not supposed to wait for the user to confirm the exit
        cleanup_print();
    }

    // Do not remove, needed for 3rd party tools such as octoprint to get status that the gcode file printing has finished
    SERIAL_ECHOLNPGM(MSG_FILE_PRINTED);
}

#if HAS_CRASH_DETECTION() || HAS_POWER_PANIC()
static void check_crash() {
    // reset the nested loop check once per main server iteration
    crash_s.needs_stack_unwind = false;

    #if HAS_POWER_PANIC()
    // handle server state-change overrides happening in the ISRs here (and nowhere else)
    if (power_panic::panic_is_active()) {
        server.print_state = State::PowerPanic_acFault;
        return;
    }
    #endif

    // Start crash recovery if TRIGGERED, but not if print is already being aborted
    if ((server.print_state != State::Aborting_Begin)
        && ((crash_s.get_state() == Crash_s::TRIGGERED_ISR)
            || (crash_s.get_state() == Crash_s::TRIGGERED_TOOLFALL)
            || (crash_s.get_state() == Crash_s::TRIGGERED_TOOLCRASH)
            || (crash_s.get_state() == Crash_s::TRIGGERED_GCODE_INTERRUPT)
            || (crash_s.get_state() == Crash_s::TRIGGERED_HOMEFAIL))) {

        // Set again to prevent race when ISR happens during this function
        crash_s.needs_stack_unwind = false;
        server.print_state = State::CrashRecovery_Begin;
        return;
    }
}
#endif

void loop() {
    ::idle(false); // Do an idle first so boot is slightly faster
#if HAS_POWER_PANIC()
    if (!power_panic::ac_fault_triggered) {
#else
    {
#endif
        // On power panic, avoid:
        // * a 1ms sleep
        // * a risk someone might inject a command in between (filament runout,
        //   estall, ...)
        queue.advance();
    }

#if HAS_SELFTEST()
    if (SelftestInstance().IsInProgress()) {
        SelftestInstance().Loop();
    }
#endif

#if HAS_CRASH_DETECTION() || HAS_POWER_PANIC()
    check_crash();
#endif

    // Revert quick_stop when commands already drained
    if (server.flags & MARLIN_SFLG_STOPPED && !is_processing()) {
        planner.resume_queuing();
        server.flags &= ~MARLIN_SFLG_STOPPED;
    }

    cycle();

#if HAS_NFC()
    if (printer_idle() && !fsm_states.get_top().has_value()) {
        handle_nfc();
    }
#endif

    print_utils_loop();
}

static bool idle_running = false;

void do_babystep_Z(float offs) {
    babystep.add_steps(Z_AXIS, static_cast<int16_t>(std::round(offs * planner.settings.axis_steps_per_mm[Z_AXIS])));
}

void enqueue_gcode(const char *gcode) {
    if (!queue.enqueue_one(gcode)) {
        bsod("enqueue_gcode failed");
    }
}

[[nodiscard]] bool enqueue_gcode_try(const char *gcode) {
    return queue.enqueue_one(gcode);
}

void enqueue_gcode_printf(const char *gcode, ...) {
    ArrayStringBuilder<MARLIN_MAX_REQUEST> request;
    {
        va_list ap;
        va_start(ap, gcode);
        request.append_vprintf(gcode, ap);
        va_end(ap);
    }
    enqueue_gcode(request.str());
}

bool inject(InjectQueueRecord record) {
    if (!queue.inject(record)) {
        return false;
    }
    return true;
}

void gcode_interrupt(GCodeLiteral gcode) {
#if !HAS_CRASH_DETECTION()
    inject(gcode);

#else
    // In some situations we just inject the gcode
    if (
        // When we're not printing (or we're printing serial)
        !crash_s.is_active() || crash_s.get_state() != Crash_s::PRINTING
        || server.print_is_serial
        || (server.print_state != State::Printing && server.print_state != State::Finishing_WaitIdle)

        // When the currently executed gcode doesn't support partial replay
        // This means basically all gcodes except for G0/1/2/3
        || !(crash_s.gcode_state.recover_flags & Crash_s::RECOVER_PARTIAL_REPLAY) //
    ) {
        inject(gcode);
        return;
    }

    print_state.gcode_interrupt_command = gcode;

    // We're using crash recovery machinery to stop the motors, store state and such
    Crash_s::instance().set_state(Crash_s::TRIGGERED_GCODE_INTERRUPT);
#endif
}

static void settings_load() {
    (void)settings.reset();
#if HAS_SHEET_PROFILES()
    probe_offset.z = SteelSheets::GetZOffset();
#endif
#if ENABLED(PIDTEMPBED)
    Temperature::temp_bed.pid.Kp = DEFAULT_bedKp;
    Temperature::temp_bed.pid.Ki = scalePID_i(DEFAULT_bedKi);
    Temperature::temp_bed.pid.Kd = scalePID_d(DEFAULT_bedKd);
#endif
#if ENABLED(PIDTEMP)
    for (auto tool : PhysicalToolIndex::all()) {
        Hotend::for_tool(tool).set_nozzle_pid_config(HotendPIDConfig {});
    }
#endif

    marlin_vars().fan_check_enabled = config_store().fan_check_enabled.get();

    planner.set_stealth_mode(config_store().stealth_mode.get());

    job_id = config_store().job_id.get();

#if HAS_TOOLCHANGER()
    // TODO: This is temporary until better offset store method is implemented
    prusa_toolchanger.load_tool_offsets();
#endif

#if HAS_PHASE_STEPPING()
    phase_stepping::load();
#endif
}

#if HAS_SELFTEST()
void test_start([[maybe_unused]] const uint64_t test_mask, [[maybe_unused]] const selftest::TestData test_data) {
    if (((server.print_state == State::Idle) || (server.print_state == State::Finished) || (server.print_state == State::Aborted)) && (!SelftestInstance().IsInProgress())) {
        SelftestInstance().Start(test_mask, test_data);
    }
}

void test_abort() {
    if (SelftestInstance().IsInProgress()) {
        SelftestInstance().Abort();
    }
}
#endif

void quick_stop() {
#if HAS_TOOLCHANGER()
    prusa_toolchanger.quick_stop();
#endif
    planner.quick_stop();
    disable_all_steppers();
    set_all_unhomed();
    server.flags |= MARLIN_SFLG_STOPPED;
}

bool printer_idle() {
    return server.print_state == State::Idle
        || server.print_state == State::Paused
        || server.print_state == State::Aborted
        || server.print_state == State::Finished
        || server.print_state == State::Exit;
}

bool print_preview() {
    return server.print_state == State::PrintPreviewInit
        || server.print_state == State::PrintPreviewImage
        || server.print_state == State::PrintPreviewConfirmed
        || server.print_state == State::PrintPreviewQuestions
#if HAS_TOOL_MAPPING()
        || server.print_state == State::PrintPreviewToolsMapping
#endif
        ;
}

bool is_printing() {
    switch (marlin_vars().print_state) {
    case State::Aborted:
    case State::Idle:
    case State::Finished:
    case State::PrintPreviewInit:
    case State::PrintPreviewImage:
#if HAS_TOOL_MAPPING()
    case State::PrintPreviewToolsMapping:
#endif
        return false;
    default:
        return true;
    }
}

bool is_processing() {
    return queue.has_commands_queued()
        || planner.processing()
        || gcode.busy_state != GcodeSuite::NOT_BUSY // We might be still in the gcode (while no commands are queued)
#if HAS_SELFTEST()
        || SelftestInstance().IsInProgress() // Some selftests are still not gcodes :(
#endif
        || !inject_queue.is_empty() //
        ;
}

bool aborting_or_aborted() {
    return (server.print_state >= State::Aborting_Begin && server.print_state <= State::Aborted);
}

bool finishing_or_finished() {
    switch (server.print_state) {
    case State::Finishing_UnloadFilament:
    case State::Finishing_ParkHead:
    case State::Finished:
        return true;

        // ! WaitIdle means the printer is waiting for the queued gcodes to finish, so it's still a printing state!
    case State::Finishing_WaitIdle:
    default:
        return false;
    }
}

bool printer_paused() {
    return server.print_state == State::Paused;
}

// Printer is paused, parking for pause, resuming from pause...
bool printer_paused_extended() {
    return is_extended_paused_state(server.print_state);
}

#if HAS_SERIAL_PRINT()
void serial_print_start() {
    if (server.print_state != State::Idle) {
        finalize_print(false);
        cleanup_print();
    }

    server.print_state = State::SerialPrintInit;
    print_state = {};
}
#endif

void print_start(const char *filename, const GCodeReaderPosition &resume_pos, PreviewSkipIfAble skip_preview, ResetToolMapping reset_tool_mapping) {
#if HAS_SELFTEST()
    if (SelftestInstance().IsInProgress()) {
        return;
    }
#endif
    if (filename == nullptr) {
        return;
    }

    // Clear warnings before print, like heaters disabled after 30 minutes.
    clear_warning(WarningType::HeatersTimeout);

    switch (server.print_state) {

        // handle preview / reprint
    case State::Finished:
    case State::Aborted:
        // correctly end previous print
        finalize_print(server.print_state == State::Finished);
        cleanup_print(reset_tool_mapping);
        break;

    case State::Idle:
    case State::PrintPreviewInit:
    case State::PrintPreviewImage:
    case State::PrintPreviewConfirmed:
    case State::PrintPreviewQuestions:
#if HAS_TOOL_MAPPING()
    case State::PrintPreviewToolsMapping:
#endif
        // These are acceptable states from which we can start the print -> continue executing the function
        break;

    default:
        // Do not start the print from other states
        return;
    }

    print_state = {};

    if (filename) {
        // Avoid possible deadlocks by disabling a gcode scan, if there's any.
        //
        // The deadlock could happen if:
        // * We started to download a file.
        // * We start to show the preview, but it's not yet downloaded enough
        //   to show on screen, in which case the scan _waits_ for it to become
        //   downloaded enough.
        // * Connect gets a command to start a print, forwarding it to us here.
        // * We would have to wait of that one to finish below (the async job thing).
        // * And we would be blocking Connect by not answering, therefore it
        //   would not be updating the file downloaded range... not unblocking
        //   the scan.
        gcode_info_scan::cancel_scan();
        // We need a copy of the sfn as well because get_LFN needs the address mutable :/
        std::array<char, filename_defs::path_buffer_size> filepath_sfn;
        strlcpy(filepath_sfn.data(), filename, filepath_sfn.size());

        std::array<char, filename_defs::filename_buffer_size> filename_lfn;

        // Do this in the async job thread to prevent blocking Marlin on I/O and possibly causing a watchdog reset
        AsyncJob async_job;
        async_job.issue([&](AsyncJobExecutionControl &) {
            get_LFN(filename_lfn.data(), filename_lfn.size(), filepath_sfn.data());
        });
        while (async_job.is_active()) {
            ::idle(true);
        }

        // Update marlin vars
        {
            MarlinVarsLockGuard lock;

            // update media_SFN_path
            strlcpy(marlin_vars().media_SFN_path.get_modifiable_ptr(lock), filepath_sfn.data(), marlin_vars().media_SFN_path.max_length());

            // set media_LFN
            strlcpy(marlin_vars().media_LFN.get_modifiable_ptr(lock), filename_lfn.data(), marlin_vars().media_LFN.max_length());
        }

        // Update GCodeInfo
        GCodeInfo::getInstance().set_gcode_file(filepath_sfn.data(), filename_lfn.data());
    }

    // Mostly we do not allow printing when some FSM is open (for example Load/Unload).
    // We allow printing from:
    //  - Printing - reprint
    //  - Print preview - calling print from internet
    const bool any_fsm_open = fsm_states.get_top().has_value();
    const bool print_preview_open = fsm_states.is_active(ClientFSM::PrintPreview);
    const bool printing_open = fsm_states.is_active(ClientFSM::Printing);
    const bool allowed_fsm_open = print_preview_open || printing_open;
    const bool can_print = !any_fsm_open || allowed_fsm_open;
    if (printing_open) {
        // FIXME: From the code in this function, it looks like this never happens.
        //        Let's gather some data and see if it does.
        log_error(MarlinServer, "ClientFSM::Printing is open and shouldn't be");
    }

    set_media_position(resume_pos.offset);
    print_state.media_restore_info = resume_pos.restore_info;

    if (skip_preview == PreviewSkipIfAble::no) {
        media_prefetch_lazy_start();
    } else {
        media_prefetch_start();
    }

    server.print_state = can_print ? State::PrintPreviewInit : State::Idle;

    PrintPreview::Instance().set_skip_if_able(skip_preview);
}

#if HAS_SERIAL_PRINT()
void serial_print_finalize(void) {
    switch (server.print_state) {

    case State::Printing:
    case State::Paused:
    case State::Resuming_Reheating:
    case State::Finishing_WaitIdle:
    #if HAS_TOOL_CRASH_RECOVERY()
    case State::CrashRecovery_Tool_Pickup:
    #endif
        server.print_state = State::Finishing_WaitIdle;
        break;
    default:
        break;
    }
}
#endif

void print_abort(void) {

    switch (server.print_state) {

#if HAS_POWER_PANIC()
    case State::PowerPanic_Resume:
    case State::PowerPanic_AwaitingResume:
#endif
    case State::Printing:
    case State::Paused:
    case State::MediaErrorRecovery_BufferData:
    case State::Resuming_BufferData:
    case State::Resuming_Reheating:
    case State::Finishing_WaitIdle:
#if HAS_TOOL_CRASH_RECOVERY()
    case State::CrashRecovery_Tool_Pickup:
#endif
        server.print_state = State::Aborting_Begin;
        break;

    case State::PrintInit:
    case State::SerialPrintInit:
    case State::PrintPreviewInit:
    case State::PrintPreviewImage:
    case State::PrintPreviewConfirmed:
    case State::PrintPreviewQuestions:
#if HAS_TOOL_MAPPING()
    case State::PrintPreviewToolsMapping:
#endif
        server.print_state = State::Aborting_Preview;
        break;

    default:
        break;
    }
}

void print_exit(void) {
    switch (server.print_state) {

    case State::Finished:
    case State::Aborted:
        server.print_state = State::Exit;
        break;

    default:
        // Can't do muchF
        break;
    }
}

void print_pause(void) {
    print_state.resume_pending = false;

    switch (server.print_state) {
    case State::Printing:
    case State::Finishing_WaitIdle:
        server.print_state = State::Pausing_Begin;
        break;

    default:
        break;
    }
}

#if HAS_CRASH_DETECTION()
/**
 * @brief Go to homing or measure axis and follow with homing.
 */
static void measure_axes_and_home() {
    #if ENABLED(AXIS_MEASURE)
    if (crash_s.is_repeated_crash()) {
        // Measure axes
        enqueue_gcode("G163 X Y S" STRINGIFY(AXIS_MEASURE_STALL_GUARD) " P" STRINGIFY(AXIS_MEASURE_CRASH_PERIOD));
        server.print_state = State::CrashRecovery_XY_Measure;
        return;
    }
    #endif

    // Homing
    set_axis_is_not_at_home(X_AXIS);
    set_axis_is_not_at_home(Y_AXIS);
    server.print_state = State::CrashRecovery_XY_HOME;
}

    #if HAS_TOOL_CRASH_RECOVERY()
/**
 * @brief Deselect tool, disable XY steppers and switch to Tool_Pickup server print_state.
 */
static void prepare_tool_pickup() {
    prusa_toolchanger.crash_deselect_tool();
    disable_XY(); // Let user move the carriage

    // Disable heaters
    thermalManager.disable_hotend();

    server.print_state = State::CrashRecovery_Tool_Pickup; // Continue with screen to wait for user to pick tools
}

/**
 * @brief Part of crash recovery begin when reason of crash is the toolchanger.
 * @note This has to call fsm_create() exactly once.
 * @return true on toolcrash when there is no parking and replay and when should break current switch case
 */
static bool crash_recovery_begin_toolchange() {
        #if HAS_DWARF()
    const Crash_recovery_tool_fsm cr_fsm { .enabled = prusa_toolchanger.get_enabled_mask() };
    fsm_create(PhasesCrashRecovery::tool_recovery, cr_fsm.serialize()); // Ask user to park all dwarves
        #elif HAS_INDX()
    fsm_create(PhaseNozzleMismatch::tool_lost); // Ask user to recover the dropped nozzle
        #else
            #error "HAS_TOOL_CRASH_RECOVERY requires HAS_DWARF or HAS_INDX"
        #endif

    if (crash_s.get_state() == Crash_s::REPEAT_WAIT) {
        prepare_tool_pickup(); // If crash happens during toolchange, skip crash recovery and go directly to tool pickup
        return true;
    }
    return false;
}

/// @brief Part of crash recovery tool pickup: waits for the user and re-picks the tool
static void crash_recovery_tool_pickup() {
        #if HAS_DWARF()
    if ((marlin_server::get_response_from_phase(PhasesCrashRecovery::tool_recovery) == Response::Continue)
        && (prusa_toolchanger.get_enabled_mask() == prusa_toolchanger.get_parked_mask())) {

        // Show homing screen, TODO: perhaps a new screen would be better
        Crash_recovery_fsm cr_fsm(SelftestSubtestState_t::running, SelftestSubtestState_t::undef);
        fsm_change(PhasesCrashRecovery::home, cr_fsm.Serialize());

        // Pickup lost tool
        tool_return_t return_type = tool_return_t::no_return; // If it continues with replay, no need to return
        xyz_pos_t return_pos = current_position.xyz(); // return Z to current Z
        if (crash_s.get_state() == Crash_s::REPEAT_WAIT) {
            // After toolcrash, return to what was requested before the crash
            // return_pos is stored in logical coordinates
            return_pos = prusa_toolchanger.return_data().return_pos.asNative();
            return_type = prusa_toolchanger.return_data().return_type;
        }
        if (!prusa_toolchanger.tool_change(prusa_toolchanger.return_data().tool,
                return_type,
                return_pos,
                tool_change_lift_t::no_lift,
                /*z_return =*/true)) {
            if (crash_s.get_state() == Crash_s::TRIGGERED_AC_FAULT) {
                return; // Powerpanic, do not retry just end
            }

            // Toolchange failed again, ask user again to park all dwarves
            crash_s.count_crash(); // Count as another crash
            const Crash_recovery_tool_fsm cr_fsm { .enabled = prusa_toolchanger.get_enabled_mask() };
            fsm_change(PhasesCrashRecovery::tool_recovery, cr_fsm.serialize());

            prepare_tool_pickup();
            return;
        }

        server.print_state = State::CrashRecovery_XY_HOME; // Reheat and resume, unpark is skipped in later stages
    } else {
        const Crash_recovery_tool_fsm cr_fsm { .enabled = prusa_toolchanger.get_enabled_mask(), .parked = prusa_toolchanger.get_parked_mask() };
        fsm_change(PhasesCrashRecovery::tool_recovery, cr_fsm.serialize());
    }
        #elif HAS_INDX()
    // wait for the user to return dropped nozzle to its dock
    if (marlin_server::get_response_from_phase(PhaseNozzleMismatch::tool_lost) == Response::Continue) {
        // tool is re-picked by crash recovery fsm
        Crash_recovery_fsm cr_fsm(SelftestSubtestState_t::running, SelftestSubtestState_t::undef);
        fsm_destroy_and_create(ClientFSM::NozzleMismatch, ClientFSM::CrashRecovery,
            fsm::BaseData(std::to_underlying(PhasesCrashRecovery::home), cr_fsm.Serialize()));
        server.print_state = State::CrashRecovery_XY_HOME;
    }
        #else
            #error "HAS_TOOL_CRASH_RECOVERY requires HAS_DWARF or HAS_INDX"
        #endif
}
    #endif
#endif

void media_prefetch_lazy_start() {
    print_state.file_open_reported = false;
    media_prefetch.start(marlin_vars().media_SFN_path.get_ptr(), GCodeReaderPosition { stream_restore_info(), media_position() });
}

void media_prefetch_start() {
    media_prefetch_lazy_start();
    media_prefetch.issue_fetch();
}

void schedule_media_retry() {
    const auto backoff_time = print_state.recover_media_error_backoff.fail();
    print_state.recover_media_error_at = ticks_s() + backoff_time;
    log_info(MarlinServer, "Scheduled media retry at %" PRIu32 ", backoff %" PRIu32, *print_state.recover_media_error_at, backoff_time);
}

void clear_media_error() {
    if (!print_state.recover_media_error_backoff.get().has_value()) {
        return;
    }

    print_state.recover_media_error_at.reset();
    print_state.recover_media_error_backoff.reset();

    clear_warning(WarningType::USBFlashDiskError);
    clear_warning(WarningType::GcodeCorruption);
    clear_warning(WarningType::NotDownloaded);
}

std::optional<WarningType> prefetch_status_to_warning(MediaPrefetchManager::Status status) {
    using Status = MediaPrefetchManager::Status;

    switch (status) {

    case Status::usb_error:
        return WarningType::USBFlashDiskError;

    case Status::corruption:
        return WarningType::GcodeCorruption;

    case Status::not_downloaded:
        return WarningType::NotDownloaded;

    case Status::ok:
    case Status::end_of_buffer:
    case Status::end_of_file:
        return std::nullopt;
    }

    bsod_unreachable();
}

void media_print_loop() {
    /// Size of the gcode queue
    METRIC_DEF(metric_gcode_queue_size, "gcd_que_sz", METRIC_VALUE_INTEGER, 100, METRIC_ENABLED);
    metric_record_integer(&metric_gcode_queue_size, queue.length);

    while (queue.length < MEDIA_FETCH_GCODE_QUEUE_FILL_TARGET) {
        MediaPrefetchManager::ReadResult data;
        using Status = MediaPrefetchManager::Status;
        const auto status = media_prefetch.read_command(data);
        const auto metrics = media_prefetch.get_metrics();

        /// Status of the last media_prefetch.read_command. 0 = ok, 1 = end of file, other = error (means that we're stalling)
        METRIC_DEF(metric_fetch_status, "ftch_status", METRIC_VALUE_INTEGER, 100, METRIC_ENABLED);
        metric_record_integer(&metric_fetch_status, static_cast<int>(status.status));

        /// Status at the end of the buffer - for early error indication
        METRIC_DEF(metric_fetch_tail_status, "ftch_tstatus", METRIC_VALUE_INTEGER, 100, METRIC_ENABLED);
        metric_record_integer(&metric_fetch_tail_status, static_cast<int>(metrics.tail_status));

        /// Occupancy of the media prefetch buffer, in percent of the buffer size
        METRIC_DEF(metric_prefetch_buffer_occupancy, "ftch_occ", METRIC_VALUE_INTEGER, 100, METRIC_ENABLED);
        metric_record_integer(&metric_prefetch_buffer_occupancy, metrics.buffer_occupancy_percent);

        /// Number of commands in the prefetch buffer
        METRIC_DEF(metric_prefetch_buffer_commands, "ftch_cmds", METRIC_VALUE_INTEGER, 100, METRIC_ENABLED);
        metric_record_integer(&metric_prefetch_buffer_commands, metrics.commands_in_buffer);

        if (!print_state.file_open_reported && metrics.stream_size_estimate) {
            print_state.file_open_reported = true;

            // Do not remove, needed for 3rd party tools such as octoprint to get status about the gcode file being opened
            SERIAL_ECHOLNPAIR(MSG_SD_FILE_OPENED, marlin_vars().media_SFN_path.get_ptr(), " Size:", metrics.stream_size_estimate);
        }

        switch (status.status) {

        case Status::ok:
            if (print_state.skip_gcode) {
                print_state.skip_gcode = false;
                continue;
            }

            clear_media_error();

            print_state.media_restore_info = data.replay_pos.restore_info;
            queue.sdpos = data.replay_pos.offset;
            if (!queue.enqueue_one(data.gcode.data(), false)) {
                bsod("enqueue_one failed");
            }
            log_debug(MarlinServer, "Enqueue: %" PRIu32 " %s", data.replay_pos.offset, data.gcode.data());

            // Issue another fetch if the media prefetch buffer is running empty
            if (metrics.buffer_occupancy_percent < 60 && metrics.tail_status != Status::end_of_file) {
                media_prefetch.issue_fetch();
            }

            if (data.cropped) {
                set_warning(WarningType::GcodeCropped);
            }

            break;

        case Status::end_of_file:
            clear_media_error();

            // We've read everything -> start finishing up the print, return from this function completely
            server.print_state = State::Finishing_WaitIdle;
            return;

        case Status::end_of_buffer:
            // Defnitely issue a prefetch here
            media_prefetch.issue_fetch();
            return;

        case Status::usb_error:
        case Status::corruption:
        case Status::not_downloaded: {
            if (status.fetch_active) {
                // There's still a fetch running, this isn't completely final ‒ the
                // fetch itself can recover from the error (and sometimes it does,
                // but the actual recovery takes time). Wait for the final verdict.
                return;
            }

            set_warning(*prefetch_status_to_warning(status.status));
            schedule_media_retry();
            print_pause();
            return;
        }
        }
    }
}

/// Update SFN filepath from LFN.
/// The SFN of the file could have been changed by the user during the pause (for example by re-uploading a damaged file).
/// BFW-5775
void update_sfn() {
    // Put into one struct so that we can squeeze it through a std::inplace_function capture
    struct {
        MutablePath filepath_sfn;
        const char *lfn;
        bool found = false;
    } d;

    // Copy the current SFN + LFN from marlin vars
    marlin_vars().media_SFN_path.copy_to(d.filepath_sfn.get_buffer(), d.filepath_sfn.maximum_length());
    log_info(MarlinServer, "Old SFN: %s", d.filepath_sfn.get());

    // Pop filename, leave path only
    d.filepath_sfn.pop();

    // This is done on the marlin thread, so we can keep using the pointer
    d.lfn = marlin_vars().media_LFN.get_ptr();

    // Do this in the async job thread to prevent blocking Marlin on I/O and possibly causing a watchdog reset
    AsyncJob async_job;
    async_job.issue([&d](AsyncJobExecutionControl &) {
        Directory dir { d.filepath_sfn.get() };
        if (!dir) {
            return;
        }

        struct dirent *ent;
        while ((ent = dir.read())) {
            if ((strcasecmp(ent->d_name, d.lfn) == 0) || (strcasecmp(ent->lfn, d.lfn) == 0)) {
                break;
            }
        }

        if (!ent) {
            return;
        }

        d.found = true;
        d.filepath_sfn.push(ent->d_name);
    });

    while (async_job.is_active()) {
        ::idle(true);
    }

    // We haven't found the file -> do nothing. Fail open is sorted out later in the code.
    if (!d.found) {
        return;
    }

    // Update the relevant variables
    log_info(MarlinServer, "New SFN: %s", d.filepath_sfn.get());
    marlin_vars().media_SFN_path.set(d.filepath_sfn.get());
    GCodeInfo::getInstance().set_gcode_file(d.filepath_sfn.get(), d.lfn);

#if HAS_POWER_PANIC()
    power_panic::refresh_sfn();
#endif
}

void print_resume(void) {
    if (server.print_state == State::Paused) {
        update_sfn();

        if (server.print_is_serial) {
            server.print_state = State::Resuming_Begin;
        } else {
            server.print_state = State::Resuming_BufferData;
            media_prefetch_start();
        }

        // pause queuing commands from serial, until resume sequence is finished.
        GCodeQueue::pause_serial_commands = true;

    } else if (is_resuming_state(server.print_state)) {
        // Do nothing

    } else if (is_pausing_state(server.print_state)) {
        print_state.resume_pending = true;

#if HAS_POWER_PANIC()
    } else if (server.print_state == State::PowerPanic_AwaitingResume) {
        power_panic::resume_continue();
        server.print_state = State::PowerPanic_Resume;
#endif
    } else {
        print_start(marlin_vars().media_SFN_path.get_ptr(), GCodeReaderPosition(), marlin_server::PreviewSkipIfAble::all);
    }
}

void try_recover_from_media_error() {
    if (server.print_state == State::Printing) {
        // If we're printing, simply try issuing a fetch to make sure everything's fine
        media_prefetch.issue_fetch();

    } else if (server.print_state == State::Paused && print_state.recover_media_error_backoff.get().has_value()) {
        // Do NOT reset - will be reset if the resume is successful
        // print_state.recover_media_error_backoff.get().reset();
        server.print_state = State::MediaErrorRecovery_BufferData;
        update_sfn();
        media_prefetch_start();

    } else {
        // We cannot attempt recovery right now, but recover_media_error_backoff should make us retry sometime later
    }
}

#if HAS_POWER_PANIC()
void powerpanic_resume(const char *media_SFN_path, const GCodeReaderPosition &resume_pos, bool auto_recover) {
    print_start(media_SFN_path, resume_pos, marlin_server::PreviewSkipIfAble::all);
    crash_s.set_state(Crash_s::PRINTING);

    // open printing screen
    fsm_create(PhasesPrinting::active);

    // Warn user of possible print fail caused by cold heatbed during PP
    if (!auto_recover) {
        set_warning(WarningType::HeatbedColdAfterPP);
    }

    // enter the main powerpanic resume loop
    server.print_state = auto_recover ? State::PowerPanic_Resume : State::PowerPanic_AwaitingResume;
    METRIC_DEF(power, "power_panic", METRIC_VALUE_EVENT, 0, METRIC_ENABLED);
    metric_record_event(&power);
}

void powerpanic_finish_recovery() {
    // WARNING: this sequence needs to _just_ set the server state and exit
    // perform any higher-level operation inside power_panic::atomic_finish

    // setup for replay and start recovery
    crash_s.set_state(Crash_s::RECOVERY);
    server.print_state = State::Resuming_UnparkHead_ZE;
}

void powerpanic_finish_pause() {
    // WARNING: this sequence needs to _just_ set the server state and exit
    // perform any higher-level operation inside power_panic::atomic_finish

    // restore leveling state and planner position (mind the order!)
    planner.leveling_active = crash_s.leveling_active;
    current_position = crash_s.start_current_position;
    planner.set_position_mm(current_position);
    server.print_state = State::Paused;
}

    #if HAS_TOOLCHANGER()
void powerpanic_finish_toolcrash() {
    // WARNING: this sequence needs to _just_ set the server state and exit
    // perform any higher-level operation inside power_panic::atomic_finish

    // Restore leveling state, do not tweak planner position manually as leveling was off when the panic happened
    set_bed_leveling_enabled(crash_s.leveling_active);

    // Go through ToolchangePowerPanic to set up the toolchanger correctly
    crash_s.set_state(Crash_s::REPEAT_WAIT);
    server.print_state = State::CrashRecovery_ToolchangePowerPanic;
}
    #endif
    #if HAS_INDX()
void powerpanic_finish_indx_toolchange() {
    // WARNING: this sequence needs to _just_ set the server state and exit;
    // perform any higher-level operation inside power_panic::atomic_finish

    // Restore leveling state, do not tweak planner position manually as leveling was off when the panic happened
    set_bed_leveling_enabled(crash_s.leveling_active);

    crash_s.set_state(Crash_s::REPEAT_WAIT);
    server.print_state = State::PowerPanic_FinishIndxToolchange;
}
    #endif
#endif

#if ENABLED(AXIS_MEASURE)
enum class Axis_length_t {
    shorter,
    longer,
    ok,
};

static Axis_length_t axis_length_ok(AxisEnum axis) {
    #if HAS_SELFTEST()
    const float len = server.axis_length[axis];

    switch (axis) {
    case X_AXIS:
        return len < selftest::Config_XAxis.length_min ? Axis_length_t::shorter : (len > selftest::Config_XAxis.length_max ? Axis_length_t::longer : Axis_length_t::ok);
    case Y_AXIS:
        return len < selftest::Config_YAxis.length_min ? Axis_length_t::shorter : (len > selftest::Config_YAxis.length_max ? Axis_length_t::longer : Axis_length_t::ok);
    default:;
    }
    return Axis_length_t::shorter;
    #else
    // #error dead code found by automatic analyses (see BFW-5461)
    return Axis_length_t::ok;
    #endif // HAS_SELFTEST
}

/// \returns true if X and Y axes have correct lengths.
/// You have to measure the length of the axes before this.
static Axis_length_t xy_axes_length_ok() {
    Axis_length_t alx = axis_length_ok(X_AXIS);
    Axis_length_t aly = axis_length_ok(Y_AXIS);
    if (alx == aly && aly == Axis_length_t::ok) {
        return Axis_length_t::ok;
    }
    // shorter is worse than longer
    if (alx == Axis_length_t::shorter || aly == Axis_length_t::shorter) {
        return Axis_length_t::shorter;
    }
    return Axis_length_t::longer;
}

static SelftestSubtestState_t axis_length_check(AxisEnum axis) {
    return axis_length_ok(axis) == Axis_length_t::ok ? SelftestSubtestState_t::ok : SelftestSubtestState_t::not_good;
}

/// Sets lengths of axes to "by-pass" xy_axes_length_ok()
static void axes_length_set_ok() {
    server.axis_length[X_AXIS] = (selftest::Config_XAxis.length_min + selftest::Config_XAxis.length_max) / 2;
    server.axis_length[Y_AXIS] = (selftest::Config_YAxis.length_min + selftest::Config_YAxis.length_max) / 2;
}

void set_axes_length(xy_float_t xy) {
    server.axis_length = xy;
}
#endif // ENABLED(AXIS_MEASURE)

// Checking valid behaviour of Heatbreak fan & Print fan of currently active extruder/tool
bool active_extruder_fan_checks() {
    auto tool = stdext::get_optional<PhysicalToolIndex>(PhysicalToolIndex::currently_selected());
    if (!tool.has_value() || !tool->is_enabled() || !marlin_vars().fan_check_enabled) {
        return false;
    }

    auto check_fan = [](CFanCtlCommon &fan, const char *fan_name) {
        if (!fan.is_fan_ok()) {
            log_error(MarlinServer, "%s FAN RPM is not OK - Actual: %d rpm, PWM: %d",
                fan_name,
                (int)fan.get_actual_rpm(),
                (int)fan.get_pwm());
            return true;
        }
        return false;
    };

    bool fan_failed = false;
#if !PRINTER_IS_PRUSA_iX()
    fan_failed |= check_fan(Fans::heat_break(*tool), "Heatbreak");
#endif
    fan_failed |= check_fan(Fans::print(*tool), "Print");
    return fan_failed;
}

static void resuming_reheating() {
    buddy::safety_timer().reset_restore_nonblocking();

    if (hotendErrorChecker.isFailed()) {
        set_warning(WarningType::HotendTempDiscrepancy);
        thermalManager.setTargetHotend(0, 0);
        thermalManager.set_print_fan_speed(255);
        server.print_state = State::Paused;
        return;
    }

    if (active_extruder_fan_checks()) {
        server.print_state = State::Paused;
        return;
    }

    // Check if nozzles are being reheated
    for (auto tool : PhysicalToolIndex::all()) {
        if (Temperature::degTargetHotend(tool) != server.resume.nozzle_temp[tool]) {
            // Stopped reheating, can happen if there is an error during reheating
            server.print_state = State::Paused;
            return;
        }
    }

    if (!Temperature::are_all_temperatures_reached()) {
        return;
    }

#if HAS_CRASH_DETECTION()
    // GCodeInterrupt uses crash recovery mechanism
    // Crash recovery goes through recovering -> pause -> resuming phase
    // So this is the right moment to enqueue and execute the interrupt gcode.
    if (!print_state.gcode_interrupt_command.is_empty()) {
        enqueue_gcode("M9944");
    }
#endif

    server.print_state = State::Resuming_ExecutingGCodeInterrupt;
}

static void _server_print_loop(void) {
    static bool did_not_start_print = true, abort_resuming = false;
    switch (server.print_state) {
    case State::Idle:
        break;
    case State::PrintPreviewInit:
        did_not_start_print = true;
        // reset both percentage counters (normal and silent)
        oProgressData.standard_mode.percent_done.mSetValue(0, 0);
        oProgressData.stealth_mode.percent_done.mSetValue(0, 0);
        PrintPreview::Instance().Init();
        server.print_state = State::PrintPreviewImage;
#if HAS_E2EE_SUPPORT()
        // remove any left over tmp trusted identities
        e2ee::remove_temporary_identites();
#endif
        break;

    case State::PrintPreviewImage:
    case State::PrintPreviewConfirmed:
#if HAS_TOOL_MAPPING()
    case State::PrintPreviewToolsMapping:
#endif
    case State::PrintPreviewQuestions: {
        // button evaluation
        // We don't particularly care about the
        // difference, but downstream users do.
        switch (PrintPreview::Instance().Loop()) {

        case PrintPreview::Result::Wait:
            break;

        case PrintPreview::Result::MarkStarted:
            // The job_id is used to identify a job for Connect & Link. We want to
            // have a unique one for each job, but have the same one through the
            // whole job. From UI perspective, the questions about filament /
            // printer type / etc are already part of the job (there's a preview in
            // Connect for whatever is being printed).

            // First, reserve the job_id in eeprom. In case we get reset, we need
            // that to not get reused by accident.
            config_store().job_id.set(job_id + 1);
            // And increment the job ID before we actually stop printing.
            job_id++;
            // Reset "time to" and percents before asking questions to "unknown"
            oProgressData.mInit();

            server.print_state = State::PrintPreviewConfirmed;
            break;

        case PrintPreview::Result::Image:
            server.print_state = State::PrintPreviewImage;
            break;

        case PrintPreview::Result::Questions:
            server.print_state = State::PrintPreviewQuestions;
            break;

        case PrintPreview::Result::Abort:
            if (did_not_start_print) {
                // Saving the result for connect, we already send the job id to them at this point.
                marlin_vars().add_job_result(job_id, marlin_vars_t::JobInfo::JobResult::aborted);
                cleanup_print();

            } else {
                server.print_state = State::Finishing_WaitIdle;
            }
            break;

#if HAS_TOOL_MAPPING()
        case PrintPreview::Result::ToolsMapping:
            server.print_state = State::PrintPreviewToolsMapping;
            break;
#endif

        case PrintPreview::Result::Print:
        case PrintPreview::Result::Inactive:
            did_not_start_print = false;
            server.print_state = State::PrintInit;
            break;
        }

        break;
    }

    case State::PrintInit:
    case State::SerialPrintInit:
        if (idle_running || is_processing()) {
            // Prints must always be started from outer loop
            break;
        }

        server.print_is_serial = (server.print_state == State::SerialPrintInit);
        server.was_print_time_saved = false;
#if HAS_WASTEBIN_FILL_TRACKING()
        // Fresh print: reset the per-print pellet/toolchange progress counter.
        WastebinWatcher::instance().reset_print_progress();
#endif
#if HAS_MMU2()
        server.mmu_maintenance_checked = false;
#endif
        planner.max_printed_z = 0;

        if (!server.print_is_serial) {
            feedrate_percentage = 100;

            // Reset flow factor for all extruders
            for (auto tool : VirtualToolIndex::all()) {
                planner.flow_percentage[tool] = 100;
                planner.refresh_e_factor(tool);
            }
        }

#if HAS_TOOL_MAPPING() && (HOTENDS > 1)
        if (!server.print_is_serial) {
            // Cooldown unused tools
            // Ignore spool join - spool joined tools will get heated as spool join is activated
            // BFW-5996
            for (uint8_t physical_tool = 0; physical_tool < PhysicalToolIndex::count; physical_tool++) {
                if (tool_mapper.to_gcode(physical_tool) == tools_mapping::no_tool) {
                    thermalManager.setTargetHotend(0, physical_tool);
                }
            }
        }
#endif

#if HAS_CRASH_DETECTION()
        crash_s.reset();
        crash_s.counters.reset();
        endstops.enable_globally(true);

        // Crash Detection is disabled during serial printing, because it does not work
        if (!server.print_is_serial) {
            crash_s.set_state(Crash_s::PRINTING);
        }
#endif

#if HAS_CEILING_CLEARANCE()
        buddy::reenable_ceiling_clearance_warning();
#endif

#if HAS_INDX()
        prusa_toolchanger.set_nozzle_check_disabled(false);
#endif

#if HAS_CANCEL_OBJECT()
        buddy::cancel_object().reset();
        for (auto &cancel_object_name : marlin_vars().cancel_object_names) {
            cancel_object_name.set(""); // Erase object names
        }
#endif

#if HAS_LOADCELL()
        if (!server.print_is_serial) {
            // Reset Live-Adjust-Z value before every print
            probe_offset.z = 0;
            marlin_vars().z_offset = 0;
        }
#endif // HAS_LOADCELL()

#if HAS_TOOLCHANGER()
        // Singletool printers have no docks, so the dock positions are never
        // calibrated and get_tool_dock_position() would red-screen on its
        // calibration check.
        if (prusa_toolchanger.is_toolchanger_enabled()) {
            METRIC_DEF(metric_dock_position, "dock_pos", METRIC_VALUE_CUSTOM, 0, METRIC_ENABLED);
            for (auto tool : PhysicalToolIndex::all().skip_all_disabled()) {
                const xy_float_t pos = prusa_toolchanger.get_tool_dock_position(tool, false);
                metric_record_custom(&metric_dock_position, ",tool=%u x=%.3f,y=%.3f", static_cast<unsigned int>(tool.to_raw()), (double)pos.x, (double)pos.y);
            }
        }
#endif

        print_job_timer.start();
        marlin_vars().print_start_time = time(nullptr);

        if (!server.print_is_serial) {
            marlin_vars().time_to_end = TIME_TO_END_INVALID;
            marlin_vars().time_to_pause = TIME_TO_END_INVALID;
        }

        server.print_state = State::Printing;

#if HAS_SERIAL_PRINT()
        if (server.print_is_serial) {
            fsm_create(PhasesSerialPrinting::active);
        } else
#endif
        {

            if (fsm_states.is_active(ClientFSM::PrintPreview)) {
                fsm_destroy_and_create(ClientFSM::PrintPreview, ClientFSM::Printing, fsm::BaseData());
            }
            if (!fsm_states.is_active(ClientFSM::Printing)) {
                // FIXME make this atomic change. It would require improvements in PrintScreen so that it can re-initialize upon phase change.
                // FYI the DESTROY invoke is in print_start()
                fsm_create(PhasesPrinting::active);
            }
        }
#if HAS_CHAMBER_VENTS()
        {
            // Find the highest chamber target temperature across the filaments used by the print
            std::optional<uint8_t> max_chamber_target_temp;
            GCodeInfo::getInstance().for_each_used_extruder([&]([[maybe_unused]] GcodeToolIndex, VirtualToolIndex virtual_tool, const GCodeInfo::ExtruderInfo &) {
                const auto target = FilamentType::for_tool_heuristic(virtual_tool).parameters().chamber_target_temperature;
                if (target.has_value() && (!max_chamber_target_temp.has_value() || *target > *max_chamber_target_temp)) {
                    max_chamber_target_temp = target;
                }
            });
            buddy::chamber().manage_ventilation_state(max_chamber_target_temp);
        }
#endif
#if HAS_CHAMBER_FILTRATION_API()
        buddy::chamber_filtration().check_filter_expiration();
#endif
#if XBUDDY_EXTENSION_VARIANT_IS_STANDARD()
        buddy::xbuddy_extension().set_chamber_regulator_legacy(true); // For compatibility with old gcodes on coreone
#endif

        if (!server.print_is_serial) {
            // TODO: Fix them enqueue hacks

#if HAS_TOOLCHANGER()
            if (prusa_toolchanger.is_toolchanger_enabled()) {
                // Handle singletool G-code which doesn't have T commands in it
                if (GCodeInfo::getInstance().is_singletool_gcode()) {
                    enqueue_gcode("T0 S1 D0"); // Pick tool 0 (remapped by stateFromPrinterCheck if needed)
                }
            }
#endif
#if HAS_MMU2()
            if (MMU2::mmu2.Enabled() && GCodeInfo::getInstance().is_singletool_gcode() && MMU2::mmu2.get_current_tool() == MMU2::FILAMENT_UNKNOWN) {
                // POC: Handle singletool G-code which doesn't have T commands in it
                // In case we don't have other filament loaded!
                // Unfortunately we don't have the nozzle heated, an ugly workaround is to enqueue an M109 :(

                const uint16_t preheat_temp = GCodeInfo::getInstance().get_hotend_preheat_temp().value_or(215U);
                enqueue_gcode_printf("M109 S%" PRIu16, preheat_temp); // speculatively, use PLA temp for MMU prints, anything else is highly unprobable at this stage
                enqueue_gcode("T0"); // tool change T0 (can be remapped to anything)
                enqueue_gcode("G92 E0"); // reset extruder position to 0

                bool is_relative = gcode.axis_is_relative(AxisEnum::E_AXIS);

                enqueue_gcode("M82"); // set E to absolute positions
    #if HAS_NEXTRUDER()
                enqueue_gcode("G1 E25 F1860"); // push filament into the nozzle - load distance from fsensor into nozzle tuned (hardcoded) for now
                enqueue_gcode("G1 E35 F300"); // slowly push another 10mm (absolute E)
    #elif PRINTER_IS_PRUSA_MK3_5()
                enqueue_gcode("G1 E50 F1860"); // push filament into the nozzle - load distance from fsensor into nozzle tuned (hardcoded) for now
                enqueue_gcode("G1 E62 F300"); // slowly push another 12mm (absolute E)
    #else
        #error
    #endif
                if (is_relative) {
                    enqueue_gcode("M83"); // set E back to relative positions
                }

                // In case of need, we can perform a custom purge line from the other end of the heatbed
                // It would require homing the axes first, moving to [maxx-10, -4] and slowly purging while moving towards the origin
            }
#endif
        }
        break;

    case State::Printing:
        print_state.resume_pending = false;

#if HAS_SERIAL_PRINT()
        if (server.print_is_serial) {
            SerialPrinting::print_loop();
        } else
#endif
        {
            media_print_loop();
        }
        break;

    case State::Pausing_Begin:
        if (idle_running) {
            // Do not change state in the middle of a gcode, only in the outer loop
            break;
        }

        process_pausing_begin_state();
        server.print_state = State::Pausing_WaitIdle;
        break;

    case State::Pausing_WaitIdle:
        if (!is_processing()) {
            server.resume.pos = current_position;
            retract_and_lift();
            park_head(true);
            server.print_state = State::Pausing_ParkHead;
        }
        break;
    case State::Pausing_ParkHead:
        if (!planner.processing()) {
#if HAS_INDX()
            // Park the tool to its dock after pausing
            tool_change(NoTool {}, tool_return_t::no_return);
#endif
            server.print_state = State::Paused;
        }
        break;
    case State::Paused:
        // resume queuing serial commands (to be able to resume)
        GCodeQueue::pause_serial_commands = false;

        if (print_state.resume_pending) {
            print_state.resume_pending = false;
            print_resume();
        } else if (print_state.recover_media_error_at.has_value() && ticks_diff(*print_state.recover_media_error_at, ticks_s()) <= 0) {
            log_info(MarlinServer, "Try recover from media error");
            print_state.recover_media_error_at.reset();
            try_recover_from_media_error();
            // Ensure we do try to unpause here.
            debug_assert(server.print_state != State::Paused);
        }

        break;
    case State::Resuming_BufferData:
    case State::MediaErrorRecovery_BufferData: {
        const auto metrics = media_prefetch.get_metrics();
        if (metrics.is_fetching) {
            // Wait till the media prefetch finishes
            break;
        }

        using Status = MediaPrefetchManager::Status;

        switch (metrics.tail_status) {

        case Status::ok:
        case Status::end_of_file:
        case Status::end_of_buffer:
            // The media_prefetch feched something successfully, let's continue with resuming!
            server.print_state = State::Resuming_Begin;
            clear_media_error();
            break;

        case Status::usb_error:
        case Status::corruption:
        case Status::not_downloaded: {
            // Still failing.
            schedule_media_retry();
            media_prefetch.stop();

            // Show a warning, but only if the unpause was requested by the user explicitly
            // Do not spam warnings when we're doing background periodic media error recoveries
            if (server.print_state != State::MediaErrorRecovery_BufferData) {
                set_warning(*prefetch_status_to_warning(metrics.tail_status));
            }

            // Go back to Paused (the only state where we could have come from).
            server.print_state = State::Paused;
        }
        }
        break;
    }
    case State::Resuming_Begin:
#if HAS_CRASH_DETECTION()
    #if ENABLED(AXIS_MEASURE)
        if (crash_s.is_repeated_crash() && xy_axes_length_ok() != Axis_length_t::ok) {
            /// resuming after a crash but axes are not ok => check again
            fsm_create(PhasesCrashRecovery::check_X);
            measure_axes_and_home();
            break;
        }
    #endif

        // forget the XYZ resume position if requested
        if (!(crash_s.recover_flags & Crash_s::RECOVER_XY_POSITION)) {
            LOOP_XY(i) {
                server.resume.pos[i] = current_position[i];
            }
        }
        if (!(crash_s.recover_flags & Crash_s::RECOVER_Z_POSITION)) {
            server.resume.pos[Z_AXIS] = current_position[Z_AXIS];
        }
#endif
        resuming_begin();
        break;

    case State::Resuming_Reheating:
        resuming_reheating();
        break;

    case State::Resuming_ExecutingGCodeInterrupt: {
        if (is_processing()) {
            break;
        }

        release_assert(print_state.gcode_interrupt_command.is_empty());

#if HAS_CRASH_DETECTION()
        if (crash_s.get_state() == Crash_s::REPEAT_WAIT) {
            server.print_state = State::Resuming_UnparkHead_ZE; // Skip unpark when recovering from toolcrash or homing fail
            return;
        }
#endif

#if HAS_NOZZLE_CLEANER()
        // Prime in nozzle cleaning area if available.
    #if HAS_CRASH_DETECTION()
        // trigger when clicking "Resume" after pause
        if ((crash_s.get_state() == Crash_s::PRINTING) ||
            // trigger when tool was lost
            (crash_s.get_state() == Crash_s::RECOVERY && crash_s.is_toolchange_event())) {
            unpark_prime();
        }
    #else // HAS_CRASH_DETECTION()
        unpark_prime();
    #endif // #else // HAS_CRASH_DETECTION()
#endif // HAS_NOZZLE_CLEANER()

        unpark_head_XY();
        server.print_state = State::Resuming_UnparkHead_XY;
        break;
    }

    case State::Resuming_UnparkHead_XY:
        if (is_processing()) {
            break;
        }
        unpark_head_ZE();
        server.print_state = State::Resuming_UnparkHead_ZE;
        break;

    case State::Resuming_UnparkHead_ZE:
        if (is_processing()) {
            break;
        }

#if HAS_CRASH_DETECTION()
        if (crash_s.get_state() == Crash_s::RECOVERY) {
            endstops.enable_globally(true);
            crash_s.set_state(Crash_s::REPLAY);
        } else if (crash_s.get_state() == Crash_s::REPEAT_WAIT) {
            endstops.enable_globally(true);
            crash_s.set_state(Crash_s::PRINTING); // Coming from toolcrash or homing fail, no replay
        } else {
            // UnparkHead can be called after a pause, in which case crash handling should already
            // be active and we don't need to change any other setting

            // Crash Detection is disabled during serial printing, because it does not work
            debug_assert(server.print_is_serial || crash_s.get_state() == Crash_s::PRINTING);
        }
#endif
        if (abort_resuming) {
            server.print_state = State::Pausing_WaitIdle;
            abort_resuming = false;
            break;
        }
        // server.motion_param.load();  // TODO: currently disabled (see Crash_s::save_parameters())
        if (print_job_timer.isPaused()) {
            print_job_timer.start();
        }
        thermalManager.set_print_fan_speed(server.resume.fan_speed); // restore fan speed
        feedrate_percentage = server.resume.print_speed;
#if HAS_SERIAL_PRINT()
        SerialPrinting::resume();
#endif
        server.print_state = State::Printing;
        break;

    case State::Aborting_Begin:
#if HAS_CRASH_DETECTION()
        if (crash_s.is_toolchange_in_progress()) {
            break; // Wait for toolchange to end
        }
#endif
        if (marlin_vars().gcode_command.get() == Cmd::G28) {
            break; // Wait for homing to end
        }

        // Unstuck any operation that is skippable
        skippable_gcode().request_skip();

        media_prefetch.stop();
        queue.clear();

        print_job_timer.stop();
        planner.quick_stop();
        wait_for_heatup = false; // This is necessary because M109/wait_for_hotend can be in progress, we need to abort it

#if HAS_CRASH_DETECTION()
        // TODO: the following should be moved to State::Aborting_ParkHead once the "stopping"
        // state is handled properly
        endstops.enable_globally(false);
        crash_s.counters.save_to_eeprom();
        server.aborting_did_crash_trigger = crash_s.did_trigger(); // Remember as it is cleared by crash_s.reset()
        crash_s.reset();
#endif

        server.print_state = State::Aborting_WaitIdle;
        break;
    case State::Aborting_WaitIdle:
        if (is_processing()) {
            break;
        }

        // allow movements again
        planner.resume_queuing();
#if HAS_SERIAL_PRINT()
        if (server.print_is_serial) {
            // will enqueue gcode that will send abort to print host
            SerialPrinting::abort();
        }
#endif
        set_current_from_steppers();
        sync_plan_position();
        // Note technically Z position is also wrong after the quick_stop(),
        // but Z is not moved often and moves slower, so the position should be
        // quite accurate
        set_axis_is_not_at_home(X_AXIS);
        set_axis_is_not_at_home(Y_AXIS);

#if HAS_EMERGENCY_STOP()
        if (!buddy::emergency_stop().in_emergency()) {
#else
        {
#endif
            retract_and_lift();
            // Skip homing and parking when no tool is picked - there's no nozzle to clean or park,
            // and pre_finalize_print's tool_change(NoTool) is a no-op in that case.
            if (std::holds_alternative<PhysicalToolIndex>(PhysicalToolIndex::currently_selected())) {
#if HAS_NOZZLE_CLEANER()
                // With nozzle cleaner, home so that the head position is known for parking and nozzle cleaning.
                // On INDX, home precisely so that finalize_print's tool_change(NoTool) docking can skip its own homing.
                GcodeSuite::G28_no_parser(true, true, false, { .z_raise = 0, .can_calibrate = false, .precise = HAS_INDX() });
#endif
                park_head(false);
            }
        }

        thermalManager.disable_all_heaters();
        thermalManager.set_print_fan_speed(0);
        server.print_state = State::Aborting_UnloadFilament;
        break;

    case State::Aborting_UnloadFilament:
        if (is_processing()) {
            break;
        }

        if (!pre_finalize_print(false)) {
            break;
        };
        server.print_state = State::Aborting_ParkHead;
        break;
    case State::Aborting_ParkHead:
        if (!is_processing()) {
#ifndef Z_ALWAYS_ON
            disable_Z();
#endif // Z_ALWAYS_ON
            server.print_state = State::Aborted;
            // Don't clean up here - it will be done in State::Exit
            finalize_print(false);
        }
        break;
    case State::Aborting_Preview:
        // Wait for operations to finish
        if (is_processing()) {
            break;
        }

#if HAS_TOOL_MAPPING()
        if (PrintPreview::Instance().GetState() == PrintPreview::State::tools_mapping_wait_user) {
            PrintPreview::tools_mapping_cleanup();
        }
#endif

        cleanup_print();
        break;

    case State::Finishing_WaitIdle:
        if (!is_processing()) {
#if HAS_CRASH_DETECTION()
            // TODO: the following should be moved to State::Finishing_ParkHead once the "stopping"
            // state is handled properly
            endstops.enable_globally(false);
            crash_s.counters.save_to_eeprom();
            crash_s.reset();
#endif

            // ! Must be before the park_head(), otherwise the head parking is still considered a print state
            server.print_state = State::Finishing_UnloadFilament;

#ifdef PARK_HEAD_ON_PRINT_FINISH
            if (!server.print_is_serial) {
                retract_and_lift();
                park_head(false);
            }
#endif // PARK_HEAD_ON_PRINT_FINISH
        }
        break;
    case State::Finishing_UnloadFilament:
        if (is_processing()) {
            break;
        }

        if (!pre_finalize_print(true)) {
            break;
        };
        server.print_state = State::Finishing_ParkHead;
        break;
    case State::Finishing_ParkHead:
        if (!is_processing()) {
            server.print_state = State::Finished;
            // Don't clean up here - it will be done in State::Exit
            finalize_print(true);
        }
        break;
    case State::Exit:
        if (idle_running) {
            // Do not change state in the middle of a gcode, only in the outer loop
            break;
        }

        cleanup_print();
        break;

#if HAS_CRASH_DETECTION()
    case State::CrashRecovery_Begin: {
        // pause and set correct resume position: this will stop media reading and clear the queue
        // TODO: this is completely broken for crashes coming from serial printing
        process_pausing_begin_state(Pause_Type::Crash);
        set_media_position(crash_s.sdpos);

        const auto orig_crash_state = crash_s.get_state();

        endstops.enable_globally(false);
        crash_s.send_reports();

        if (orig_crash_state != Crash_s::TRIGGERED_GCODE_INTERRUPT) {
            crash_s.count_crash();
        }

        if (orig_crash_state == Crash_s::TRIGGERED_TOOLCRASH || orig_crash_state == Crash_s::TRIGGERED_HOMEFAIL) {
            crash_s.set_state(Crash_s::REPEAT_WAIT);
        } else {
            crash_s.set_state(Crash_s::RECOVERY);
        }

        static constexpr Crash_recovery_fsm cr_fsm(SelftestSubtestState_t::running, SelftestSubtestState_t::undef);

        /**
         * Unreadable switch with 4 posibilites:
         *
         * HAS_TOOLCHANGER() && ENABLED(AXIS_MEASURE)
         * if {toolchange} -> else if {home} -> else if {axis_measure} -> else {crash}
         *
         * HAS_TOOLCHANGER() && !ENABLED(AXIS_MEASURE)
         * if {toolchange} -> else if {home} -> else {crash}
         *
         * !HAS_TOOLCHANGER() && ENABLED(AXIS_MEASURE)
         * if {home} -> else if {axis_measure} -> else {crash}
         *
         * !HAS_TOOLCHANGER() && !ENABLED(AXIS_MEASURE)
         * if {home} -> else {crash}
         *
         * Allways exactly one crash_recovery_begin_~~~() is called.
         * Each of them calls fsm_create() exactly once.
         */
        if (0) {
        } // dummy if to start with else

    #if HAS_TOOL_CRASH_RECOVERY()
        else if (crash_s.is_toolchange_event()) {
            if (crash_recovery_begin_toolchange()) {
                break; // Skip crash recovery and go directly to toolchange
            }
        }
    #endif

        else if (crash_s.get_state() == Crash_s::REPEAT_WAIT) { // REPEAT_WAIT could be toolfall, but it was handled above
            fsm_create(PhasesCrashRecovery::home, cr_fsm.Serialize());
            measure_axes_and_home(); // If crash happens during homing, skip crash recovery and go directly to measuring axes / homing
            break; // Skip crash recovery and go directly to homing
        }

    #if ENABLED(AXIS_MEASURE)
        else if (crash_s.is_repeated_crash()) {
            fsm_create(PhasesCrashRecovery::check_X, cr_fsm.Serialize()); // check axes first
        }
    #endif /*ENABLED(AXIS_MEASURE)*/

        else if (orig_crash_state == Crash_s::TRIGGERED_GCODE_INTERRUPT) {
            fsm_create(PhasesCrashRecovery::home_gcode_interrupt, cr_fsm.Serialize());
        }

        else { // All toolfalls, crashes and homing fails are handled above, only regular crash remains
            fsm_create(PhasesCrashRecovery::home, cr_fsm.Serialize());
        }

        // save the current resume position
        server.resume.pos = current_position;

    #if HAS_PAUSE()
        /// retract and save E stepper position
        retract();
    #endif

        server.print_state = State::CrashRecovery_Retracting;
        break;
    }
    #if HAS_TOOL_CRASH_RECOVERY()
    case State::CrashRecovery_ToolchangePowerPanic: {
        // server.resume.nozzle_temp is already configured by powerpanic
        endstops.enable_globally(false);
        crash_recovery_begin_toolchange(); // Also sets server.print_state
        break;
    }
    #endif
    #if HAS_INDX()
    case State::PowerPanic_FinishIndxToolchange: {
        endstops.enable_globally(false);
        const power_panic::state_toolchanger_t tc = power_panic::state_buf.toolchanger;
        const std::variant<PhysicalToolIndex, NoTool> active_tool
            = PhysicalToolIndex::from_raw_notool(power_panic::state_buf.planner.active_tool);
        const PrusaToolChanger::ToolchangeReturnData rd {
            .tool = PhysicalToolIndex::from_raw_notool(tc.tool_nr),
            .return_type = tc.return_type,
            .return_pos = tc.return_pos,
        };
        // The bool result is discarded; failure is handled downstream by resuming_begin(),
        // which reapplies heater targets and calls tool_change(server.resume.active_tool,
        // no_return) as a fallback pickup. sdpos still points at T<N>, so gcode replay
        // re-executes it once printing resumes.
        // NOTE: endstops.enable_globally(false) above remains in effect through
        // resuming_begin(); a fallback tool_change() that needs G28 would silently fail to home.
        (void)prusa_toolchanger.recover_pp_toolchange(rd, active_tool, tc.phase);
        server.print_state = State::Resuming_Begin;
        break;
    }
    #endif
    case State::CrashRecovery_Retracting: {
        if (planner.processing()) {
            break;
        }

        lift_head();
        server.print_state = State::CrashRecovery_Lifting;
        break;
    }
    case State::CrashRecovery_Lifting: {
        if (planner.processing()) {
            break;
        }

    #if HAS_TOOL_CRASH_RECOVERY()
        if (crash_s.is_toolchange_event()) {
            prepare_tool_pickup(); // Go to tool pickup instead of homing
            break;
        }
    #endif

        measure_axes_and_home();
        break;
    }
    case State::CrashRecovery_XY_Measure: {
        if (is_processing()) {
            break;
        }

    #if ENABLED(AXIS_MEASURE)
        METRIC_DEF(crash_len, "crash_length", METRIC_VALUE_CUSTOM, 0, METRIC_ENABLED);
        metric_record_custom(&crash_len, " x=%.3f,y=%.3f", (double)server.axis_length[X_AXIS], (double)server.axis_length[Y_AXIS]);
    #endif

        set_axis_is_not_at_home(X_AXIS);
        set_axis_is_not_at_home(Y_AXIS);
        server.print_state = State::CrashRecovery_XY_HOME;
        break;
    }
    #if HAS_TOOL_CRASH_RECOVERY()
    case State::CrashRecovery_Tool_Pickup: {
        if (is_processing()) {
            break;
        }

        crash_recovery_tool_pickup();
        break;
    }
    #endif
    case State::CrashRecovery_XY_HOME: {
        if (is_processing()) {
            break;
        }

        // TODO: this doesn't respect Crash_s::REPLAY_NONE which should prevent re-home as well
        if (axis_unhomed_error(_BV(X_AXIS) | _BV(Y_AXIS)
                | (crash_s.is_homefail_z() ? _BV(Z_AXIS) : 0))) { // Needs homing
            TemporaryBedLevelingState tbs(false); // Disable for the additional homing, keep previous state after homing
            if (!GcodeSuite::G28_no_parser(true, true, crash_s.is_homefail_z(), { .z_raise = 0 })) {
                // Unsuccesfull rehome
                set_axis_is_not_at_home(X_AXIS);
                set_axis_is_not_at_home(Y_AXIS);
                crash_s.count_crash(); // Count as another crash

                if (crash_s.is_repeated_crash()) { // Cannot home repeatedly
                    disable_XY(); // Let user move the carriage
                    Crash_recovery_fsm cr_fsm(SelftestSubtestState_t::undef, SelftestSubtestState_t::undef);
                    fsm_change(PhasesCrashRecovery::home_fail, cr_fsm.Serialize()); // Retry screen
                    server.print_state = State::CrashRecovery_HOMEFAIL; // Ask to retry
                }
                break;
            }
        }

        if (!crash_s.is_repeated_crash()) {
            fsm_destroy(ClientFSM::CrashRecovery);

            // Necessary for print_resume to work
            server.print_state = State::Paused;
            print_resume();
            break;
        }
    #if ENABLED(AXIS_MEASURE)
        Axis_length_t alok = xy_axes_length_ok();
        if (alok != Axis_length_t::ok) {
            server.print_state = State::CrashRecovery_Axis_NOK;
            Crash_recovery_fsm cr_fsm(axis_length_check(X_AXIS), axis_length_check(Y_AXIS));
            PhasesCrashRecovery pcr = (alok == Axis_length_t::shorter) ? PhasesCrashRecovery::axis_short : PhasesCrashRecovery::axis_long;
            fsm_change(pcr, cr_fsm.Serialize());
            break;
        }
    #endif
        Crash_recovery_fsm cr_fsm(SelftestSubtestState_t::undef, SelftestSubtestState_t::undef);
        fsm_change(PhasesCrashRecovery::repeated_crash, cr_fsm.Serialize());
        server.print_state = State::CrashRecovery_Repeated_Crash;
        break;
    }
    case State::CrashRecovery_HOMEFAIL: {
        switch (marlin_server::get_response_from_phase(PhasesCrashRecovery::home_fail)) {
        case Response::Retry: {
            Crash_recovery_fsm cr_fsm(SelftestSubtestState_t::running, SelftestSubtestState_t::undef);
            fsm_change(PhasesCrashRecovery::home, cr_fsm.Serialize()); // Homing screen
            measure_axes_and_home();
            break;
        }
        default:
            break;
        }
        break;
    }
    case State::CrashRecovery_Axis_NOK: {
        switch (marlin_server::get_response_from_phase(PhasesCrashRecovery::axis_NOK)) {
        case Response::Retry:
            measure_axes_and_home();
            break;
        case Response::Resume: /// ignore wrong length of axes
            fsm_destroy(ClientFSM::CrashRecovery);
    #if ENABLED(AXIS_MEASURE)
            axes_length_set_ok(); /// ignore re-test of lengths
    #endif
            // Necessary for print_resume to work
            server.print_state = State::Paused;
            print_resume();
            break;
        case Response::_none:
            break;
        default:
            server.print_state = State::Paused;
            fsm_destroy(ClientFSM::CrashRecovery);
        }
        break;
    }
    case State::CrashRecovery_Repeated_Crash: {
        switch (marlin_server::get_response_from_phase(PhasesCrashRecovery::repeated_crash)) {
        case Response::Resume:
            fsm_destroy(ClientFSM::CrashRecovery);

            // Necessary for print_resume to work
            server.print_state = State::Paused;
            print_resume();
            break;
        case Response::_none:
            break;
        default:
            server.print_state = State::Paused;
            fsm_destroy(ClientFSM::CrashRecovery);
        }
        break;
    }
#endif
#if HAS_POWER_PANIC()
    case State::PowerPanic_acFault:
        power_panic::panic_loop();
        break;
    case State::PowerPanic_AwaitingResume:
    case State::PowerPanic_Resume:
        power_panic::resume_loop();
        break;
#endif
    default:
        break;
    }

    bool do_fan_check = marlin_vars().fan_check_enabled;
#if HAS_SELFTEST()
    // Do not check fan error in marlin server during Fan selftest
    do_fan_check &= !fsm_states[ClientFSM::FansSelftest].has_value();
#endif

    if (do_fan_check) {
#if !PRINTER_IS_PRUSA_iX()
        for (auto tool : PhysicalToolIndex::all()) {
            hotendFanErrorChecker[tool].checkTrue(Fans::heat_break(tool).is_fan_ok(), WarningType::HotendFanError, true, true);
        }
#endif
        if (auto tool = PhysicalToolIndex::currently_selected_opt()) {
            printFanErrorChecker.checkTrue(Fans::print(*tool).is_fan_ok(), WarningType::PrintFanError, false, true);
        }
#if HAS_INDX()
        // The dock fan is auxiliary (cools the tool dock) and not present on
        // all dev units yet, so a fault must only warn — never pause the print.
        dockFanErrorChecker.checkTrue(Fans::dock_fan().is_fan_ok(), WarningType::DockFanError, false, false);
#endif

#if XBUDDY_EXTENSION_VARIANT_IS_STANDARD()
        const bool cool_fan_ok = buddy::xbuddy_extension().is_fan_ok(buddy::XBuddyExtension::Fan::cooling_fan_1) && buddy::xbuddy_extension().is_fan_ok(buddy::XBuddyExtension::Fan::cooling_fan_2);
        xbe_cool_fan_checker.checkTrue(cool_fan_ok, WarningType::ChamberCoolingFanError, false, false);
        if (cool_fan_ok) {
            xbe_cool_fan_checker.reset();
        }

        const bool filter_fan_ok = buddy::xbuddy_extension().is_fan_ok(buddy::XBuddyExtension::Fan::filtration_fan);
        xbe_filter_fan_checker.checkTrue(filter_fan_ok, WarningType::ChamberFiltrationFanError, false, false);
        if (filter_fan_ok) {
            xbe_filter_fan_checker.reset();
        }
#endif /* XBUDDY_EXTENSION_VARIANT_IS_STANDARD() */
#if XL_ENCLOSURE_SUPPORT()
        const bool enclosure_fan_ok = Fans::enclosure().is_fan_ok();
        if (!enclosure_fan_ok && !enclosure_fan_checker.isFailed()) {
            xl_enclosure.setEnabled(false);
        }
        enclosure_fan_checker.checkTrue(enclosure_fan_ok, WarningType::EnclosureFanError, false, false);
        if (enclosure_fan_ok) {
            enclosure_fan_checker.reset();
        }
#endif
    }

    for (auto tool : PhysicalToolIndex::all()) {
        if (Fans::heat_break(tool).get_rpm_is_ok()) {
            hotendFanErrorChecker[tool].reset();
        }
    }
    if (auto tool = PhysicalToolIndex::currently_selected_opt()) {
        if (Fans::print(*tool).get_rpm_is_ok()) {
            printFanErrorChecker.reset();
        }
    }
#if HAS_INDX()
    if (Fans::dock_fan().get_rpm_is_ok()) {
        dockFanErrorChecker.reset();
    }
#endif

#if HAS_TEMP_HEATBREAK
    for (auto tool : PhysicalToolIndex::all()) {
    #if HAS_TOOLCHANGER()
        if (!tool.is_enabled()) {
            continue;
        }
    #endif

        const auto temp = thermalManager.degHeatbreak(tool);

        // Heatbreak is not yet initialized -> nothing to check
        if (temp == TempInfo::celsius_uninitialized) {
            continue;
        }
        // Heatbreak started reporting valid temperatures -> clear the warning
        else if (temp > 10) {
            heatBreakThermistorErrorChecker[tool].reset();
        }
        // Getting 0 -> heatbreak error
        else {
            heatBreakThermistorErrorChecker[tool].checkTrue(!NEAR_ZERO(temp), WarningType::HeatBreakThermistorFail, true, true);
        }
    }
#endif

#if ENABLED(MODEL_DETECT_STUCK_THERMISTOR)
    static_assert(PhysicalToolIndex::count == 1);
    hotendErrorChecker.checkTrue(Hotend::for_tool(PhysicalToolIndex::from_raw(0)).is_thermal_model_protection_ok());
#endif

    // Check MCU temperatures
    mcuMaxTempErrorChecker.check(AdcGet::getMCUTemp(), WarningType::BuddyMCUMaxTemp, "Buddy");
#if HAS_DWARF()
    for (auto tool : PhysicalToolIndex::all()) {
        if (tool.is_enabled()) {
            static_assert(PhysicalToolIndex::count <= 9);
            char dwarf_name[] = "Dwarf 0";
            dwarf_name[sizeof(dwarf_name) - 2] += tool.display_index();
            dwarfMaxTempErrorChecker[tool].check(buddy::puppies::dwarfs[tool].get_mcu_temperature(), WarningType::DwarfMCUMaxTemp, dwarf_name);
        }
    }
#endif /*HAS_DWARF()*/
#if HAS_REMOTE_BED()
    modbedMaxTempErrorChecker.check(remote_bed::get_mcu_temperature(), WarningType::BedMCUMaxTemp, "Bed");
#endif
}

void resuming_begin(void) {
    // Reset errors, so it can be triggered immediately again
    for (auto tool : PhysicalToolIndex::all()) {
        hotendFanErrorChecker[tool].reset();
    }
    printFanErrorChecker.reset();
#if HAS_INDX()
    dockFanErrorChecker.reset();
#endif

    mcuMaxTempErrorChecker.reset();
#if HAS_DWARF()
    for (auto tool : PhysicalToolIndex::all()) {
        if (tool.is_enabled()) {
            dwarfMaxTempErrorChecker[tool].reset();
        }
    }
#endif /*HAS_DWARF()*/
#if HAS_REMOTE_BED()
    modbedMaxTempErrorChecker.reset();
#endif /*HAS_REMOTE_BED()*/

#if HAS_INDX()
    // Pick the tool from its dock before setting temperatures.
    // Inactive tools don't accept temperatures and won't heat up.
    //
    // And waiting after they've heat up wouldn't work, for obivous reasons.
    if (server.resume.active_tool != PrusaToolChanger::MARLIN_NO_TOOL_PICKED) {
        tool_change(PhysicalToolIndex::from_raw(server.resume.active_tool), tool_return_t::no_return);
        // Crash recovery must resume in place, without the park detour
        if (!crash_s.did_trigger()) {
            // Park for the reheat and the long Z return; on INDX the park position is over the wastebin
            mapi::park(mapi::get_parking_position(mapi::ParkPosition::park).without_z_move());
        }
    }
#endif

    for (auto tool : PhysicalToolIndex::all()) {
        thermalManager.setTargetHotend(server.resume.nozzle_temp[tool], tool);
    }

    thermalManager.set_print_fan_speed(0); // disable print fan
    server.print_state = State::Resuming_Reheating;
}

const GCodeReaderStreamRestoreInfo &stream_restore_info() {
    return print_state.media_restore_info;
}

void print_quick_stop_powerpanic() {
    queue.clear();
}

uint32_t media_position() {
    return queue.last_executed_sdpos;
}

void set_media_position(uint32_t set) {
    // Both sdpos and last_executed_sdpos are needed to be set cause if any gcode is queued and sdpos is invalid while lastExecutedSdpos is not,
    // it is overridden and therefore lost. (This was happening during PP when the print was paused)
    queue.sdpos = set;
    queue.last_executed_sdpos = set;

    // The queue should be empty when we're setting the position,
    // but if it isn't, we need to set the sdpos for all items in the queue
    // Otherwise executing one of the items would overwrite `queue.last_executed_sdpos`
    // This actually happens on crash recovery where SerialPrinting::pause overwrites whatever was set by the Crash_s.
    for (auto &sdpos : queue.sdpos_buffer) {
        sdpos = set;
    }
}

static void retract() {
    mapi::retract_to(STANDARD_RETRACT_LENGTH, buddy::standard_feedrates::current_extruder(buddy::standard_feedrates::Extruder::retract));
}

static void lift_head() {

    // In case of pause/toolfall/etc, we want to move the bed by considerable distance,
    // to create enough space for user to reach in (to fix the tool fall for example)
    float height = Z_NOZZLE_PARK_RISE;

    // Big Z moves might not be needed in other cases -> namely filament runout
    // on a printer with nozzle cleaner - the user does not need to reach in to
    // clean extruded filament
    if (crash_s.did_trigger() && !crash_s.is_toolchange_event()) {
        height = Z_NOZZLE_PARK_RISE_M600;
    }

    float target_z = std::max(current_position.z, planner.max_printed_z) + height;

#ifdef Z_NOZZLE_PARK_POINT_MIN
    if (crash_s.get_state() != Crash_s::RECOVERY) {
        // This usually moves the bed to the middle of the printer or lower,
        // pointless during crash
        target_z = std::max(target_z, Z_NOZZLE_PARK_POINT_MIN);
    }
#endif

    const float distance = std::min(target_z, Z_MAX_POS) - current_position.z;

    static_assert(Z_NOZZLE_PARK_POINT > 0);

    if (axes_home_level.is_homed(Z_AXIS, AxisHomeLevel::imprecise)) {
        // Do prepare_move_to, as it segments the move and thus allows better emergency_stop
        destination = current_position;
        destination.z += distance;
        prepare_move_to(destination, HOMING_FEEDRATE_INVERTED_Z, {});
        planner.synchronize();

    } else {
        // have to use HOMING_FEEDRATE, otherwise the stallguards might not trigger
        do_homing_move(Z_AXIS, distance, HOMING_FEEDRATE_INVERTED_Z);
    }
}

static void retract_and_lift() {
    retract();
    lift_head();
}

static void park_head([[maybe_unused]] bool is_pause) {
    if (!all_axes_homed()) {
        return;
    }

#if HAS_TOOLCHANGER()
    // Check that we are not in dock
    // Can happen if stopped during toolchanging, toolchange will finish but last move doesn't wait for planner.synchronize();
    if (prusa_toolchanger.is_pos_in_toolchange_area(current_position.xy())) {
        current_position.y = PrusaToolChanger::SAFE_Y_WITH_TOOL;
        line_to_current_position(NOZZLE_PARK_XY_FEEDRATE); // Move to safe Y
        planner.synchronize();
    }
#endif

#if PRINTER_IS_PRUSA_iX()
    if (is_pause) {
        mapi::park(mapi::get_parking_position(mapi::ParkPosition::park).without_z_move());
    } else
#endif
    {
        mapi::park(mapi::get_parking_position(mapi::ParkPosition::print_end).without_z_move());
    }
}

#if HAS_NOZZLE_CLEANER()
void unpark_prime() {
    if (std::holds_alternative<NoTool>(PhysicalToolIndex::currently_selected())) {
        return;
    }

    if (!all_axes_homed() || thermalManager.tooColdToExtrude(active_extruder)) {
        return;
    }

    nozzle_cleaner::load_and_execute(nozzle_cleaner::Sequence::purge_clean);
    sync_e_position_to(server.resume.pos.e);
}
#endif // HAS_NOZZLE_CLEANER()

void unpark_head_XY(void) {
    // TODO: double check this condition: when recovering from a crash, Z is not known, but we *can*
    // unpark, so we bypass this check as we need to move back
    if ((!HAS_CRASH_DETECTION() || !crash_s.did_trigger()) && !all_axes_homed()) {
        return;
    }

    // The pause park position is off the print, so do the long Z return there, before the
    // XY traverse; the traverse keeps a small clearance above the print, unpark_head_ZE
    // does the rest
    static constexpr float traverse_clearance = 5;
    const float traverse_z = std::max(server.resume.pos.z, planner.max_printed_z) + traverse_clearance;
    do_blocking_move_to_z(std::min(current_position.z, traverse_z), NOZZLE_PARK_Z_FEEDRATE, Segmented::yes);

    mapi::park({ .x = server.resume.pos.x, .y = std::min<float>(server.resume.pos.y, Y_BED_SIZE) });
}

void unpark_head_ZE(void) {
    // TODO: see comment above on unparking: if axes are not known, lift is skipped, but not this
    if (!all_axes_homed()) {
        return;
    }

    // Move Z
    destination = current_position;
    destination.z = server.resume.pos.z;
    prepare_internal_move_to_destination(NOZZLE_PARK_Z_FEEDRATE);

#if HAS_PAUSE()
    // Undo E retract
    mapi::extruder_move(server.resume.pos.e - current_position.e, buddy::standard_feedrates::current_extruder(buddy::standard_feedrates::Extruder::deretract));
#endif
}

bool all_axes_homed(void) {
    return ::all_axes_homed();
}

bool all_axes_known(void) {
    return ::all_axes_known();
}

int get_exclusive_mode(void) {
    return (server.flags & MARLIN_SFLG_EXCMODE) ? 1 : 0;
}

void set_exclusive_mode(int exclusive) {
    if (exclusive) {
        SerialUSB.setIsWriteOnly(true);
        server.flags |= MARLIN_SFLG_EXCMODE; // enter exclusive mode
    } else {
        server.flags &= ~MARLIN_SFLG_EXCMODE; // exit exclusive mode
        SerialUSB.setIsWriteOnly(false);
    }
}

void set_target_bed(int16_t value) {
    marlin_vars().target_bed = value;
    thermalManager.setTargetBed(value);
}

namespace call_manually {

    void set_temp_to_display(float value, PhysicalToolIndex tool) {
        marlin_vars().hotend(tool).display_nozzle = value;
    }

} // namespace call_manually

bool get_media_inserted(void) {
    return marlin_vars().media_inserted;
}

resume_state_t *get_resume_data() {
    return &server.resume;
}

void set_resume_data(const resume_state_t *data) {
    // ensure this is called only from the marlin thread
    debug_assert(osThreadGetId() == server_task);
    server.resume = *data;
}

int32_t get_knob_position() {
    return server.knob_position;
}

//-----------------------------------------------------------------------------
// private functions

// send event notification to client (called from server thread)
static bool _send_notify_event_to_client([[maybe_unused]] int client_id, ClientQueue &queue, Event evt_id, uint32_t usr32, uint16_t usr16) {
    const marlin_client::ClientEvent client_message {
        .event = evt_id,
        .unused = 0,
        .usr16 = usr16,
        .usr32 = usr32,
    };
    return queue.try_send(client_message, 0);
}

// send event notification to client - multiple events (called from server thread)
// returns mask of successfully sent events
static uint64_t _send_notify_events_to_client(int client_id, ClientQueue &queue, uint64_t evt_msk) {
    if (evt_msk == 0) {
        return 0;
    }
    uint64_t sent = 0;
    uint64_t msk = 1;
    for (uint8_t evt_int = 0; evt_int <= std::to_underlying(Event::_last); evt_int++) {
        Event evt_id = Event(evt_int);
        if (msk & evt_msk) {
            switch (Event(evt_id)) {
                // Events without arguments
                // TODO: send all these in a single message as a bitfield
            case Event::MediaInserted:
            case Event::MediaError:
            case Event::MediaRemoved:
            case Event::RequestCalibrationsScreen:
                if (_send_notify_event_to_client(client_id, queue, evt_id, 0, 0)) {
                    sent |= msk; // event sent, set bit
                }
                break;
            case Event::NotAcknowledge:
            case Event::Acknowledge:
                if (_send_notify_event_to_client(client_id, queue, evt_id, 0, 0)) {
                    sent |= msk; // event sent, set bit
                }
                break;
            // unused events
            case Event::_count:
                debug_assert(false);
                break;
            }
            if ((sent & msk) == 0) {
                break; // skip sending if queue is full
            }
        }
        msk <<= 1;
    }
    return sent;
}

// send event notification to all clients (called from server thread)
// returns bitmask - bit0 = notify for client0 successfully send, bit1 for client1...
static uint8_t _send_notify_event(Event evt_id, uint32_t usr32, uint16_t usr16) {
    uint8_t client_msk = 0;
    for (int client_id = 0; client_id < MARLIN_MAX_CLIENTS; client_id++) {
        if (server.notify_events[client_id] & ((uint64_t)1 << std::to_underlying(evt_id))) {
            if (_send_notify_event_to_client(client_id, marlin_client::marlin_client_queue[client_id], evt_id, usr32, usr16) == 0) {
                server.client_events[client_id] |= ((uint64_t)1 << std::to_underlying(evt_id)); // event not sent, set bit
            } else {
                // event sent, clear flag
                client_msk |= (1 << client_id);
            }
        }
    }
    return client_msk;
}

// update all server variables
static void _server_update_vars() {
    const auto prefetch_metrics = media_prefetch.get_metrics();

    marlin_vars().gqueue = queue.length;
    marlin_vars().inject_queue_empty = inject_queue.is_empty();
    marlin_vars().is_processing = is_processing();

    // Get native position
    {
        xyze_pos_t pos_mm, curr_pos_mm;
        planner.get_axis_position_mm(pos_mm);
        curr_pos_mm = current_position;
        LOOP_XYZE(i) {
            marlin_vars().native_pos[i] = pos_mm[i];
            marlin_vars().native_curr_pos[i] = curr_pos_mm[i];
        }

        // Convert to logical position
        planner.unapply_leveling(pos_mm);

        const auto logical_pos_mm = pos_mm.asLogical();
        const auto logical_curr_pos_mm = curr_pos_mm.asLogical();
        LOOP_XYZE(i) {
            marlin_vars().logical_pos[i] = logical_pos_mm[i];
            marlin_vars().logical_curr_pos[i] = logical_curr_pos_mm[i];
        }
    }

    for (auto tool : PhysicalToolIndex::all()) {
        auto &extruder = marlin_vars().hotend(tool);
        const auto &hotend = Hotend::for_tool(tool);

        // Downstream consumers (GUI, Connect, chamber_filtration)
        // get's exposed the marlin approach to uninitialized temp
        extruder.temp_nozzle = hotend.nozzle_temp().value_or(TempInfo::celsius_uninitialized);
        extruder.target_nozzle = hotend.nozzle_target_temp();
        extruder.pwm_nozzle = hotend.nozzle_heater_pwm().value;

#if HAS_TEMP_HEATBREAK
        // TODO: this should track multiple extruders
        extruder.temp_heatbreak = hotend.heatbreak_temp();
#endif
#if HAS_TEMP_HEATBREAK_CONTROL
        extruder.target_heatbreak = hotend.heatbreak_target_temp();
#endif
        extruder.print_fan_rpm = Fans::print(tool).get_actual_rpm();
        extruder.heatbreak_fan_rpm = Fans::heat_break(tool).get_actual_rpm();
    }

#if HAS_INDX()
    // INDX_TODO: Update NoTool on INDX - when NoTool is active we still want to
    // update the RPMs of the print and heatbreak fan; A more conceptual
    // solution is needed.
    auto &no_tool_hotend = marlin_vars().hotend(NoTool());
    no_tool_hotend.print_fan_rpm = Fans::print(PhysicalToolIndex::from_raw(0)).get_actual_rpm();
    no_tool_hotend.heatbreak_fan_rpm = Fans::heat_break(PhysicalToolIndex::from_raw(0)).get_actual_rpm();
#endif

    for (auto tool : VirtualToolIndex::all()) {
        auto &virtual_tool_data = marlin_vars().virtual_tools[tool];
        virtual_tool_data.flow_factor = planner.flow_percentage[tool];
    }

    marlin_vars().temp_bed = thermalManager.degBed();
    marlin_vars().target_bed = thermalManager.degTargetBed();
#if HAS_MODULAR_BED()
    marlin_vars().enabled_bedlet_mask = thermalManager.getEnabledBedletMask();
#endif

    marlin_vars().z_offset = probe_offset.z;
    marlin_vars().print_fan_speed = thermalManager.print_fan_speed;
    marlin_vars().print_speed = static_cast<uint16_t>(feedrate_percentage);

    auto progress_data = oProgressData.mode_specific(config_store().stealth_mode.get());

    // If the mode-specific progress data is all empty (never set by the M73 command),
    // fall back to standard mode progress data to show at least something
    if (!progress_data.percent_done.mIsUsed() && !progress_data.time_to_end.mIsUsed() && !progress_data.time_to_pause.mIsUsed()) {
        progress_data = oProgressData.standard_mode;
    }

    marlin_vars().print_duration = print_job_timer.duration();
    marlin_vars().sd_percent_done = [&]() -> uint8_t {
        if (progress_data.percent_done.mIsActual(marlin_vars().print_duration)) {
            return static_cast<uint8_t>(progress_data.percent_done.mGetValue());
        } else if (prefetch_metrics.stream_size_estimate > 0) {
            return std::min<uint8_t>(static_cast<uint8_t>(std::round(100.0f * queue.last_executed_sdpos / prefetch_metrics.stream_size_estimate)), 99);
        } else {
            return 0;
        }
    }();

    if (const bool media = usb_host::is_media_inserted(); marlin_vars().media_inserted != media) {
        marlin_vars().media_inserted = media;
        _send_notify_event(marlin_vars().media_inserted ? Event::MediaInserted : Event::MediaRemoved, 0, 0);
    }

    const auto duration = marlin_vars().print_duration.get();
    const auto print_speed = marlin_vars().print_speed.get();

    const auto update_time_to = [&](const ClValidityValueSec &progress_data_value, MarlinVariable<uint32_t> &marlin_var) {
        uint32_t v = TIME_TO_END_INVALID;
        if (progress_data.percent_done.mIsActual(duration) && progress_data_value.mIsActual(duration)) {
            v = progress_data_value.mGetValue();
        }

        if (print_speed == 100 || v == TIME_TO_END_INVALID) {
            marlin_var = v;
        } else {
            // multiply by 100 is safe, it limits time_to_end to ~21mil. seconds (248 days)
            marlin_var = (v * 100) / print_speed;
        }
    };
    update_time_to(progress_data.time_to_end, marlin_vars().time_to_end);
    update_time_to(progress_data.time_to_pause, marlin_vars().time_to_pause);

    if (server.print_state == State::Printing) {
        marlin_vars().time_to_end.execute_with([&](const uint32_t &time_to_end) {
            if (time_to_end != TIME_TO_END_INVALID) {
                marlin_vars().print_end_time = time(nullptr) + time_to_end;
            } else {
                marlin_vars().print_end_time = TIMESTAMP_INVALID;
            }
        });
    }

    marlin_vars().job_id = job_id;
    marlin_vars().travel_acceleration = planner.settings.travel_acceleration;
    marlin_vars().max_printed_z = planner.max_printed_z;

    uint8_t mmu2State =
#if HAS_MMU2()
        uint8_t(MMU2::mmu2.State());
#else
        0;
#endif
    marlin_vars().mmu2_state = mmu2State;

    bool mmu2FindaPressed =
#if HAS_MMU2()
        MMU2::mmu2.FindaDetectsFilament();
#else
        false;
#endif

    marlin_vars().mmu2_finda = mmu2FindaPressed;

    marlin_vars().active_extruder = VirtualToolIndex::from_raw_notool(active_extruder);

#if ENABLED(PREVENT_COLD_EXTRUSION)
    marlin_vars().extrude_min_temp = thermalManager.extrude_min_temp;
    marlin_vars().allow_cold_extrude = thermalManager.allow_cold_extrude;
#endif /* ENABLED(PREVENT_COLD_EXTRUSION) */

    // print state is updated last, to make sure other related variables (like job_id, filenames) are already set when we start print
    marlin_vars().print_state = static_cast<State>(server.print_state);

    marlin_vars().media_position = media_position();

    marlin_vars().media_size_estimate = prefetch_metrics.stream_size_estimate;
}

bool _process_server_valid_request(const Request &request, int client_id) {
    switch (request.type) {
    case Request::Type::Gcode:
        //@TODO return value depending on success of enqueueing gcode
        return enqueue_gcode_try(request.gcode);
    case Request::Type::Inject:
        return inject(request.inject);
    case Request::Type::GcodeInterrupt:
        gcode_interrupt(request.gcode_interrupt);
        return true;
    case Request::Type::SetVariable:
        _server_set_var(request);
        return true;
    case Request::Type::Babystep:
        do_babystep_Z(request.babystep);
        return true;
#if HAS_CANCEL_OBJECT()
    case Request::Type::CancelObjectID:
    case Request::Type::UncancelObjectID:
        buddy::cancel_object().set_object_cancelled(request.cancel_object_id, request.type == Request::Type::CancelObjectID);
        return true;
#else
    // #error dead code found by automatic analyses (see BFW-5461)
    case Request::Type::CancelObjectID:
    case Request::Type::UncancelObjectID:
        return false;
#endif
    case Request::Type::PrintStart:
        print_start(request.print_start.filename, GCodeReaderPosition(), request.print_start.skip_preview, request.print_start.reset_tool_mapping);
        return true;
    case Request::Type::SetWarning:
        set_warning(request.warning_type);
        return true;
    case Request::Type::EventMask:
        server.notify_events[client_id] = request.event_mask;
        // Send Event::MediaInserted event if media currently inserted
        // This is temporary solution, Event::MediaInserted and Event::MediaRemoved events are replaced
        // with variable media_inserted, but some parts of application still using the events.
        // We need this workaround for app startup.
        if ((server.notify_events[client_id] & make_mask(Event::MediaInserted)) && marlin_vars().media_inserted) {
            server.client_events[client_id] |= make_mask(Event::MediaInserted);
        }
        return true;
#if HAS_SELFTEST()
    case Request::Type::TestStart:
        marlin_server::test_start(request.test_start.test_mask, request.test_start.test_data);
        return true;
#else
        // #error dead code found by automatic analyses (see BFW-5461)
        return false;
#endif
    }
    bsod("Unknown request %d", std::to_underlying(request.type));
}

void send_request_flag(const RequestFlag request) {

    // These requests shouldn't be called at once (in single step of marlin_server's cycle)
    static constexpr uint32_t exclusive_print_request_mask = (0x1 << std::to_underlying(RequestFlag::PrintResume)) | (0x1 << std::to_underlying(RequestFlag::PrintPause)) | (0x1 << std::to_underlying(RequestFlag::PrintAbort));
    const uint32_t curr_request_flag = 0x1 << std::to_underlying(request);

    uint32_t flags = request_flags.load();
    uint32_t new_flags;

    do {
        new_flags = flags;
        // Clear exclusive print flags if set
        if (curr_request_flag & exclusive_print_request_mask) {
            new_flags &= ~exclusive_print_request_mask;
        }
        new_flags |= curr_request_flag;
    } while (!request_flags.compare_exchange_strong(flags, new_flags));
}

static void process_request_flags() {
    const uint32_t flags = request_flags.exchange(0);
    if (flags == 0) {
        return;
    }

    for (uint8_t i = 0; i < std::to_underlying(RequestFlag::_cnt); i++) {
        if (!(flags & (0x1 << i))) {
            continue;
        }

        switch (RequestFlag(i)) {
        case RequestFlag::PrintAbort:
            print_abort();
            break;
        case RequestFlag::PrintPause:
            print_pause();
            break;
        case RequestFlag::PrintResume:
            print_resume();
            break;
        case RequestFlag::TryRecoverFromMediaError:
            try_recover_from_media_error();
            break;
        case RequestFlag::PrintExit:
            print_exit();
            break;
        case RequestFlag::KnobMoveUp:
            buddy::safety_timer().reset_norestore();
            server.knob_position++;
            break;
        case RequestFlag::KnobMoveDown:
            buddy::safety_timer().reset_norestore();
            server.knob_position--;
            break;
        case RequestFlag::KnobClick:
            buddy::safety_timer().reset_restore_nonblocking();
            break;
#if HAS_SELFTEST()
        case RequestFlag::TestAbort:
            test_abort();
            break;
#endif
#if HAS_CANCEL_OBJECT()
        case RequestFlag::CancelCurrentObject:
            buddy::cancel_object().set_object_cancelled(buddy::cancel_object().current_object(), true);
            break;
#endif
        case RequestFlag::_cnt:
            break;
        }
    }
}

static bool _process_server_request(const Request &request) {
    const uint8_t client_id = request.client_id;
    if (client_id >= MARLIN_MAX_CLIENTS) {
        return true;
    }

    const bool processed = _process_server_valid_request(request, client_id);

    // force update of marlin variables after proecssing request -> to ensure client can read latest variables after request completion
    _server_update_vars();

    if (request.response_required) {
        Event evt_result = processed ? Event::Acknowledge : Event::NotAcknowledge;
        if (!_send_notify_event_to_client(client_id, marlin_client::marlin_client_queue[client_id], evt_result, 0, 0)) {
            // FIXME: Take care of resending process elsewhere.
            server.client_events[client_id] |= make_mask(evt_result); // set bit if notification not sent
        }
    }
    return processed;
}

// set variable from string request
static void _server_set_var(const Request &request) {
    const uintptr_t variable_identifier = request.set_variable.variable;

    // Set normal (non-extruder) variables
    if (variable_identifier == reinterpret_cast<uintptr_t>(&marlin_vars().target_bed)) {
        marlin_vars().target_bed = static_cast<int16_t>(request.set_variable.uint32_value);
        thermalManager.setTargetBed(marlin_vars().target_bed);
        return;
    }
    if (variable_identifier == reinterpret_cast<uintptr_t>(&marlin_vars().z_offset)) {
        marlin_vars().z_offset = request.set_variable.float_value;
#if HAS_BED_PROBE
        probe_offset.z = marlin_vars().z_offset;
#endif // HAS_BED_PROBE
        return;
    }
    if (variable_identifier == reinterpret_cast<uintptr_t>(&marlin_vars().print_fan_speed)) {
        marlin_vars().print_fan_speed = request.set_variable.uint32_value;
        thermalManager.set_print_fan_speed(marlin_vars().print_fan_speed);
        return;
    }
    if (variable_identifier == reinterpret_cast<uintptr_t>(&marlin_vars().print_speed)) {
        marlin_vars().print_speed = request.set_variable.uint32_value;
        feedrate_percentage = (int16_t)marlin_vars().print_speed;
        return;
    }
    if (variable_identifier == reinterpret_cast<uintptr_t>(&marlin_vars().fan_check_enabled)) {
        marlin_vars().fan_check_enabled = request.set_variable.uint32_value;
        return;
    }

    // Now see if extruder variable is set
    for (auto tool : PhysicalToolIndex::all()) {
        auto &physical_tool_data = marlin_vars().hotend(tool);
        if (reinterpret_cast<uintptr_t>(&physical_tool_data.target_nozzle) == variable_identifier) {
            physical_tool_data.target_nozzle = static_cast<int16_t>(request.set_variable.uint32_value);
            thermalManager.setTargetHotend(physical_tool_data.target_nozzle, tool);
            return;
        }
    }

    for (auto tool : VirtualToolIndex::all()) {
        auto &virtual_tool_data = marlin_vars().virtual_tools[tool];
        if (reinterpret_cast<uintptr_t>(&virtual_tool_data.flow_factor) == variable_identifier) {
            virtual_tool_data.flow_factor = request.set_variable.uint32_value;
            planner.flow_percentage[tool] = (int16_t)virtual_tool_data.flow_factor;
            planner.refresh_e_factor(tool);
            return;
        }
    }

    // if we got here, no variable was set, return error
    bsod("unimplemented _server_set_var for var_id %i", (int)variable_identifier);
}

FSMResponseVariant get_response_variant_from_phase(FSMAndPhase fsm_and_phase, bool consume_response) {
    // The FSM should be active the whole time we're waiting for the response.
    // If it isn't, something's probably wrong
    debug_assert(fsm_states[fsm_and_phase.fsm].has_value());

    FSMResponseVariant result;

    fsm_response.transform([&](EncodedFSMResponse &value) {
        if (value.fsm_and_phase != fsm_and_phase) {
            // The response is for a different phase -> do not consume it, do not return it
            return;
        }

        result = value.response;

        if (consume_response) {
            value = empty_encoded_fsm_response;
        }
    });

    if (result != FSMResponseVariant {}) {
        // Receiving a valid response from anywhere (for example Connect) counts as activity, prolong the activity heaters timeout
        buddy::safety_timer().reset_norestore();
    }

    return result;
}

void set_response(const EncodedFSMResponse &response) {
    fsm_response = response;
}

/// Clears any pending response for the provided FSM
void clear_fsm_response(ClientFSM fsm) {
    fsm_response.transform([fsm](EncodedFSMResponse &response) {
        if (response.fsm_and_phase.fsm == fsm) {
            response = empty_encoded_fsm_response;
        }
    });
}

FSMResponseVariant wait_for_response_variant(FSMAndPhase fsm_and_phase, uint32_t timeout_ms) {
    // Warning phase response is consumed in marlin_server::handle_warnings
    debug_assert(fsm_and_phase != PhasesWarning::Warning);

    const auto wait_start = ticks_ms();

    while (true) {
        if (auto r = get_response_variant_from_phase(fsm_and_phase)) {
            return r;
        }

        if (timeout_ms && ticks_diff(ticks_ms(), wait_start) > int32_t(timeout_ms)) {
            return FSMResponseVariant::make(Response::_none);
        }

        ::idle(true);
    }
}

bool is_marlin_server_thread() {
    return osThreadGetId() == defaultTaskHandle;
}

// Secret function called from M9944
void execute_gcode_interrupt() {
    // Someone might get a dumb idea to call this internal g-code explicitly...
    release_assert(!print_state.gcode_interrupt_command.is_empty());

    const auto orig_offset = native_logical_offset();

    ArrayStringBuilder<MAX_CMD_SIZE> sb;
    print_state.gcode_interrupt_command.to_string(sb);
    gcode.process_subcommands_now(const_cast<char *>(sb.str()));

    // Clear the interrupt command AFTER it was successfully processed
    // If there would be a nested crash during the execution, the interrupting gcode will be repeated
    print_state.gcode_interrupt_command = {};

    if (!planner.draining()) {
        // Okay, so why we need this ugly thing:
        // The gcode_interrupt could have executed a toolchange (for example bcs of spool join)
        // at which point all of the resume positions need to be adjusted,
        // because the new tool has a different tool offset
        //
        // It's very important that we do this with the correct target temperature,
        // because tool offset also changes with that if HAS_NOZZLE_THERMAL_COMPENSATION
        //
        // BFW-9250

        const XYZval<float, NativePosTag> delta = orig_offset - native_logical_offset();
        server.resume.pos += delta;
        crash_s.start_current_position += delta;
        crash_s.crash_native_position += delta;
        crash_s.crash_machine_position = to_machine_pos(to_native_pos(crash_s.crash_machine_position) + delta);
    }
}

} // namespace marlin_server

#if _DEBUG
/// @note Hacky link for Marlin.cpp used for development.
/// @todo Remove when stepper timeout screen is solved properly.
void marlin_server_steppers_timeout_warning() {
    marlin_server::set_warning(WarningType::SteppersTimeout);
}
#endif //_DEBUG

//-----------------------------------------------------------------------------
// ExtUI event handlers

[[noreturn]] void kill(PGM_P const lcd_error, PGM_P const lcd_component, [[maybe_unused]] const bool steppers_off) {
    const char *msg = lcd_error ?: GET_TEXT(MSG_KILLED);
    log_info(MarlinServer, "Printer killed: %s", msg);
    fatal_error(msg, lcd_component);
}

namespace ExtUI {

using namespace marlin_server;

void onIdle() {
    // Note: idle_running can lock out some functionality inside cycle()
    // that is happening in the outer loop
    AutoRestore idle_running_guard(idle_running, true);

    cycle();

    // update sensor values for metrics and sensor screens
    sensor_data().update();
    buddy::metrics::record();

#if BOARD_IS_XLBUDDY() && HAS_EXTENDED_PRINTER_TYPE() && HAS_CPU_FAN()
    // Update CPU fan speed based on temperature (XLS only). On plain XL
    // we never touch the fan; the CFanCtl3Wire instance stays at PWM=0,
    // pin low, MOSFET off. Suppressed during selftest so the M1978 fan
    // test can drive the fan to 100 % without the temperature policy
    // racing it back to 0.
    #if HAS_SELFTEST()
    const bool selftest_running = SelftestInstance().IsInProgress();
    #else
    constexpr bool selftest_running = false;
    #endif
    if (!selftest_running && PrinterModelInfo::current().model == PrinterModel::xls) {
        cpu_fan_controller::update(std::max(std::max(sensor_data().MCUTemp.load(), sensor_data().sandwichTemp.load()), sensor_data().boardTemp.load()));
    }
#endif
}

void onPrintTimerStarted() {
    log_info(MarlinServer, "ExtUI: onPrintTimerStarted");
}

void onPrintTimerPaused() {
    log_info(MarlinServer, "ExtUI: onPrintTimerPaused");
}

void onPrintTimerStopped() {
    log_info(MarlinServer, "ExtUI: onPrintTimerStopped");
}

void onMeshUpdate([[maybe_unused]] const uint8_t xpos, [[maybe_unused]] const uint8_t ypos, [[maybe_unused]] const float zval) {
    log_debug(MarlinServer, "ExtUI: onMeshUpdate x: %u, y: %u, z: %.2f", xpos, ypos, (double)zval);
}

} // namespace ExtUI

alignas(std::max_align_t) uint8_t FSMExtendedDataManager::extended_data_buffer[FSMExtendedDataManager::buffer_size] = { 0 };
size_t FSMExtendedDataManager::identifier = { 0 };

void marlin_server::request_calibrations_screen() {
    _send_notify_event(marlin_server::Event::RequestCalibrationsScreen, 0, 0);
}
