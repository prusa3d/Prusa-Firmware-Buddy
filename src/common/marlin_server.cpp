#include "marlin_server.hpp"

#include <freertos/critical_section.hpp>
#include <marlin_stubs/skippable_gcode.hpp>
#include "marlin_client_queue.hpp"
#include "marlin_server_request.hpp"
#include <inttypes.h>
#include <stdarg.h>
#include <cstdint>
#include <stdio.h>
#include <string.h> //strncmp
#include <assert.h>
#include <charconv>

#include "adc.hpp"
#include "marlin_events.h"
#include "marlin_print_preview.hpp"
#include "bsod.h"
#include "module/prusa/tool_mapper.hpp"
#include "module/prusa/spool_join.hpp"
#include "print_utils.hpp"
#include "random.h"
#include "timing.h"
#include "cmsis_os.h"
#include <logging/log.hpp>
#include <bsod_gui.hpp>
#include <usb_host.h>
#include <st25dv64k.h>
#include <usb_host.h>
#include <lfn.h>
#include <media_prefetch/media_prefetch.hpp>
#include <gcode/gcode_reader_restore_info.hpp>
#include <dirent.h>
#include <scope_guard.hpp>
#include <tools_mapping.hpp>
#include <RAII.hpp>

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
#include "Marlin/src/feature/bed_preheat.hpp"
#include "../Marlin/src/libs/nozzle.h"
#include "../Marlin/src/core/language.h" //GET_TEXT(MSG)
#include "../Marlin/src/gcode/gcode.h"
#include "../Marlin/src/gcode/lcd/M73_PE.h"
#include "../Marlin/src/feature/print_area.h"
#include "../Marlin/src/Marlin.h"
#include "utility_extensions.hpp"

#if ENABLED(PRUSA_MMU2)
    #include "../Marlin/src/feature/prusa/MMU2/mmu2_mk4.h"
#endif

#if ENABLED(CANCEL_OBJECTS)
    #include "../Marlin/src/feature/cancel_object.h"
#endif

#include "hwio.h"
#include "wdt.hpp"
#include "../marlin_stubs/M123.hpp"
#include "fsm_states.hpp"
#include "odometer.hpp"
#include "metric.h"
#include "app_metrics.h"
#include "media_prefetch_instance.hpp"

#include <option/has_leds.h>
#if HAS_LEDS()
    #include "led_animations/printer_animation_state.hpp"
#endif

#include "fanctl.hpp"
#include "lcd/extensible_ui/ui_api.h"

#include <option/has_gui.h>
#include <option/has_toolchanger.h>
#include <option/has_selftest.h>
#include <option/has_mmu2.h>
#include <option/has_dwarf.h>
#include <option/has_modularbed.h>
#include <option/has_loadcell.h>
#include <option/has_nfc.h>
#include <option/has_sheet_profiles.h>
#include <option/has_i2c_expander.h>
#include <option/has_chamber_api.h>
#include <option/has_xbuddy_extension.h>
#include <option/has_emergency_stop.h>
#include <option/has_uneven_bed_prompt.h>

#if HAS_DWARF()
    #include <puppies/Dwarf.hpp>
#endif /*HAS_DWARF()*/

#if HAS_MODULARBED()
    #include <puppies/modular_bed.hpp>
#endif /*HAS_MODULARBED()*/

#if HAS_SELFTEST()
    #include "printer_selftest.hpp"
    #include "i_selftest.hpp"
#endif

#if HAS_SHEET_PROFILES()
    #include "SteelSheets.hpp"
#endif

#if ENABLED(CRASH_RECOVERY)
    #include "../Marlin/src/feature/prusa/crash_recovery.hpp"
    #include "crash_recovery_type.hpp"
    #include "selftest_axis.h"
#endif

#if ENABLED(POWER_PANIC)
    #include "power_panic.hpp"
#endif

#if ENABLED(PRUSA_TOOLCHANGER)
    #include "module/prusa/toolchanger.h"
#endif

#if HAS_MMU2()
    #include <mmu2/mmu2_fsm.hpp>
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

#if HAS_XBUDDY_EXTENSION()
    #include <feature/xbuddy_extension/xbuddy_extension.hpp>
#endif
#if HAS_EMERGENCY_STOP()
    #include <feature/emergency_stop/emergency_stop.hpp>
#endif

#include <wui.h>

using namespace ExtUI;

using ClientQueue = marlin_client::ClientQueue;

LOG_COMPONENT_DEF(MarlinServer, logging::Severity::info);

//-----------------------------------------------------------------------------
// external variables from marlin_client

namespace marlin_client {
extern osThreadId marlin_client_task[MARLIN_MAX_CLIENTS]; // task handles
extern ClientQueue marlin_client_queue[MARLIN_MAX_CLIENTS];
} // namespace marlin_client

namespace marlin_server {

void media_prefetch_start();

namespace {

    struct server_t {
        EventMask notify_events[MARLIN_MAX_CLIENTS]; // event notification mask - message filter
        EventMask notify_changes[MARLIN_MAX_CLIENTS]; // variable change notification mask - message filter
        EventMask client_events[MARLIN_MAX_CLIENTS]; // client event mask - unsent messages
        char *event_messages[MARLIN_MAX_CLIENTS]; // last Event::Message for clients
        State print_state; // printing state (printing, paused, ...)
        bool print_is_serial = false; //< When true, current print is not from USB, but sent via gcode commands.
#if ENABLED(CRASH_RECOVERY) //
        bool aborting_did_crash_trigger = false; // To remember crash_s state when aborting
#endif /*ENABLED(CRASH_RECOVERY)*/
        uint32_t paused_ticks; // tick count in moment when printing paused
        resume_state_t resume; // resume data (state before pausing)
        bool enable_nozzle_temp_timeout; // enables nozzle temperature timeout in print pause
        uint32_t last_update; // last update tick count
        uint32_t command; // actually running command
        uint32_t command_begin; // variable for notification
        uint32_t command_end; // variable for notification
        uint32_t knob_click_counter;
        uint32_t knob_move_counter;
        uint16_t flags; // server flags (MARLIN_SFLG)
        uint8_t idle_cnt; // idle call counter

#if ENABLED(AXIS_MEASURE)
        /// length of axes measured after crash
        /// negative numbers represent undefined length
        xy_float_t axis_length = { -1, -1 };
        Measure_axis *measure_axis = nullptr;
#endif // ENABLED(AXIS_MEASURE)

#if HAS_BED_PROBE || HAS_LOADCELL() && ENABLED(PROBE_CLEANUP_SUPPORT)
        bool mbl_failed;
#endif
        bool was_print_time_saved = false;
    };

    server_t server; // server structure - initialize task to zero

    /// State variables that reset with each print
    struct PrintState {

        /// When print_resume is called during the pausing (or possibly other sequences), we first have to finish the sequence and then start resuming.
        /// This flag stores that we have a resume pending and we should start executing it when we can.
        bool resume_pending = false;

        bool paused_due_to_media_error = false;

        /// Position the media should be resumed to
        GCodeReaderStreamRestoreInfo media_restore_info;

        /// Denotes whether a single gcode should be skipped
        /// Some pauses should cause (partial) gcode replay on resume - crash, power panic, ..., some shouldn't.
        /// This does that
        bool skip_gcode = false;

        /// Whether file open was reported on the serial line.
        /// We cannot do this directly when calling media_prefecth start, we need to wait till we have file size estimate
        bool file_open_reported = false;
    };

    PrintState print_state;

    enum class Pause_Type {
        Pause,
        Repeat_Last_Code,
        Crash
    };

    /**
     * @brief Pauses reading from a file, stops watch, saves temperatures, disables fan.
     * Does not change server.print_state. You need to set that manually.
     * @param type pause type used for different media_print pause
     * @param resume_pos position to resume from, used only in Pause_Type::Crash
     */
    void pause_print(Pause_Type type = Pause_Type::Pause);

    fsm::States fsm_states;

    class ErrorChecker {
    public:
        constexpr ErrorChecker() = default;

        constexpr bool isFailed() const { return m_failed; }

        void checkTrue(bool condition, WarningType warning, bool disable_hotend) {
            if (condition || m_failed) {
                return;
            }
            set_warning(warning);

            if (server.print_state == State::Printing) {
                pause_print(); // Must store current hotend temperatures before they are set to 0
                server.print_state = State::Pausing_WaitIdle;
            }

            if (disable_hotend) {
                HOTEND_LOOP() {
                    thermalManager.setTargetHotend(0, e);
                }
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
#if FAN_COUNT > 0
                    thermalManager.set_fan_speed(0, 255);
#endif
                }
            }

            ErrorChecker::checkTrue(condition, WarningType::HotendTempDiscrepancy, true);

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

            this->checkTrue(!warning, warning_type, true);
        }
    };

    constinit std::array<ErrorChecker, HOTENDS> hotendFanErrorChecker;
    constinit ErrorChecker printFanErrorChecker;

#ifdef HAS_TEMP_HEATBREAK
    constinit std::array<ErrorChecker, HOTENDS> heatBreakThermistorErrorChecker;
#endif
    constinit HotendErrorChecker hotendErrorChecker;

    constinit MCUTempErrorChecker mcuMaxTempErrorChecker; ///< Check Buddy MCU temperature
#if HAS_DWARF()
    static constexpr std::array<const char *, HOTENDS> dwarf_names {
        "Dwarf 1", "Dwarf 2", "Dwarf 3", "Dwarf 4", "Dwarf 5", "Dwarf 6"
    };

    /// Check Dwarf MCU temperature
    constinit std::array<MCUTempErrorChecker, HOTENDS> dwarfMaxTempErrorChecker;
#endif /*HAS_DWARF()*/
#if HAS_MODULARBED()
    constinit MCUTempErrorChecker modbedMaxTempErrorChecker; ///< Check ModularBed MCU temperature
#endif /*HAS_MODULARBED()*/

    void pause_print(Pause_Type type) {
        if (!server.print_is_serial) {
            switch (type) {

            case Pause_Type::Crash:
            case Pause_Type::Repeat_Last_Code:
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

        SerialPrinting::pause();

        print_job_timer.pause();
        HOTEND_LOOP() {
            server.resume.nozzle_temp[e] = marlin_vars().hotend(e).target_nozzle; // save nozzle target temp
        }
        server.resume.nozzle_temp_paused = true;
        server.resume.fan_speed = marlin_vars().print_fan_speed; // save fan speed
        server.resume.print_speed = marlin_vars().print_speed;
#if FAN_COUNT > 0
        if (hotendErrorChecker.runFullFan()) {
            thermalManager.set_fan_speed(0, 255);
        } else {
            thermalManager.set_fan_speed(0, 0); // disable print fan
        }
#endif
    }

    void clear_warnings() {
        if (fsm_states.is_active(ClientFSM::Warning)) {
            fsm_destroy(ClientFSM::Warning);
        }
    }
    void handle_warnings() {
        const auto phase_opt = fsm_states[ClientFSM::Warning];
        if (!phase_opt.has_value()) {
            return;
        }

        const auto phase = static_cast<PhasesWarning>(phase_opt->GetPhase());

        const auto consume_response = [&]() {
            const auto response = get_response_from_phase(phase);
            if (response != Response::_none) {
                const WarningType warning_type = static_cast<WarningType>(*phase_opt->GetData().data());
                clear_warning(warning_type);
            }

            return response;
        };

        switch (phase) {

        case PhasesWarning::Warning:
            consume_response();
            break;

#if XL_ENCLOSURE_SUPPORT()
        case PhasesWarning::EnclosureFilterExpiration:
            if (auto r = consume_response(); r != Response::_none) {
                xl_enclosure.setUpReminder(r);
            }
            break;
#endif

        case PhasesWarning::ProbingFailed:
            switch (consume_response()) {
            case Response::Yes:
                print_resume();
                break;

            case Response::No:
                print_abort();
                break;

            default:
                break;
            }
            break;

        case PhasesWarning::NozzleCleaningFailed:
            switch (consume_response()) {
            case Response::Retry:
                print_resume();
                break;

            case Response::No:
                print_abort();
                break;

            default:
                break;
            }
            break;

        default:
            // Most warnings are handled somewhere else and we shouldn't consume and process the responses
            break;
        }
    }
} // end anonymous namespace

static void commit_fsm_states() {
    ++fsm_states.generation;
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

void fsm_change(FSMAndPhase fsm_and_phase, fsm::PhaseData data) {
    const auto base_data = fsm::BaseData(fsm_and_phase.phase, data);

    if (fsm_states[fsm_and_phase.fsm] != base_data) {
        fsm_states[fsm_and_phase.fsm] = base_data;
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
ServerQueue server_queue;

constexpr EncodedFSMResponse empty_encoded_fsm_response = {
    .response = {},
    .fsm_and_phase = FSMAndPhase(static_cast<ClientFSM>(0xff), 0xff),
};
static EncodedFSMResponse server_side_encoded_fsm_response = empty_encoded_fsm_response;

void _add_status_msg(const char *const popup_msg) {
    // I could check client mask here
    for (size_t i = 0; i < MARLIN_MAX_CLIENTS; ++i) {
        if (server.event_messages[i]) {
            // FIXME: It would be great if we didn't lose messages there.
            //        For now, let's keep the original implementation.
            free(server.event_messages[i]);
        }
        // FIXME: It would be really great if we didn't allocate here.
        //        Maybe we could instead setup some state in marlin_vars_t
        //        For now, let's keep the original implementation.
        server.event_messages[i] = strdup(popup_msg);
    }
}

//-----------------------------------------------------------------------------
// forward declarations of private functions

static void _server_print_loop(void);
static uint64_t _send_notify_events_to_client(int client_id, ClientQueue &queue, uint64_t evt_msk);
static uint8_t _send_notify_event(Event evt_id, uint32_t usr32, uint16_t usr16);
static void _server_update_vars();
static bool _process_server_request(const Request &);
static void _server_set_var(const Request &);

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
        server.event_messages[i] = nullptr;
    }
    server_task = osThreadGetId();
    server.enable_nozzle_temp_timeout = true;
#if HAS_BED_PROBE || HAS_LOADCELL() && ENABLED(PROBE_CLEANUP_SUPPORT)
    server.mbl_failed = false;
#endif
    // Random at boot, to avoid chance of reusing the same (0/1) dialog ID
    // after a reboot.
    fsm_states.generation = rand_u();

    marlin_vars().init();
#if HAS_SHEET_PROFILES()
    SteelSheets::CheckIfCurrentValid();
#endif
    settings_load();
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

#if ENABLED(PRUSA_MMU2)
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
    const auto original_temp = thermalManager.degTargetHotend(active_extruder);
    enqueue_gcode("M702 W2");
    enqueue_gcode_printf("M104 S%i", original_temp);
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

#if HAS_EMERGENCY_STOP()
    buddy::emergency_stop().step();
#endif

#if HAS_XBUDDY_EXTENSION()
    buddy::xbuddy_extension().step();
#endif

    static bool is_nested = false;
    if (is_nested) {
        return;
    }
    AutoRestore _nr(is_nested, true);

    bool call_print_loop = true;
#if HAS_SELFTEST()
    if (SelftestInstance().IsInProgress()) {
        SelftestInstance().Loop();
        call_print_loop = false;
    }
#endif

#if HAS_MMU2()
    MMU2::Fsm::Instance().Loop();
#endif

    handle_warnings();

#if XL_ENCLOSURE_SUPPORT()
    int16_t dwarf_temp = std::numeric_limits<int16_t>().min();
    #if HAS_TOOLCHANGER()
    dwarf_temp = prusa_toolchanger.getActiveToolOrFirst().get_board_temperature();
    #endif
    std::optional<WarningType> notif = xl_enclosure.loop(buddy::puppies::modular_bed.get_mcu_temperature(), dwarf_temp, server.print_state);

    // Filter expiration, expiration warning, 5 day postponed reminder
    if (notif.has_value()) {
        set_warning(*notif); // Notify the GUI about the warning
    }

#endif

    if (call_print_loop) {
        _server_print_loop(); // we need call print loop here because it must be processed while blocking commands (M109)
    }

    FSM_notifier::SendNotification();

    print_fan_spd();

#if HAS_TOOLCHANGER()
    // Check if tool didn't fall off
    prusa_toolchanger.loop(!printer_idle(), printer_paused());
#endif /*HAS_TOOLCHANGER()*/

#if HAS_I2C_EXPANDER()
    io_expander_read_loop();
#endif // HAS_I2C_EXPANDER()

    if (Request request; server_queue.try_receive(request, 0)) {
        _process_server_request(request);
    }

    // update variables
    send_notifications_to_clients();
    server_update_vars();
}

void static finalize_print(bool finished) {
#if ENABLED(POWER_PANIC)
    power_panic::reset();
#endif

    print_job_timer.stop();
    _server_update_vars();
    // Check if the stopwatch was NOT stopped to and add the current printime to the statistics.
    // finalize_print is beeing called multiple times and we don't want to add the time twice.
    if (!server.was_print_time_saved) {
        Odometer_s::instance().add_time(marlin_vars().print_duration);
        server.was_print_time_saved = true;
    }

#if !PRINTER_IS_PRUSA_iX()
    // On iX, we're not cooling down the bed after the print.
    // Resetting bounding rect would result in turning all bedlets on, which we don't want.
    // First - it's increasing power consumption; second - it could clear the bed preheat status.
    // BFW-5085
    print_area.reset_bounding_rect();
#endif

#if ENABLED(PRUSA_TOOL_MAPPING)
    tool_mapper.reset();
    spool_join.reset();
#endif
#if ENABLED(GCODE_COMPATIBILITY_MK3)
    GcodeSuite::gcode_compatibility_mode = GcodeSuite::GcodeCompatibilityMode::NONE;
#endif

#if ENABLED(FAN_COMPATIBILITY_MK4_MK3)
    GcodeSuite::fan_compatibility_mode = GcodeSuite::FanCompatibilityMode::NONE;
#endif
#if HAS_CHAMBER_API()
    buddy::chamber().reset();
#endif
    // Reset IS at the end of the print
    input_shaper::init();

    media_prefetch.stop();

    server.print_is_serial = false; // reset flag about serial print

    marlin_vars().print_end_time = time(nullptr);
    marlin_vars().add_job_result(job_id, finished ? marlin_vars_t::JobInfo::JobResult::finished : marlin_vars_t::JobInfo::JobResult::aborted);

    // Do not remove, needed for 3rd party tools such as octoprint to get status that the gcode file printing has finished
    SERIAL_ECHOLNPGM(MSG_FILE_PRINTED);
}

static const uint8_t MARLIN_IDLE_CNT_BUSY = 1;

#if ANY(CRASH_RECOVERY, POWER_PANIC)
static void check_crash() {
    // reset the nested loop check once per main server iteration
    crash_s.needs_stack_unwind = false;

    #if ENABLED(POWER_PANIC)
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
            || (crash_s.get_state() == Crash_s::TRIGGERED_HOMEFAIL))) {

        // Set again to prevent race when ISR happens during this function
        crash_s.needs_stack_unwind = false;
        server.print_state = State::CrashRecovery_Begin;
        return;
    }
}
#endif // ENABLED(CRASH_RECOVERY)

void loop() {
#if ANY(CRASH_RECOVERY, POWER_PANIC)
    check_crash();
#endif
    if (server.idle_cnt >= MARLIN_IDLE_CNT_BUSY) {
        if (server.flags & MARLIN_SFLG_BUSY) {
            log_debug(MarlinServer, "State: Ready");
            server.flags &= ~MARLIN_SFLG_BUSY;
            if ((Cmd(server.command) != Cmd::NONE) && (Cmd(server.command) != Cmd::M600)) {
                _send_notify_event(Event::CommandEnd, server.command, 0);
                server.command = ftrstd::to_underlying(Cmd::NONE);
            }
        }
    }

    // Revert quick_stop when commands already drained
    if (server.flags & MARLIN_SFLG_STOPPED && !queue.has_commands_queued() && !planner.processing()) {
        planner.resume_queuing();
        server.flags &= ~MARLIN_SFLG_STOPPED;
    }

    server.idle_cnt = 0;
    cycle();

#if HAS_EMERGENCY_STOP()
    // During printing, possibly block anytime
    if (server.print_state == State::Printing) {
        buddy::emergency_stop().maybe_block();
    }
#endif

#if HAS_NFC()
    if (printer_idle() && !fsm_states.get_top().has_value()) {
        handle_nfc();
    }
#endif
}

static bool idle_running = false;

static void idle(void) {
    cycle();

    // cycle -> loop -> idle -> MarlinUI::update() -> ExtUI::onIdle -> idle -> cycle
    // This is only a work-around: this should be avoided at a higher level
    if (idle_running) {
        return;
    }

    AutoRestore _ar(idle_running, true);

    if (server.idle_cnt < MARLIN_IDLE_CNT_BUSY) {
        server.idle_cnt++;
    } else if ((server.flags & MARLIN_SFLG_BUSY) == 0) {

        log_debug(MarlinServer, "State: Busy");
        server.flags |= MARLIN_SFLG_BUSY;
        if (parser.command_letter == 'G') {
            switch (parser.codenum) {
            case 28:
            case 29:
                server.command = ftrstd::to_underlying(Cmd::G) + parser.codenum;
                break;
            }
        } else if (parser.command_letter == 'M') {
            switch (parser.codenum) {
            case 109:
            case 190:
            case 303:
            // case 600: // hacked in gcode (_force_M600_notify)
            case 701:
            case 702:
                server.command = ftrstd::to_underlying(Cmd::M) + parser.codenum;
                break;
            }
        }
        if (Cmd(server.command) != Cmd::NONE) {
            server.command_begin = server.command;
            server.command_end = server.command;
            _send_notify_event(Event::CommandBegin, server.command, 0);
        }
    }

#if HAS_EMERGENCY_STOP()
    // During printing, possibly block anytime
    if (server.print_state == State::Printing) {
        buddy::emergency_stop().maybe_block();
    }
#endif
}

void do_babystep_Z(float offs) {
    babystep.add_steps(Z_AXIS, std::round(offs * planner.settings.axis_steps_per_mm[Z_AXIS]));
    babystep.task();
}

extern void move_axis(float pos, float feedrate, size_t axis) {
    current_position[axis] = pos;
    line_to_current_position(feedrate);
}

void move_xyz_axes_to(xyz_float_t position, float feedrate) {
    current_position = position;
    line_to_current_position(feedrate);
}

bool enqueue_gcode(const char *gcode) {
    return queue.enqueue_one(gcode);
}

bool enqueue_gcode_printf(const char *gcode, ...) {
    char request[MARLIN_MAX_REQUEST];
    va_list ap;
    va_start(ap, gcode);
    const int ret = vsnprintf(request, MARLIN_MAX_REQUEST, gcode, ap);
    va_end(ap);
    enqueue_gcode(request);
    return ret;
}

bool inject(InjectQueueRecord record) {
    if (!queue.inject(record)) {
        // TODO: If requested, figure out thread-safe way to call Sound_Play(eSOUND_TYPE::SingleBeepAlwaysLoud);
        return false;
    }
    return true;
}

static void settings_load() {
    (void)settings.reset();
#if HAS_SHEET_PROFILES()
    probe_offset.z = SteelSheets::GetZOffset();
#endif
#if ENABLED(PIDTEMPBED)
    Temperature::temp_bed.pid.Kp = config_store().pid_bed_p.get();
    Temperature::temp_bed.pid.Ki = config_store().pid_bed_i.get();
    Temperature::temp_bed.pid.Kd = config_store().pid_bed_d.get();
#endif
#if ENABLED(PIDTEMP)
    HOTEND_LOOP() {
        Temperature::temp_hotend[e].pid.Kp = config_store().pid_nozzle_p.get();
        Temperature::temp_hotend[e].pid.Ki = config_store().pid_nozzle_i.get();
        Temperature::temp_hotend[e].pid.Kd = config_store().pid_nozzle_d.get();
    }
    thermalManager.updatePID();
#endif

    marlin_vars().fan_check_enabled = config_store().fan_check_enabled.get();
    marlin_vars().fs_autoload_enabled = config_store().fs_autoload_enabled.get();

    marlin_vars().stealth_mode = config_store().stealth_mode.get();
    planner.set_stealth_mode(config_store().stealth_mode.get());

    job_id = config_store().job_id.get();

#if ENABLED(PRUSA_TOOLCHANGER)
    // TODO: This is temporary until better offset store method is implemented
    prusa_toolchanger.load_tool_offsets();
#endif

#if HAS_PHASE_STEPPING()
    phase_stepping::load();
#endif
}

uint32_t get_command(void) {
    return server.command;
}

void set_command(uint32_t command) {
    server.command = command;
}

void test_start([[maybe_unused]] const uint64_t test_mask, [[maybe_unused]] const selftest::TestData test_data) {
#if HAS_SELFTEST()
    if (((server.print_state == State::Idle) || (server.print_state == State::Finished) || (server.print_state == State::Aborted)) && (!SelftestInstance().IsInProgress())) {
        SelftestInstance().Start(test_mask, test_data);
    }
#endif
}

void test_abort(void) {
#if HAS_SELFTEST()
    if (SelftestInstance().IsInProgress()) {
        SelftestInstance().Abort();
    }
#endif
}

void quick_stop() {
#if HAS_TOOLCHANGER()
    prusa_toolchanger.quick_stop();
#endif
    planner.quick_stop();
    disable_all_steppers();
    set_all_unhomed();
    set_all_unknown();
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
#if HAS_TOOLCHANGER() || HAS_MMU2()
        || server.print_state == State::PrintPreviewToolsMapping
#endif
        || server.print_state == State::WaitGui;
}

bool is_printing() {
    switch (marlin_vars().print_state) {
    case State::Aborted:
    case State::Idle:
    case State::Finished:
    case State::PrintPreviewInit:
    case State::PrintPreviewImage:
#if HAS_TOOLCHANGER() || HAS_MMU2()
    case State::PrintPreviewToolsMapping:
#endif
        return false;
    default:
        return true;
    }
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

void serial_print_start() {
    server.print_state = State::SerialPrintInit;
    print_state = {};
}

void print_start(const char *filename, const GCodeReaderPosition &resume_pos, marlin_server::PreviewSkipIfAble skip_preview) {
#if HAS_SELFTEST()
    if (SelftestInstance().IsInProgress()) {
        return;
    }
#endif
    if (filename == nullptr) {
        return;
    }

    // Clear warnings before print, like heaters disabled after 30 minutes.
    clear_warnings();

    switch (server.print_state) {

        // handle preview / reprint
    case State::Finished:
    case State::Aborted:
        // correctly end previous print
        finalize_print(server.print_state == State::Finished);
        if (fsm_states.is_active(ClientFSM::Printing)) {
            // exit from print screen, if opened
            fsm_destroy(ClientFSM::Printing);
        }
        break;

    case State::Idle:
    case State::PrintPreviewInit:
    case State::PrintPreviewImage:
    case State::PrintPreviewConfirmed:
    case State::PrintPreviewQuestions:
#if HAS_TOOLCHANGER() || HAS_MMU2()
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
        // We need a copy of the sfn as well because get_LFN needs the address mutable :/
        std::array<char, FILE_PATH_BUFFER_LEN> filepath_sfn;
        strlcpy(filepath_sfn.data(), filename, filepath_sfn.size());

        std::array<char, FILE_NAME_BUFFER_LEN> filename_lfn;
        get_LFN(filename_lfn.data(), filename_lfn.size(), filepath_sfn.data());

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

    set_media_position(resume_pos.offset);
    print_state.media_restore_info = resume_pos.restore_info;
    media_prefetch_start();

    server.print_state = State::WaitGui;

    PrintPreview::Instance().set_skip_if_able(skip_preview);
}

void gui_ready_to_print() {
    switch (server.print_state) {

    case State::WaitGui:
        server.print_state = State::PrintPreviewInit;
        break;

    default:
        log_error(MarlinServer, "Wrong print state, expected: %u, is: %u",
            static_cast<unsigned>(State::WaitGui), static_cast<unsigned>(server.print_state));
        break;
    }
}

void gui_cant_print() {
    switch (server.print_state) {

    case State::WaitGui:
        server.print_state = State::Idle;
        break;

    default:
        log_error(MarlinServer, "Wrong print state, expected: %u, is: %u",
            static_cast<unsigned>(State::WaitGui), static_cast<unsigned>(server.print_state));
        break;
    }
}

void serial_print_finalize(void) {
    switch (server.print_state) {

    case State::Printing:
    case State::Paused:
    case State::Resuming_Reheating:
    case State::Finishing_WaitIdle:
#if HAS_TOOLCHANGER()
    case State::CrashRecovery_Tool_Pickup:
#endif
        server.print_state = State::Finishing_WaitIdle;
        break;
    default:
        break;
    }
}

void print_abort(void) {

    switch (server.print_state) {

#if ENABLED(POWER_PANIC)
    case State::PowerPanic_Resume:
    case State::PowerPanic_AwaitingResume:
#endif
    case State::Printing:
    case State::Paused:
    case State::Resuming_Reheating:
    case State::Finishing_WaitIdle:
#if HAS_TOOLCHANGER()
    case State::CrashRecovery_Tool_Pickup:
#endif
        server.print_state = State::Aborting_Begin;
        break;

    case State::PrintPreviewInit:
    case State::PrintPreviewImage:
    case State::PrintPreviewConfirmed:
    case State::PrintPreviewQuestions:
#if HAS_TOOLCHANGER() || HAS_MMU2()
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

#if ENABLED(POWER_PANIC)
    case State::PowerPanic_Resume:
    case State::PowerPanic_AwaitingResume:
#endif
    case State::Printing:
    case State::Paused:
    case State::Resuming_Reheating:
    case State::Finishing_WaitIdle:
        // do nothing
        break;

    default:
        server.print_state = State::Exit;
        break;
    }
}

void print_pause(void) {
    print_state.resume_pending = false;

    if (server.print_state == State::Printing) {
        server.print_state = State::Pausing_Begin;
    }
}

/**
 * @brief Restore paused nozzle temperature to enable filament change
 */
void unpause_nozzle(const uint8_t extruder) {
    thermalManager.setTargetHotend(server.resume.nozzle_temp[extruder], extruder);
    set_temp_to_display(server.resume.nozzle_temp[extruder], extruder);
    server.resume.nozzle_temp_paused = false;
}

#if ENABLED(CRASH_RECOVERY)
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

    #if HAS_TOOLCHANGER()
/**
 * @brief Deselect tool, disable XY steppers and switch to Tool_Pickup server print_state.
 */
static void prepare_tool_pickup() {
    prusa_toolchanger.crash_deselect_dwarf(); // Deselect dwarf as if all were parked
    disable_XY(); // Let user move the carriage

    // Disable heaters
    HOTEND_LOOP() {
        if ((marlin_vars().hotend(e).target_nozzle > 0)) {
            thermalManager.setTargetHotend(0, e);
            set_temp_to_display(0, e);
        }
    }

    server.print_state = State::CrashRecovery_Tool_Pickup; // Continue with screen to wait for user to pick tools
}

/**
 * @brief Part of crash recovery begin when reason of crash is the toolchanger.
 * @note This has to call fsm_create() exactly once.
 * @return true on toolcrash when there is no parking and replay and when should break current switch case
 */
static bool crash_recovery_begin_toolchange() {
    Crash_recovery_tool_fsm cr_fsm(prusa_toolchanger.get_enabled_mask(), 0);
    fsm_create(PhasesCrashRecovery::tool_recovery, cr_fsm.Serialize()); // Ask user to park all dwarves

    if (crash_s.get_state() == Crash_s::REPEAT_WAIT) {
        prepare_tool_pickup(); // If crash happens during toolchange, skip crash recovery and go directly to tool pickup
        return true;
    }
    return false;
}
    #endif /*HAS_TOOLCHANGER()*/

/**
 * @brief Part of crash recovery begin when reason of crash is failed homing.
 * @note This has to call fsm_create() exactly once.
 * @note Should break current switch case after this.
 */
static void crash_recovery_begin_home() {
    Crash_recovery_fsm cr_fsm(SelftestSubtestState_t::running, SelftestSubtestState_t::undef);
    fsm_create(PhasesCrashRecovery::home, cr_fsm.Serialize());

    measure_axes_and_home(); // If crash happens during homing, skip crash recovery and go directly to measuring axes / homing
}

    #if ENABLED(AXIS_MEASURE)
/**
 * @brief Part of crash recovery begin when it is a regular crash, axis measure is enabled and this is a repeated crash.
 * @note This has to call fsm_create() exactly once.
 * @note Do not break current switch case after this, will park and replay.
 */
static void crash_recovery_begin_axis_measure() {
    Crash_recovery_fsm cr_fsm(SelftestSubtestState_t::running, SelftestSubtestState_t::undef);
    fsm_create(PhasesCrashRecovery::check_X, cr_fsm.Serialize()); // check axes first
}
    #endif /*ENABLED(AXIS_MEASURE)*/

/**
 * @brief Part of crash recovery begin when it is a regular crash.
 * @note This has to call fsm_create() exactly once.
 * @note Do not break current switch case after this, will park and replay.
 */
static void crash_recovery_begin_crash() {
    Crash_recovery_fsm cr_fsm(SelftestSubtestState_t::running, SelftestSubtestState_t::undef);
    fsm_create(PhasesCrashRecovery::home, cr_fsm.Serialize());
}
#endif /*ENABLED(CRASH_RECOVERY)*/

void media_prefetch_start() {
    print_state.file_open_reported = false;
    media_prefetch.start(marlin_vars().media_SFN_path.get_ptr(), GCodeReaderPosition { stream_restore_info(), media_position() });
    media_prefetch.issue_fetch();
}

void media_print_loop() {
    /// Size of the gcode queue
    METRIC_DEF(metric_gcode_queue_size, "gcd_que_sz", METRIC_VALUE_INTEGER, 100, METRIC_ENABLED);
    metric_record_integer(&metric_gcode_queue_size, queue.length);

    while (queue.length < MEDIA_FETCH_GCODE_QUEUE_FILL_TARGET) {
        MediaPrefetchManager::ReadResult data;
        using Status = MediaPrefetchManager::Status;
        const Status status = media_prefetch.read_command(data);
        const auto metrics = media_prefetch.get_metrics();

        /// Status of the last media_prefetch.read_command. 0 = ok, 1 = end of file, other = error (means that we're stalling)
        METRIC_DEF(metric_fetch_status, "ftch_status", METRIC_VALUE_INTEGER, 100, METRIC_ENABLED);
        metric_record_integer(&metric_fetch_status, static_cast<int>(status));

        /// Status at the end of the buffer - for early error indication
        METRIC_DEF(metric_fetch_tail_status, "ftch_tstatus", METRIC_VALUE_INTEGER, 100, METRIC_ENABLED);
        metric_record_integer(&metric_fetch_tail_status, static_cast<int>(metrics.tail_status));

        /// Occupancy of the media prefetch buffer, in percent of the buffer size
        METRIC_DEF(metric_prefetch_buffer_occupancy, "ftch_occ", METRIC_VALUE_INTEGER, 100, METRIC_ENABLED);
        metric_record_integer(&metric_prefetch_buffer_occupancy, metrics.buffer_occupancy_percent);

        /// Number of commands in the prefetch buffer
        METRIC_DEF(metric_prefetch_buffer_commands, "ftch_cmds", METRIC_VALUE_INTEGER, 100, METRIC_ENABLED);
        metric_record_integer(&metric_prefetch_buffer_commands, metrics.commands_in_buffer);

        // To-do: automatic unpause when paused if the condition fixes itself?
        const auto media_error = [](WarningType warning_type) {
            set_warning(warning_type);
            print_state.paused_due_to_media_error = true;
            print_pause();
        };

        const auto clear_media_error = [] {
            if (!print_state.paused_due_to_media_error) {
                return;
            }

            print_state.paused_due_to_media_error = false;

            clear_warning(WarningType::USBFlashDiskError);
            clear_warning(WarningType::GcodeCorruption);
            clear_warning(WarningType::NotDownloaded);
        };

        if (!print_state.file_open_reported && metrics.stream_size_estimate) {
            print_state.file_open_reported = true;

            // Do not remove, needed for 3rd party tools such as octoprint to get status about the gcode file being opened
            SERIAL_ECHOLNPAIR(MSG_SD_FILE_OPENED, marlin_vars().media_SFN_path.get_ptr(), " Size:", metrics.stream_size_estimate);
        }

        switch (status) {

        case Status::ok:
            if (print_state.skip_gcode) {
                print_state.skip_gcode = false;
                continue;
            }

            clear_media_error();

            print_state.media_restore_info = data.replay_pos.restore_info;
            queue.sdpos = data.replay_pos.offset;
            queue.enqueue_one(data.gcode.data(), false);
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
            media_error(WarningType::USBFlashDiskError);
            return;

        case Status::corruption:
            media_error(WarningType::GcodeCorruption);
            return;

        case Status::not_downloaded:
            media_error(WarningType::NotDownloaded);
            return;
        }
    }
}

/// Update SFN filepath from LFN.
/// The SFN of the file could have been changed by the user during the pause (for example by re-uploading a damaged file).
/// BFW-5775
void update_sfn() {
    // Copy the current SFN + LFN from marlin vars
    MutablePath filepath_sfn;
    marlin_vars().media_SFN_path.copy_to(filepath_sfn.get_buffer(), filepath_sfn.maximum_length());
    log_info(MarlinServer, "Old SFN: %s", filepath_sfn.get());

    // Pop filename, leave path only
    filepath_sfn.pop();

    // This is done on the marlin thread, so we can keep using the pointer
    const char *lfn = marlin_vars().media_LFN.get_ptr();

    DIR *dir = opendir(filepath_sfn.get());
    if (!dir) {
        return;
    }
    ScopeGuard dir_guard([&] { closedir(dir); });

    struct dirent *ent;
    while ((ent = readdir(dir))) {
        if ((strcasecmp(ent->d_name, lfn) == 0) || (strcasecmp(ent->lfn, lfn) == 0)) {
            break;
        }
    }
    if (!ent) {
        return;
    }

    // Store the new SFN
    filepath_sfn.push(ent->d_name);
    log_info(MarlinServer, "New SFN: %s", filepath_sfn.get());

    // Update the relevant variables
    marlin_vars().media_SFN_path.set(filepath_sfn.get());
    GCodeInfo::getInstance().set_gcode_file(filepath_sfn.get(), lfn);
}

void print_resume(void) {
    if (server.print_state == State::Paused) {
        update_sfn();

        server.print_state = State::Resuming_Begin;

        // pause queuing commands from serial, until resume sequence is finished.
        GCodeQueue::pause_serial_commands = true;

    } else if (is_resuming_state(server.print_state)) {
        // Do nothing

    } else if (is_pausing_state(server.print_state)) {
        print_state.resume_pending = true;

#if ENABLED(POWER_PANIC)
    } else if (server.print_state == State::PowerPanic_AwaitingResume) {
        power_panic::resume_continue();
        server.print_state = State::PowerPanic_Resume;
#endif
    } else {
        print_start(nullptr, GCodeReaderPosition(), marlin_server::PreviewSkipIfAble::all);
    }
}

void try_recover_from_media_error() {
    if (server.print_state == State::Printing) {
        // If we're printing, simply try issuing a fetch to make sure everything's fine
        media_prefetch.issue_fetch();

    } else if (print_state.paused_due_to_media_error) {
        // Do NOT reset - will be reset if the resume is successful
        // print_state.paused_due_to_media_error = false;
        print_resume();
    }
}

// Fast temperature recheck.
// Does not check stability of the temperature.
bool print_reheat_ready() {
    // check nozzles
    HOTEND_LOOP() {
        auto &extruder = marlin_vars().hotend(e);
        if (extruder.target_nozzle != server.resume.nozzle_temp[e] || (extruder.target_nozzle > 0 && extruder.temp_nozzle < (extruder.target_nozzle - TEMP_HYSTERESIS))) {
            return false;
        }
    }

    // check bed
    if (marlin_vars().temp_bed < (marlin_vars().target_bed - TEMP_BED_HYSTERESIS)) {
        return false;
    }

    return true;
}

#if ENABLED(POWER_PANIC)
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
    #endif /*HAS_TOOLCHANGER()*/
#endif /*ENABLED(POWER_PANIC)*/

#if ENABLED(AXIS_MEASURE)
enum class Axis_length_t {
    shorter,
    longer,
    ok,
};

static Axis_length_t axis_length_ok(AxisEnum axis) {
    #if HAS_SELFTEST()
    const float len = server.axis_length.pos[axis];

    switch (axis) {
    case X_AXIS:
        return len < selftest::Config_XAxis.length_min ? Axis_length_t::shorter : (len > selftest::Config_XAxis.length_max ? Axis_length_t::longer : Axis_length_t::ok);
    case Y_AXIS:
        return len < selftest::Config_YAxis.length_min ? Axis_length_t::shorter : (len > selftest::Config_YAxis.length_max ? Axis_length_t::longer : Axis_length_t::ok);
    default:;
    }
    return Axis_length_t::shorter;
    #else
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
    server.axis_length.pos[X_AXIS] = (selftest::Config_XAxis.length_min + selftest::Config_XAxis.length_max) / 2;
    server.axis_length.pos[Y_AXIS] = (selftest::Config_YAxis.length_min + selftest::Config_YAxis.length_max) / 2;
}

void set_axes_length(xy_float_t xy) {
    server.axis_length = xy;
}
#endif // ENABLED(AXIS_MEASURE)

static const uint8_t PAUSE_NOZZLE_TIMEOUT = 45; // nozzle "sleep" after 45s inside paused state

void nozzle_timeout_on() {
    server.enable_nozzle_temp_timeout = true;
};
void nozzle_timeout_off() {
    server.enable_nozzle_temp_timeout = false;
}
void nozzle_timeout_loop() {
    if ((ticks_ms() - server.paused_ticks > (1000 * PAUSE_NOZZLE_TIMEOUT)) && server.enable_nozzle_temp_timeout) {
        HOTEND_LOOP() {
            if ((marlin_vars().hotend(e).target_nozzle > 0)) {
                thermalManager.setTargetHotend(0, e);
                set_temp_to_display(0, e);
            }
        }
    }
}

// Checking valid behaviour of Heatbreak fan & Print fan of currently active extruder/tool
bool fan_checks() {
    if (marlin_vars().fan_check_enabled
#if HAS_TOOLCHANGER()
        && prusa_toolchanger.is_any_tool_active() // Nothing to check
#endif /*HAS_TOOLCHANGER()*/
    ) {
        // Allow fan check only if fan had time to build up RPM (after CFanClt::rpm_stabilization)
        // CFanClt error states are checked in the end of each _server_print_loop()
        auto check_fan = [](CFanCtlCommon &fan, const char *fan_name) {
            const auto fan_state = fan.getState();
            if ((fan_state == CFanCtlCommon::FanState::running || fan_state == CFanCtlCommon::FanState::error_running || fan_state == CFanCtlCommon::FanState::error_starting) && !fan.getRPMIsOk()) {
                log_error(MarlinServer, "%s FAN RPM is not OK - Actual: %d rpm, PWM: %d",
                    fan_name,
                    (int)fan.getActualRPM(),
                    fan.getPWM());
                return true;
            }
            return false;
        };

        bool fan_failed = false;
#if !PRINTER_IS_PRUSA_iX()
        fan_failed |= check_fan(Fans::heat_break(active_extruder), "Heatbreak");
#endif
        fan_failed |= check_fan(Fans::print(active_extruder), "Print");
        return fan_failed;
    }
    return false;
}

static void resuming_reheating() {
    if (hotendErrorChecker.isFailed()) {
        set_warning(WarningType::HotendTempDiscrepancy);
        thermalManager.setTargetHotend(0, 0);
#if FAN_COUNT > 0
        thermalManager.set_fan_speed(0, 255);
#endif
        server.print_state = State::Paused;
    }

    if (fan_checks()) {
        server.print_state = State::Paused;
        return;
    }

    // Check if nozzles are being reheated
    HOTEND_LOOP() {
        if (thermalManager.degTargetHotend(e) == 0 && server.resume.nozzle_temp[e] > 0) {
            // Stopped reheating, can happen if there is an error during reheating
            server.print_state = State::Paused;
            return;
        }
    }

    if (!print_reheat_ready()) {
        return;
    }

#if HAS_BED_PROBE || HAS_LOADCELL() && ENABLED(PROBE_CLEANUP_SUPPORT)
    // There's homing after MBL fail so no need to unpark at all
    if (server.mbl_failed) {
        server.print_state = State::Resuming_UnparkHead_ZE;
        return;
    }
#endif

#if ENABLED(CRASH_RECOVERY)
    if (crash_s.get_state() == Crash_s::REPEAT_WAIT) {
        server.print_state = State::Resuming_UnparkHead_ZE; // Skip unpark when recovering from toolcrash or homing fail
        return;
    }
#endif /*ENABLED(CRASH_RECOVERY)*/

    unpark_head_XY();
    server.print_state = State::Resuming_UnparkHead_XY;
}

static void _server_print_loop(void) {
    static bool did_not_start_print = true, abort_resuming = false;
    switch (server.print_state) {
    case State::Idle:
        break;
    case State::WaitGui:
        // without gui just act as if state == State::PrintPreviewInit
#if HAS_GUI()
        break;
#endif
    case State::PrintPreviewInit:
        did_not_start_print = true;
        // reset both percentage counters (normal and silent)
        oProgressData.standard_mode.percent_done.mSetValue(0, 0);
        oProgressData.stealth_mode.percent_done.mSetValue(0, 0);
        PrintPreview::Instance().Init();
        server.print_state = State::PrintPreviewImage;
        break;

    case State::PrintPreviewImage:
    case State::PrintPreviewConfirmed:
#if HAS_TOOLCHANGER() || HAS_MMU2()
    case State::PrintPreviewToolsMapping:
#endif
    case State::PrintPreviewQuestions: {
        // button evaluation
        // We don't particularly care about the
        // difference, but downstream users do.

        auto old_state = server.print_state;
        auto new_state = old_state;
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

            new_state = State::PrintPreviewConfirmed;
            break;

        case PrintPreview::Result::Image:
            new_state = State::PrintPreviewImage;
            break;

        case PrintPreview::Result::Questions:
            new_state = State::PrintPreviewQuestions;
            break;

        case PrintPreview::Result::Abort:
            new_state = did_not_start_print ? State::Idle : State::Finishing_WaitIdle;
            if (did_not_start_print) {
                // Saving the result for connect, we already send the job id to them at this point.
                marlin_vars().add_job_result(job_id, marlin_vars_t::JobInfo::JobResult::aborted);
            }
            media_prefetch.stop();
            fsm_destroy(ClientFSM::PrintPreview);
            break;

#if HAS_TOOLCHANGER() || HAS_MMU2()
        case PrintPreview::Result::ToolsMapping:
            new_state = State::PrintPreviewToolsMapping;
            break;
#endif

        case PrintPreview::Result::Print:
        case PrintPreview::Result::Inactive:
            did_not_start_print = false;
            new_state = State::PrintInit;

#if HAS_TOOLCHANGER()
            if (prusa_toolchanger.is_toolchanger_enabled()) {
                // Handle singletool G-code which doesn't have T commands in it
                if (GCodeInfo::getInstance().is_singletool_gcode()) {
                    enqueue_gcode("T0 S1 D0"); // Pick tool 0 (can be remapped to anything) before print
                }
            }
#endif /*HAS_TOOLCHANGER()*/
#if HAS_MMU2()
            if (MMU2::mmu2.Enabled() && GCodeInfo::getInstance().is_singletool_gcode() && MMU2::mmu2.get_current_tool() == MMU2::FILAMENT_UNKNOWN) {
                // POC: Handle singletool G-code which doesn't have T commands in it
                // In case we don't have other filament loaded!
                // Unfortunately we don't have the nozzle heated, an ugly workaround is to enqueue an M109 :(

                const auto preheat_temp = GCodeInfo::getInstance().get_hotend_preheat_temp().value_or(215);
                enqueue_gcode_printf("M109 S%i", preheat_temp); // speculatively, use PLA temp for MMU prints, anything else is highly unprobable at this stage
                enqueue_gcode("T0"); // tool change T0 (can be remapped to anything)
                enqueue_gcode("G92 E0"); // reset extruder position to 0

                bool is_relative = gcode.axis_is_relative(AxisEnum::E_AXIS);

                enqueue_gcode("M82"); // set E to absolute positions
    #if HAS_LOADCELL()
                enqueue_gcode("G1 E25 F1860"); // push filament into the nozzle - load distance from fsensor into nozzle tuned (hardcoded) for now
                enqueue_gcode("G1 E35 F300"); // slowly push another 10mm (absolute E)
    #else
                enqueue_gcode("G1 E50 F1860"); // push filament into the nozzle - load distance from fsensor into nozzle tuned (hardcoded) for now
                enqueue_gcode("G1 E62 F300"); // slowly push another 12mm (absolute E)
    #endif
                if (is_relative) {
                    enqueue_gcode("M83"); // set E back to relative positions
                }

                // In case of need, we can perform a custom purge line from the other end of the heatbed
                // It would require homing the axes first, moving to [maxx-10, -4] and slowly purging while moving towards the origin
            }
#endif
            break;
        }

        server.print_state = new_state;

        break;
    }
    case State::PrintInit:
        server.print_is_serial = false;
        server.was_print_time_saved = false;
        feedrate_percentage = 100;

        // Reset flow factor for all extruders
        HOTEND_LOOP() {
            planner.flow_percentage[e] = 100;
            planner.refresh_e_factor(e);
        }

#if ENABLED(PRUSA_TOOL_MAPPING) && (HOTENDS > 1)
        // Cooldown unused tools
        // Ignore spool join - spool joined tools will get heated as spool join is activated
        // BFW-5996
        for (uint8_t physical_tool = 0; physical_tool < HOTENDS; physical_tool++) {
            if (tool_mapper.to_gcode(physical_tool) == tools_mapping::no_tool) {
                thermalManager.setTargetHotend(0, physical_tool);
            }
        }
#endif

#if ENABLED(CRASH_RECOVERY)
        crash_s.reset();
        crash_s.counters.reset();
        endstops.enable_globally(true);
        crash_s.set_state(Crash_s::PRINTING);
#endif // ENABLED(CRASH_RECOVERY)

#if ENABLED(CANCEL_OBJECTS)
        cancelable.reset();
        for (auto &cancel_object_name : marlin_vars().cancel_object_names) {
            cancel_object_name.set(""); // Erase object names
        }
#endif

#if HAS_LOADCELL()
        // Reset Live-Adjust-Z value before every print
        probe_offset.z = 0;
        marlin_vars().z_offset = 0;
#endif // HAS_LOADCELL()

        print_job_timer.start();
        marlin_vars().time_to_end = TIME_TO_END_INVALID;
        marlin_vars().time_to_pause = TIME_TO_END_INVALID;
        marlin_vars().print_start_time = time(nullptr);
        server.print_state = State::Printing;
        if (fsm_states.is_active(ClientFSM::PrintPreview)) {
            fsm_destroy_and_create(ClientFSM::PrintPreview, ClientFSM::Printing, fsm::BaseData());
        }
        if (!fsm_states.is_active(ClientFSM::Printing)) {
            // FIXME make this atomic change. It would require improvements in PrintScreen so that it can re-initialize upon phase change.
            // FYI the DESTROY invoke is in print_start()
            // NOTE this works surely thanks to State::WaitGui being in between the DESTROY and CREATE
            fsm_create(PhasesPrinting::active);
        }
#if HAS_BED_PROBE || HAS_LOADCELL() && ENABLED(PROBE_CLEANUP_SUPPORT)
        server.mbl_failed = false;
#endif
        break;
    case State::SerialPrintInit:
        server.print_is_serial = true;
        server.was_print_time_saved = false;
#if ENABLED(CRASH_RECOVERY)
        crash_s.reset();
        crash_s.counters.reset();
        endstops.enable_globally(true);
        // Crash Detection is disabled during serial printing, because it does not work
        // crash_s.set_state(Crash_s::PRINTING);
#endif // ENABLED(CRASH_RECOVERY)
#if ENABLED(CANCEL_OBJECTS)
        cancelable.reset();
        for (auto &cancel_object_name : marlin_vars().cancel_object_names) {
            cancel_object_name.set(""); // Erase object names
        }
#endif
        print_job_timer.start();
        marlin_vars().print_start_time = time(nullptr);
        fsm_create(PhasesSerialPrinting::active);
        server.print_state = State::Printing;
        break;

    case State::Printing:
        print_state.resume_pending = false;

        if (server.print_is_serial) {
            SerialPrinting::print_loop();
        } else {
            media_print_loop();
        }
        break;

    case State::Pausing_Begin:
        pause_print();
        [[fallthrough]];
    case State::Pausing_Failed_Code:
        server.print_state = State::Pausing_WaitIdle;
        break;
    case State::Pausing_WaitIdle:
        if (!queue.has_commands_queued() && !planner.processing() && gcode.busy_state == GcodeSuite::NOT_BUSY) {
            park_head();
            server.print_state = State::Pausing_ParkHead;
        }
        break;
    case State::Pausing_ParkHead:
        if (!planner.processing()) {
            server.paused_ticks = ticks_ms(); // time when printing paused
            server.print_state = State::Paused;
        }
        break;
    case State::Paused:
        nozzle_timeout_loop();
        gcode.reset_stepper_timeout(); // prevent disable axis
        // resume queuing serial commands (to be able to resume)
        GCodeQueue::pause_serial_commands = false;

        if (print_state.resume_pending) {
            print_state.resume_pending = false;
            print_resume();
        }
        break;
    case State::Resuming_Begin:
#if ENABLED(CRASH_RECOVERY)
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
    case State::Resuming_UnparkHead_XY:
        if (fan_checks()) {
            abort_resuming = true;
        }
        if (planner.processing()) {
            break;
        }
        unpark_head_ZE();
        server.print_state = State::Resuming_UnparkHead_ZE;
        break;
    case State::Resuming_UnparkHead_ZE:
        if (fan_checks()) {
            abort_resuming = true;
        }

        if (queue.has_commands_queued() || planner.processing()) {
            break;
        }
#if ENABLED(CRASH_RECOVERY)
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
            assert(server.print_is_serial || crash_s.get_state() == Crash_s::PRINTING);
        }
#endif
#if HAS_BED_PROBE || HAS_LOADCELL() && ENABLED(PROBE_CLEANUP_SUPPORT)
        if (server.mbl_failed) {
            gcode.process_subcommands_now_P("G28");
            server.mbl_failed = false;
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
#if FAN_COUNT > 0
        thermalManager.set_fan_speed(0, server.resume.fan_speed); // restore fan speed
#endif
        feedrate_percentage = server.resume.print_speed;
        SerialPrinting::resume();
        server.print_state = State::Printing;
        break;

    case State::Aborting_Begin:
#if ENABLED(CRASH_RECOVERY)
        if (crash_s.is_toolchange_in_progress()) {
            break; // Wait for toolchange to end
        }
#endif /*ENABLED(CRASH_RECOVERY)*/
        if (Cmd(server.command) == Cmd::G28) {
            break; // Wait for homing to end
        }

        // Unstuck any operation that is skippable
        skippable_gcode().request_skip();

        media_prefetch.stop();
        queue.clear();

        print_job_timer.stop();
        planner.quick_stop();
        wait_for_heatup = false; // This is necessary because M109/wait_for_hotend can be in progress, we need to abort it

#if ENABLED(CRASH_RECOVERY)
        // TODO: the following should be moved to State::Aborting_ParkHead once the "stopping"
        // state is handled properly
        endstops.enable_globally(false);
        crash_s.counters.save_to_eeprom();
        server.aborting_did_crash_trigger = crash_s.did_trigger(); // Remember as it is cleared by crash_s.reset()
        crash_s.reset();
#endif // ENABLED(CRASH_RECOVERY)

        server.print_state = State::Aborting_WaitIdle;
        break;
    case State::Aborting_WaitIdle:
        if (queue.has_commands_queued() || planner.processing()) {
            break;
        }

        // allow movements again
        planner.resume_queuing();
        if (server.print_is_serial) {
            // will enqueue gcode that will send abort to print host
            SerialPrinting::abort();
        }
        set_current_from_steppers();
        sync_plan_position();
        report_current_position();

#if HAS_EMERGENCY_STOP()
        if (!buddy::emergency_stop().in_emergency()) {
#else
        {
#endif
            if (axes_need_homing()
#if ENABLED(CRASH_RECOVERY)
                || server.aborting_did_crash_trigger
#endif /*ENABLED(CRASH_RECOVERY)*/
            )
                lift_head(); // It would be dangerous to move XY
            else {
                park_head();
            }
        }

        thermalManager.disable_all_heaters();
#if FAN_COUNT > 0
        thermalManager.set_fan_speed(0, 0);
#endif
        HOTEND_LOOP() {
            set_temp_to_display(0, e);
        }

        server.print_state = State::Aborting_UnloadFilament;
        break;

    case State::Aborting_UnloadFilament:
        if (!queue.has_commands_queued() && !planner.processing()) {
#if ENABLED(PRUSA_MMU2)
            if (MMU2::mmu2.Enabled()) {
                safely_unload_filament_from_nozzle_to_mmu();
            }
#endif
            server.print_state = State::Aborting_ParkHead;
        }
        break;
    case State::Aborting_ParkHead:
        if (!queue.has_commands_queued() && !planner.processing()) {
            disable_XY();
#ifndef Z_ALWAYS_ON
            disable_Z();
#endif // Z_ALWAYS_ON
            disable_e_steppers();
            server.print_state = State::Aborted;
            if (server.print_is_serial) {
                fsm_destroy(ClientFSM::Serial_printing);
            }
            finalize_print(false);
        }
        break;
    case State::Aborting_Preview:
        // Wait for operations to finish
        if (queue.has_commands_queued() || planner.processing()) {
            break;
        }

#if HAS_TOOLCHANGER() || HAS_MMU2()
        if (PrintPreview::Instance().GetState() == PrintPreview::State::tools_mapping_wait_user) {
            PrintPreview::tools_mapping_cleanup();
        }
#endif

        // Can go directly to Idle because we didn't really start printing.
        server.print_state = State::Idle;
        PrintPreview::Instance().ChangeState(IPrintPreview::State::inactive);
        fsm_destroy(ClientFSM::PrintPreview);
        media_prefetch.stop();
        break;

    case State::Finishing_WaitIdle:
        if (!queue.has_commands_queued() && !planner.processing()) {
#if ENABLED(CRASH_RECOVERY)
            // TODO: the following should be moved to State::Finishing_ParkHead once the "stopping"
            // state is handled properly
            endstops.enable_globally(false);
            crash_s.counters.save_to_eeprom();
            crash_s.reset();
#endif // ENABLED(CRASH_RECOVERY)

            // ! Must be before the park_head(), otherwise the head parking is still considered a print state
            server.print_state = State::Finishing_UnloadFilament;

#ifdef PARK_HEAD_ON_PRINT_FINISH
            if (!server.print_is_serial) {
                // do not move head if printing via serial
                park_head();
            }
#endif // PARK_HEAD_ON_PRINT_FINISH
        }
        break;
    case State::Finishing_UnloadFilament:
        if (!queue.has_commands_queued() && !planner.processing()) {
#if ENABLED(PRUSA_MMU2)
            if (MMU2::mmu2.Enabled() && GCodeInfo::getInstance().is_singletool_gcode()) {
                // When we are running single-filament gcode with MMU, we should unload current filament.
                safely_unload_filament_from_nozzle_to_mmu();
            }
#endif // ENABLED(PRUSA_MMU2)
            server.print_state = State::Finishing_ParkHead;
        }
        break;
    case State::Finishing_ParkHead:
        if (!queue.has_commands_queued() && !planner.processing()) {
            server.print_state = State::Finished;
            if (server.print_is_serial) {
                fsm_destroy(ClientFSM::Serial_printing);
            }
            finalize_print(true);
        }
        break;
    case State::Exit:
        // make the State::Exit state more resilient to repeated calls (e.g. USB drive pulled out prematurely at the end-of-print screen)
        if (fsm_states.is_active(ClientFSM::Printing)) {
            finalize_print(false);
            fsm_destroy(ClientFSM::Printing);
        }
        if (fsm_states.is_active(ClientFSM::Serial_printing)) {
            finalize_print(false);
        }

        media_prefetch.stop();
        server.print_state = State::Idle;
        break;

#if ENABLED(CRASH_RECOVERY)
    case State::CrashRecovery_Begin: {
        // pause and set correct resume position: this will stop media reading and clear the queue
        // TODO: this is completely broken for crashes coming from serial printing
        pause_print(Pause_Type::Crash);
        set_media_position(crash_s.sdpos);

        endstops.enable_globally(false);
        crash_s.send_reports();
        crash_s.count_crash();
        if (crash_s.get_state() == Crash_s::TRIGGERED_TOOLCRASH || crash_s.get_state() == Crash_s::TRIGGERED_HOMEFAIL) {
            crash_s.set_state(Crash_s::REPEAT_WAIT);
        } else {
            crash_s.set_state(Crash_s::RECOVERY);
        }

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

    #if HAS_TOOLCHANGER()
        else if (crash_s.is_toolchange_event()) {
            if (crash_recovery_begin_toolchange()) {
                break; // Skip crash recovery and go directly to toolchange
            }
        }
    #endif /*HAS_TOOLCHANGER()*/

        else if (crash_s.get_state() == Crash_s::REPEAT_WAIT) { // REPEAT_WAIT could be toolfall, but it was handled above
            crash_recovery_begin_home();
            break; // Skip crash recovery and go directly to homing
        }

    #if ENABLED(AXIS_MEASURE)
        else if (crash_s.is_repeated_crash()) {
            crash_recovery_begin_axis_measure();
        }
    #endif /*ENABLED(AXIS_MEASURE)*/

        else { // All toolfalls, crashes and homing fails are handled above, only regular crash remains
            crash_recovery_begin_crash();
        }

        // save the current resume position
        server.resume.pos = current_position;

    #if ENABLED(ADVANCED_PAUSE_FEATURE)
        /// retract and save E stepper position
        retract();
    #endif // ENABLED(ADVANCED_PAUSE_FEATURE)

        server.print_state = State::CrashRecovery_Retracting;
        break;
    }
    #if HAS_TOOLCHANGER()
    case State::CrashRecovery_ToolchangePowerPanic: {
        // server.resume.nozzle_temp is already configured by powerpanic
        server.resume.nozzle_temp_paused = true; // Nozzle temperatures are stored in server.resume
        endstops.enable_globally(false);
        crash_recovery_begin_toolchange(); // Also sets server.print_state
        break;
    }
    #endif /*HAS_TOOLCHANGER()*/
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

    #if HAS_TOOLCHANGER()
        if (crash_s.is_toolchange_event()) {
            prepare_tool_pickup(); // Go to tool pickup instead of homing
            break;
        }
    #endif /*HAS_TOOLCHANGER()*/

        measure_axes_and_home();
        break;
    }
    case State::CrashRecovery_XY_Measure: {
        if (queue.has_commands_queued() || planner.processing()) {
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
    #if HAS_TOOLCHANGER()
    case State::CrashRecovery_Tool_Pickup: {
        if (queue.has_commands_queued() || planner.processing()) {
            break;
        }

        if ((marlin_server::get_response_from_phase(PhasesCrashRecovery::tool_recovery) == Response::Continue)
            && (prusa_toolchanger.get_enabled_mask() == prusa_toolchanger.get_parked_mask())) {

            // Show homing screen, TODO: perhaps a new screen would be better
            Crash_recovery_fsm cr_fsm(SelftestSubtestState_t::running, SelftestSubtestState_t::undef);
            fsm_change(PhasesCrashRecovery::home, cr_fsm.Serialize());

            // Pickup lost tool
            tool_return_t return_type = tool_return_t::no_return; // If it continues with replay, no need to return
            xyz_pos_t return_pos = current_position; //                              return Z to current Z
            if (crash_s.get_state() == Crash_s::REPEAT_WAIT) {
                // After toolcrash, return to what was requested before the crash
                return_pos = prusa_toolchanger.get_precrash().return_pos;
                toNative(return_pos); // Needs to be modified in place, stored in logical coordinates
                return_type = prusa_toolchanger.get_precrash().return_type;
            }
            if (!prusa_toolchanger.tool_change(prusa_toolchanger.get_precrash().tool_nr,
                    return_type,
                    return_pos,
                    tool_change_lift_t::no_lift,
                    /*z_return =*/true)) {
                if (crash_s.get_state() == Crash_s::TRIGGERED_AC_FAULT) {
                    break; // Powerpanic, do not retry just end
                }

                // Toolchange failed again, ask user again to park all dwarves
                crash_s.count_crash(); // Count as another crash
                Crash_recovery_tool_fsm cr_fsm(prusa_toolchanger.get_enabled_mask(), 0);
                fsm_change(PhasesCrashRecovery::tool_recovery, cr_fsm.Serialize());

                prepare_tool_pickup();
                break;
            }

            server.print_state = State::CrashRecovery_XY_HOME; // Reheat and resume, unpark is skipped in later stages
        } else {
            Crash_recovery_tool_fsm cr_fsm(prusa_toolchanger.get_enabled_mask(), prusa_toolchanger.get_parked_mask());
            fsm_change(PhasesCrashRecovery::tool_recovery, cr_fsm.Serialize());
            gcode.reset_stepper_timeout(); // Prevent disable axis
        }
        break;
    }
    #endif /*HAS_TOOLCHANGER()*/
    case State::CrashRecovery_XY_HOME: {
        if (queue.has_commands_queued() || planner.processing()) {
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
            server.print_state = State::Resuming_Begin;
            fsm_destroy(ClientFSM::CrashRecovery);
            break;
        }
        server.paused_ticks = ticks_ms(); // time when printing paused
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
        nozzle_timeout_loop();
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
        gcode.reset_stepper_timeout(); // Prevent disable axis
        break;
    }
    case State::CrashRecovery_Axis_NOK: {
        nozzle_timeout_loop();
        switch (marlin_server::get_response_from_phase(PhasesCrashRecovery::axis_NOK)) {
        case Response::Retry:
            measure_axes_and_home();
            break;
        case Response::Resume: /// ignore wrong length of axes
            server.print_state = State::Resuming_Begin;
            fsm_destroy(ClientFSM::CrashRecovery);
    #if ENABLED(AXIS_MEASURE)
            axes_length_set_ok(); /// ignore re-test of lengths
    #endif
            break;
        case Response::_none:
            break;
        default:
            server.print_state = State::Paused;
            fsm_destroy(ClientFSM::CrashRecovery);
        }
        gcode.reset_stepper_timeout(); // prevent disable axis
        break;
    }
    case State::CrashRecovery_Repeated_Crash: {
        nozzle_timeout_loop();
        switch (marlin_server::get_response_from_phase(PhasesCrashRecovery::repeated_crash)) {
        case Response::Resume:
            server.print_state = State::Resuming_Begin;
            fsm_destroy(ClientFSM::CrashRecovery);
            break;
        case Response::_none:
            break;
        default:
            server.print_state = State::Paused;
            fsm_destroy(ClientFSM::CrashRecovery);
        }
        gcode.reset_stepper_timeout(); // prevent disable axis
        break;
    }
#endif // ENABLED(CRASH_RECOVERY)
#if ENABLED(POWER_PANIC)
    case State::PowerPanic_acFault:
        power_panic::panic_loop();
        break;
    case State::PowerPanic_AwaitingResume:
    case State::PowerPanic_Resume:
        power_panic::resume_loop();
        break;
#endif // ENABLED(POWER_PANIC)
    default:
        break;
    }

    if (marlin_vars().fan_check_enabled) {
        HOTEND_LOOP() {
#if !PRINTER_IS_PRUSA_iX()
            const auto fan_state = Fans::heat_break(e).getState();
            hotendFanErrorChecker[e].checkTrue(fan_state != CFanCtlCommon::FanState::error_running && fan_state != CFanCtlCommon::FanState::error_starting, WarningType::HotendFanError, true);
#endif
        }
        const auto fan_state = Fans::print(active_extruder).getState();
        printFanErrorChecker.checkTrue(fan_state != CFanCtlCommon::FanState::error_running && fan_state != CFanCtlCommon::FanState::error_starting, WarningType::PrintFanError, false);
    }

    HOTEND_LOOP() {
        if (Fans::heat_break(e).getRPMIsOk()) {
            hotendFanErrorChecker[e].reset();
        }
    }
    if (Fans::print(active_extruder).getRPMIsOk()) {
        printFanErrorChecker.reset();
    }

#if HAS_TEMP_HEATBREAK
    HOTEND_LOOP() {
    #if ENABLED(PRUSA_TOOLCHANGER)
        if (!prusa_toolchanger.is_tool_enabled(e)) {
            continue;
        }
    #endif

        const auto temp = thermalManager.degHeatbreak(e);

        // Heatbreak is not yet initialized -> nothing to check
        if (temp == TempInfo::celsius_uninitialized) {
            continue;
        }
        // Heatbreak started reporting valid temperatures -> clear the warning
        else if (temp > 10) {
            heatBreakThermistorErrorChecker[e].reset();
        }
        // Getting 0 -> heatbreak error
        else {
            heatBreakThermistorErrorChecker[e].checkTrue(!NEAR_ZERO(temp), WarningType::HeatBreakThermistorFail, true);
        }
    }
#endif

    hotendErrorChecker.checkTrue(Temperature::saneTempReadingHotend(0));

    // Check MCU temperatures
    mcuMaxTempErrorChecker.check(AdcGet::getMCUTemp(), WarningType::BuddyMCUMaxTemp, "Buddy");
#if HAS_DWARF()
    HOTEND_LOOP() {
        if (prusa_toolchanger.is_tool_enabled(e)) {
            dwarfMaxTempErrorChecker[e].check(buddy::puppies::dwarfs[e].get_mcu_temperature(), WarningType::DwarfMCUMaxTemp, dwarf_names[e]);
        }
    }
#endif /*HAS_DWARF()*/
#if HAS_MODULARBED()
    modbedMaxTempErrorChecker.check(buddy::puppies::modular_bed.get_mcu_temperature(), WarningType::ModBedMCUMaxTemp, "Modular Bed");
#endif /*HAS_MODULARBED()*/
}

void resuming_begin(void) {
    // Reset errors, so it can be triggered immediately again
    HOTEND_LOOP() {
        hotendFanErrorChecker[e].reset();
    }
    printFanErrorChecker.reset();

    mcuMaxTempErrorChecker.reset();
#if HAS_DWARF()
    HOTEND_LOOP() {
        if (prusa_toolchanger.is_tool_enabled(e)) {
            dwarfMaxTempErrorChecker[e].reset();
        }
    }
#endif /*HAS_DWARF()*/
#if HAS_MODULARBED()
    modbedMaxTempErrorChecker.reset();
#endif /*HAS_MODULARBED()*/

    nozzle_timeout_on(); // could be turned off after pause by changing temperature.
    if (print_reheat_ready()) {

#if HAS_BED_PROBE || HAS_LOADCELL() && ENABLED(PROBE_CLEANUP_SUPPORT)
        if (server.mbl_failed) {
            // There's homing after MBL fail so no need to unpark at all
            server.print_state = State::Resuming_UnparkHead_ZE;
        } else
#endif /*HAS_BED_PROBE || HAS_LOADCELL() && ENABLED(PROBE_CLEANUP_SUPPORT)*/
#if ENABLED(CRASH_RECOVERY)
            if (crash_s.get_state() == Crash_s::REPEAT_WAIT) {
            // Skip unpark when recovering from toolcrash or homing fail
            server.print_state = State::Resuming_UnparkHead_ZE;
        } else
#endif /*ENABLED(CRASH_RECOVERY)*/
        {
            unpark_head_XY();
            server.print_state = State::Resuming_UnparkHead_XY;
        }
    } else {
        HOTEND_LOOP() {
            unpause_nozzle(e);
        }
#if FAN_COUNT > 0
        thermalManager.set_fan_speed(0, 0); // disable print fan
#endif
        server.print_state = State::Resuming_Reheating;
    }

    if (!server.print_is_serial) {
        media_prefetch_start();
    }
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
    queue.last_executed_sdpos = set;
}

void retract() {
    // server.motion_param.save_reset();  // TODO: currently disabled (see Crash_s::save_parameters())
#if ENABLED(ADVANCED_PAUSE_FEATURE)
    float mm = PAUSE_PARK_RETRACT_LENGTH / planner.e_factor[active_extruder];
    #if BOTH(CRASH_RECOVERY, LIN_ADVANCE)
    if (crash_s.did_trigger()) {
        mm += crash_s.advance_mm;
    }
    #endif
    plan_move_by(PAUSE_PARK_RETRACT_FEEDRATE, 0, 0, 0, -mm);
#endif // ENABLED(ADVANCED_PAUSE_FEATURE)
}

void lift_head() {
#if ENABLED(NOZZLE_PARK_FEATURE)
    const float distance = std::min<float>(
                               std::max<float>({
                                   Z_NOZZLE_PARK_POINT + current_position.z,
    #ifdef Z_NOZZLE_PARK_POINT_MIN
                                   Z_NOZZLE_PARK_POINT_MIN,
    #endif
                               }),
                               Z_MAX_POS)
        - current_position.z;
    static_assert(Z_NOZZLE_PARK_POINT > 0);

    if (TEST(axis_known_position, Z_AXIS)) {
        // Do prepare_move_to_destination, as it segments the move and thus allows better emergency_stop
        AutoRestore _ar(feedrate_mm_s, MMM_TO_MMS(HOMING_FEEDRATE_INVERTED_Z));
        destination = current_position;
        destination.z += distance;
        prepare_move_to_destination();
        planner.synchronize();

    } else {
        // If the Z is not homed, do a "homing" move with quickstops that will stop as soon as we hit the limits
        TemporaryGlobalEndstopsState _es(true);

        // do_homing_move does not update current position, we have to do it manually
        // have to use HOMING_FEEDRATE, otherwise the stallguards might not trigger
        if (do_homing_move(Z_AXIS, distance, MMM_TO_MMS(HOMING_FEEDRATE_INVERTED_Z))) {
            current_position.z = Z_MAX_POS;
        } else {
            current_position.z += distance;
        }
        sync_plan_position();
    }
#endif // ENABLED(NOZZLE_PARK_FEATURE)
}

void park_head() {
#if ENABLED(NOZZLE_PARK_FEATURE)
    if (!all_axes_homed()) {
        return;
    }

    server.resume.pos = current_position;
    retract();
    lift_head();

    #if HAS_TOOLCHANGER()
    // Check that we are not in dock
    // Can happen if stopped during toolchanging, toolchange will finish but last move doesn't wait for planner.synchronize();
    if (current_position.y > PrusaToolChanger::SAFE_Y_WITH_TOOL) {
        current_position.y = PrusaToolChanger::SAFE_Y_WITH_TOOL;
        line_to_current_position(NOZZLE_PARK_XY_FEEDRATE); // Move to safe Y
        planner.synchronize();
    }
    #endif /*HAS_TOOLCHANGER()*/

    xyz_pos_t park = XYZ_NOZZLE_PARK_POINT;
    #ifdef XYZ_NOZZLE_PARK_POINT_M600
    const xyz_pos_t park_clean = XYZ_NOZZLE_PARK_POINT_M600;
    if (server.mbl_failed) {
        park = park_clean;
    }
    #endif // XYZ_NOZZLE_PARK_POINT_M600
    park.z = current_position.z;
    plan_park_move_to_xyz(park, NOZZLE_PARK_XY_FEEDRATE, NOZZLE_PARK_Z_FEEDRATE);
#endif // NOZZLE_PARK_FEATURE
}

void unpark_head_XY(void) {
#if ENABLED(NOZZLE_PARK_FEATURE)
    // TODO: double check this condition: when recovering from a crash, Z is not known, but we *can*
    // unpark, so we bypass this check as we need to move back
    if (TERN1(CRASH_RECOVERY, !crash_s.did_trigger()) && !all_axes_homed()) {
        return;
    }

    current_position.x = server.resume.pos.x;
    current_position.y = server.resume.pos.y;
    NOMORE(current_position.y, Y_BED_SIZE); // Prevent crashing into parked tools
    line_to_current_position(NOZZLE_PARK_XY_FEEDRATE);
#endif // NOZZLE_PARK_FEATURE
}

void unpark_head_ZE(void) {
#if ENABLED(NOZZLE_PARK_FEATURE)
    // TODO: see comment above on unparking: if axes are not known, lift is skipped, but not this
    if (!all_axes_homed()) {
        return;
    }

    // Move Z
    current_position.z = server.resume.pos.z;
    destination = current_position;
    prepare_internal_move_to_destination(NOZZLE_PARK_Z_FEEDRATE);

    #if ENABLED(ADVANCED_PAUSE_FEATURE)
    // Undo E retract
    plan_move_by(PAUSE_PARK_RETRACT_FEEDRATE, 0, 0, 0, server.resume.pos.e - current_position.e);
    #endif // ENABLED(ADVANCED_PAUSE_FEATURE)
#endif // NOZZLE_PARK_FEATURE
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

void set_target_bed(float value) {
    marlin_vars().target_bed = value;
    thermalManager.setTargetBed(value);
}

void set_temp_to_display(float value, uint8_t extruder) {
    marlin_vars().hotend(extruder).display_nozzle = value;
}

bool get_media_inserted(void) {
    return marlin_vars().media_inserted;
}

resume_state_t *get_resume_data() {
    return &server.resume;
}

void set_resume_data(const resume_state_t *data) {
    // ensure this is called only from the marlin thread
    assert(osThreadGetId() == server_task);
    server.resume = *data;
}

extern uint32_t get_user_click_count(void) {
    return server.knob_click_counter;
}

extern uint32_t get_user_move_count(void) {
    return server.knob_move_counter;
}

//-----------------------------------------------------------------------------
// private functions

static bool _send_message_event_to_client(int client_id, ClientQueue &queue) {
    char *message = std::exchange(server.event_messages[client_id], nullptr);

    const marlin_client::ClientEvent client_event {
        .event = Event::Message,
        .unused = 0,
        .usr16 = 0,
        .message = message,
    };
    if (queue.try_send(client_event, 0)) {
        // message was sent, client will free it
        return true;
    } else {
        // message was not sent, we have to free it
        free(message);
        return false;
    }
}

// send event notification to client (called from server thread)
static bool _send_notify_event_to_client(int client_id, ClientQueue &queue, Event evt_id, uint32_t usr32, uint16_t usr16) {
    switch (evt_id) {
    case Event::Message:
        return _send_message_event_to_client(client_id, queue);
    default: {
        const marlin_client::ClientEvent client_message {
            .event = evt_id,
            .unused = 0,
            .usr16 = usr16,
            .usr32 = usr32,
        };
        return queue.try_send(client_message, 0);
    }
    }
}

// send event notification to client - multiple events (called from server thread)
// returns mask of successfully sent events
static uint64_t _send_notify_events_to_client(int client_id, ClientQueue &queue, uint64_t evt_msk) {
    if (evt_msk == 0) {
        return 0;
    }
    uint64_t sent = 0;
    uint64_t msk = 1;
    for (uint8_t evt_int = 0; evt_int <= ftrstd::to_underlying(Event::_last); evt_int++) {
        Event evt_id = Event(evt_int);
        if (msk & evt_msk) {
            switch (Event(evt_id)) {
                // Events without arguments
                // TODO: send all these in a single message as a bitfield
            case Event::MediaInserted:
            case Event::MediaError:
            case Event::MediaRemoved:
            case Event::RequestCalibrationsScreen:
            // StatusChanged event - one string argument
            case Event::StatusChanged:
                if (_send_notify_event_to_client(client_id, queue, evt_id, 0, 0)) {
                    sent |= msk; // event sent, set bit
                }
                break;
            // CommandBegin/End - one ui32 argument (CMD)
            case Event::CommandBegin:
                if (_send_notify_event_to_client(client_id, queue, evt_id, server.command_begin, 0)) {
                    sent |= msk; // event sent, set bit
                }
                break;
            case Event::CommandEnd:
                if (_send_notify_event_to_client(client_id, queue, evt_id, server.command_end, 0)) {
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
            case Event::Message:
                sent |= msk; // fake event sent for unused and forced events
                break;
            case Event::_count:
                assert(false);
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
        if (server.notify_events[client_id] & ((uint64_t)1 << ftrstd::to_underlying(evt_id))) {
            if (_send_notify_event_to_client(client_id, marlin_client::marlin_client_queue[client_id], evt_id, usr32, usr16) == 0) {
                server.client_events[client_id] |= ((uint64_t)1 << ftrstd::to_underlying(evt_id)); // event not sent, set bit
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
    marlin_vars().pqueue = planner.movesplanned();

    // Get native position
    xyze_pos_t pos_mm, curr_pos_mm;
    planner.get_axis_position_mm(pos_mm);
    curr_pos_mm = current_position;
    LOOP_XYZE(i) {
        marlin_vars().native_pos[i] = pos_mm[i];
        marlin_vars().native_curr_pos[i] = curr_pos_mm[i];
    }
    // Convert to logical position
    planner.unapply_leveling(pos_mm);
    toLogical(pos_mm);
    toLogical(curr_pos_mm);
    LOOP_XYZE(i) {
        marlin_vars().logical_pos[i] = pos_mm[i];
        marlin_vars().logical_curr_pos[i] = curr_pos_mm[i];
    }

    HOTEND_LOOP() {
        auto &extruder = marlin_vars().hotend(e);

        extruder.temp_nozzle = thermalManager.degHotend(e);
        extruder.target_nozzle = thermalManager.degTargetHotend(e);
        extruder.pwm_nozzle = thermalManager.getHeaterPower(static_cast<heater_ind_t>(H_E0 + e));

#if (TEMP_SENSOR_HEATBREAK > 0)
        // TODO: this should track multiple extruders
        extruder.temp_heatbreak = thermalManager.temp_heatbreak[e].celsius;
        extruder.target_heatbreak = thermalManager.temp_heatbreak[e].target;
#endif
        extruder.flow_factor = static_cast<uint16_t>(planner.flow_percentage[e]);
        extruder.print_fan_rpm = Fans::print(e).getActualRPM();
        extruder.heatbreak_fan_rpm = Fans::heat_break(e).getActualRPM();
    }

#if ENABLED(CANCEL_OBJECTS)
    marlin_vars().set_cancel_object_mask(cancelable.canceled); // Canceled objects
    marlin_vars().cancel_object_count = cancelable.object_count; // Total number of objects
#endif /*ENABLED(CANCEL_OBJECTS)*/

    marlin_vars().temp_bed = thermalManager.degBed();
    marlin_vars().target_bed = thermalManager.degTargetBed();
#if ENABLED(MODULAR_HEATBED)
    marlin_vars().enabled_bedlet_mask = thermalManager.getEnabledBedletMask();
#endif

    marlin_vars().z_offset = probe_offset.z;
#if FAN_COUNT > 0
    marlin_vars().print_fan_speed = thermalManager.fan_speed[0];
#endif
    marlin_vars().print_speed = static_cast<uint16_t>(feedrate_percentage);

    auto progress_data = oProgressData.mode_specific(marlin_vars().stealth_mode);

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
            return std::min<uint8_t>(std::round(100.0f * queue.last_executed_sdpos / prefetch_metrics.stream_size_estimate), 99);
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

    marlin_vars().active_extruder = active_extruder;

    marlin_vars().extrude_min_temp = thermalManager.extrude_min_temp;
    marlin_vars().allow_cold_extrude = thermalManager.allow_cold_extrude;

    // print state is updated last, to make sure other related variables (like job_id, filenames) are already set when we start print
    marlin_vars().print_state = static_cast<State>(server.print_state);

    marlin_vars().media_position = media_position();

    marlin_vars().media_size_estimate = prefetch_metrics.stream_size_estimate;
}

bool _process_server_valid_request(const Request &request, int client_id) {
    switch (request.type) {
    case Request::Type::Gcode:
        //@TODO return value depending on success of enqueueing gcode
        return enqueue_gcode(request.gcode);
    case Request::Type::Inject:
        inject(request.inject);
        return true;
    case Request::Type::SetVariable:
        _server_set_var(request);
        return true;
    case Request::Type::Babystep:
        do_babystep_Z(request.babystep);
        return true;
#if ENABLED(CANCEL_OBJECTS)
    case Request::Type::CancelObjectID:
        cancelable.cancel_object(request.cancel_object_id);
        return true;
    case Request::Type::UncancelObjectID:
        cancelable.uncancel_object(request.uncancel_object_id);
        return true;
    case Request::Type::CancelCurrentObject:
        cancelable.cancel_active_object();
        return true;
#else
    case Request::Type::CancelObjectID:
    case Request::Type::UncancelObjectID:
    case Request::Type::CancelCurrentObject:
        return false;
#endif
    case Request::Type::PrintStart:
        print_start(request.print_start.filename, GCodeReaderPosition(), request.print_start.skip_preview);
        return true;
    case Request::Type::PrintReady:
        gui_ready_to_print();
        return true;
    case Request::Type::GuiCantPrint:
        gui_cant_print();
        return true;
    case Request::Type::PrintAbort:
        print_abort();
        return true;
    case Request::Type::PrintPause:
        print_pause();
        return true;
    case Request::Type::PrintResume:
        print_resume();
        return true;
    case Request::Type::TryRecoverFromMediaError:
        try_recover_from_media_error();
        return true;
    case Request::Type::PrintExit:
        print_exit();
        return true;
    case Request::Type::KnobMove:
        ++server.knob_move_counter;
        return true;
    case Request::Type::KnobClick:
        ++server.knob_click_counter;
        return true;
    case Request::Type::SetWarning:
        set_warning(request.warning_type);
        return true;
    case Request::Type::FSM:
        server_side_encoded_fsm_response = request.encoded_fsm_response;
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
    case Request::Type::TestStart:
        marlin_server::test_start(
            request.test_start.test_mask,
            selftest::deserialize_test_data_from_int(request.test_start.test_data_index, request.test_start.test_data_data));
        return true;
    case Request::Type::TestAbort:
        test_abort();
        return true;
    }
    bsod("Unknown request %d", ftrstd::to_underlying(request.type));
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
        marlin_vars().target_bed = request.set_variable.float_value;
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
#if FAN_COUNT > 0
        thermalManager.set_fan_speed(0, marlin_vars().print_fan_speed);
#endif
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
    if (variable_identifier == reinterpret_cast<uintptr_t>(&marlin_vars().fs_autoload_enabled)) {
        marlin_vars().fs_autoload_enabled = request.set_variable.uint32_value;
        return;
    }

    // Now see if extruder variable is set
    HOTEND_LOOP() {
        auto &extruder = marlin_vars().hotend(e);
        if (reinterpret_cast<uintptr_t>(&extruder.target_nozzle) == variable_identifier) {
            extruder.target_nozzle = request.set_variable.float_value;

            // if print is paused we want to change the resume temp and turn off timeout
            // this prevents going back to temperature before pause and enables to heat nozzle during pause
            if (server.print_state == State::Paused) {
                nozzle_timeout_off();
                server.resume.nozzle_temp[e] = extruder.target_nozzle;
            }
            thermalManager.setTargetHotend(extruder.target_nozzle, e);
            return;
        } else if (reinterpret_cast<uintptr_t>(&extruder.flow_factor) == variable_identifier) {
            extruder.flow_factor = request.set_variable.uint32_value;
            planner.flow_percentage[e] = (int16_t)extruder.flow_factor;
            planner.refresh_e_factor(e);
            return;
        } else if (reinterpret_cast<uintptr_t>(&extruder.display_nozzle) == variable_identifier) {
            extruder.display_nozzle = request.set_variable.float_value;
            return;
        }
    }

    // if we got here, no variable was set, return error
    bsod("unimplemented _server_set_var for var_id %i", (int)variable_identifier);
}

void set_warning(WarningType type) {
    log_warning(MarlinServer, "Warning type %d set", (int)type);
    log_info(MarlinServer, "WARNING: %" PRIu32, ftrstd::to_underlying(type));

    // We are just creating it here, it is then handled in handle_warning in cycle function
    fsm::PhaseData data;
    memcpy(data.data(), &type, sizeof(data));
    // We don't want to overlay two warnings and the new one is likely more important.
    clear_warnings();
    fsm_create(warning_type_phase(type), data);
}

void clear_warning(WarningType type) {
    if (is_warning_active(type)) {
        fsm_destroy(ClientFSM::Warning);
    }
}

bool is_warning_active(WarningType type) {
    return fsm_states.is_active(ClientFSM::Warning) && type == std::bit_cast<WarningType>(fsm_states[ClientFSM::Warning]->GetData());
}

/*****************************************************************************/
// FSM_notifier
FSM_notifier::data FSM_notifier::s_data;
FSM_notifier *FSM_notifier::activeInstance = nullptr;

FSM_notifier::FSM_notifier(ClientFSM type, uint8_t phase, float min, float max,
    uint8_t progress_min, uint8_t progress_max, const MarlinVariable<float> &var_id)
    : temp_data(s_data) {
    s_data.type = type;
    s_data.phase = phase;
    s_data.scale = static_cast<float>(progress_max - progress_min) / (max - min);
    s_data.offset = -min * s_data.scale + static_cast<float>(progress_min);
    s_data.progress_min = progress_min;
    s_data.progress_max = progress_max;
    s_data.var_id = &var_id;
    s_data.last_progress_sent = std::nullopt;
    activeInstance = this;
}

// static method
// notifies clients about progress rise
// scales "bound" variable via following formula to calculate progress
// x = (actual - s_data.min) * s_data.scale + s_data.progress_min;
// x = actual * s_data.scale - s_data.min * s_data.scale + s_data.progress_min;
// s_data.offset == -s_data.min * s_data.scale + s_data.progress_min
// simplified formula
// x = actual * s_data.scale + s_data.offset;
void FSM_notifier::SendNotification() {
    if (!activeInstance) {
        return;
    }
    if (s_data.type == ClientFSM::_none) {
        return;
    }

    float actual = *s_data.var_id;
    actual = actual * s_data.scale + s_data.offset;

    int progress = static_cast<int>(actual); // int - must be signed
    if (progress < s_data.progress_min) {
        progress = s_data.progress_min;
    }
    if (progress > s_data.progress_max) {
        progress = s_data.progress_max;
    }

    // after first sent, progress can only rise
    // no value: comparison returns true
    if (progress > s_data.last_progress_sent) {
        s_data.last_progress_sent = progress;
        fsm_change(FSMAndPhase(s_data.type, s_data.phase), activeInstance->serialize(progress));
    }
}

FSM_notifier::~FSM_notifier() {
    s_data = temp_data;
    activeInstance = nullptr;
}

FSMResponseVariant get_response_variant_from_phase(FSMAndPhase fsm_and_phase) {
    // FIXME: Critical section is used to mimic original behaviour with std::atomic
    //        However, maybe we should instead require that the calling task
    //        is actually Marlin task. This is most probably the case,
    //        but checking that is beyond the scope of this patch.
    freertos::CriticalSection critical_section;
    const EncodedFSMResponse value = server_side_encoded_fsm_response;
    if (value.fsm_and_phase == fsm_and_phase) {
        server_side_encoded_fsm_response = empty_encoded_fsm_response;
        return value.response;
    } else {
        return {};
    }
}

Response wait_for_response(FSMAndPhase fsm_and_phase) {
    while (true) {
        if (auto r = get_response_from_phase(fsm_and_phase); r != Response::_none) {
            return r;
        }

        ::idle(true);
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

namespace ExtUI {

using namespace marlin_server;

void onStartup() {
}

void onIdle() {
    idle();
    buddy::metrics::RecordMarlinVariables();
    buddy::metrics::RecordRuntimeStats();
    buddy::metrics::RecordPrintFilename();
#if (BOARD_IS_XLBUDDY())
    buddy::metrics::record_dwarf_internal_temperatures();
#endif
    print_utils_loop();
}

void onPrinterKilled(PGM_P const msg, PGM_P const component) {
    log_info(MarlinServer, "Printer killed: %s", msg);
    vTaskEndScheduler();
    wdt_iwdg_refresh(); // watchdog reset
    fatal_error(msg, component);
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

void onUserConfirmRequired(const char *const msg) {
    log_info(MarlinServer, "ExtUI: onUserConfirmRequired: %s", msg);
}

#if HAS_BED_PROBE || HAS_LOADCELL() && ENABLED(PROBE_CLEANUP_SUPPORT)
static void mbl_error(WarningType warning) {
    if (server.print_state != State::Printing && server.print_state != State::Pausing_Begin) {
        return;
    }

    server.print_state = State::Pausing_Failed_Code;
    /// pause immediatelly to save current file position
    pause_print(Pause_Type::Repeat_Last_Code);
    server.mbl_failed = true;
    set_warning(warning);
}
#endif

void onStatusChanged(const char *const msg) {
    if (!msg) {
        return; // ignore errorneous nullptr messages
    }

    static bool pending_err_msg = false;

    log_info(MarlinServer, "ExtUI: onStatusChanged: %s", msg);
    _send_notify_event(Event::StatusChanged, 0, 0); // this includes MMU:P progress messages - just plain textual information
    if (msg != nullptr && strcmp(msg, "Prusa-mini Ready.") == 0) {
    } // TODO
    else if (strcmp(msg, "TMC CONNECTION ERROR") == 0) {
        // FIXME: Nobody was consuming this at all, so disabled.
        //_send_notify_event(Event::Error, MARLIN_ERR_TMCDriverError, 0);
    } else {
        if (!is_abort_state(server.print_state)) {
            pending_err_msg = false;
        }
        if (!pending_err_msg) {
/// FIXME: Message through Marlin's UI could be delayed and we won't pause print at the MBL command
#if HAS_BED_PROBE
            if (strcmp(msg, MSG_ERR_PROBING_FAILED) == 0) {
                mbl_error(WarningType::ProbingFailed);
                pending_err_msg = true;
            }
#endif
#if HAS_LOADCELL() && ENABLED(PROBE_CLEANUP_SUPPORT)
            if (strcmp(msg, MSG_ERR_NOZZLE_CLEANING_FAILED) == 0) {
                mbl_error(WarningType::NozzleCleaningFailed);
                pending_err_msg = true;
            }
#endif
            if (msg[0] != 0) { // empty message filter
                _add_status_msg(msg);
                _send_notify_event(Event::Message, 0, 0);
            }
        }
    }
}

void onFactoryReset() {
    log_info(MarlinServer, "ExtUI: onFactoryReset");
}

void onLoadSettings(char const *) {
    log_info(MarlinServer, "ExtUI: onLoadSettings");
}

void onStoreSettings(char *) {
    log_info(MarlinServer, "ExtUI: onStoreSettings");
}

void onConfigurationStoreWritten([[maybe_unused]] bool success) {
    log_info(MarlinServer, "ExtUI: onConfigurationStoreWritten");
}

void onConfigurationStoreRead([[maybe_unused]] bool success) {
    log_info(MarlinServer, "ExtUI: onConfigurationStoreRead");
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
