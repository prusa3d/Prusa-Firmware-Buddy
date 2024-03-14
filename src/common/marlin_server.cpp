// marlin_server.cpp

#include "marlin_server.hpp"
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
#include "app.h"
#include "bsod.h"
#include "module/prusa/tool_mapper.hpp"
#include "module/prusa/spool_join.hpp"
#include "timing.h"
#include "cmsis_os.h"
#include "log.h"
#include <bsod_gui.hpp>

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
#include "utility_extensions.hpp"

#if ENABLED(PRUSA_MMU2)
    #include "../Marlin/src/feature/prusa/MMU2/mmu2_mk4.h"
#endif

#if ENABLED(CANCEL_OBJECTS)
    #include "../Marlin/src/feature/cancel_object.h"
#endif

#include "hwio.h"
#include "media.h"
#include "wdt.h"
#include "../marlin_stubs/G26.hpp"
#include "../marlin_stubs/M123.hpp"
#include "fsm_types.hpp"
#include "odometer.hpp"
#include "metric.h"

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

#if HAS_DWARF()
    #include <puppies/Dwarf.hpp>
#endif /*HAS_DWARF()*/

#if HAS_MODULARBED()
    #include <puppies/modular_bed.hpp>
#endif /*HAS_MODULARBED()*/

#if HAS_SELFTEST()
    #include "printer_selftest.hpp"
#endif

#include "SteelSheets.hpp"

#ifdef MINDA_BROKEN_CABLE_DETECTION
    #include "Z_probe.hpp" //get_Z_probe_endstop_hits
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
    #include "mmu2_fsm.hpp"
#endif

#include <variant8.h>
#include <config_store/store_instance.hpp>
using namespace ExtUI;

LOG_COMPONENT_DEF(MarlinServer, LOG_SEVERITY_INFO);

//-----------------------------------------------------------------------------
// external variables from marlin_client

namespace marlin_client {
extern osThreadId marlin_client_task[MARLIN_MAX_CLIENTS]; // task handles
extern osMessageQId marlin_client_queue[MARLIN_MAX_CLIENTS]; // input queue handles (uint32_t)
} // namespace marlin_client

namespace marlin_server {

namespace {

    struct server_t {
        EventMask notify_events[MARLIN_MAX_CLIENTS]; // event notification mask - message filter
        EventMask notify_changes[MARLIN_MAX_CLIENTS]; // variable change notification mask - message filter
        EventMask client_events[MARLIN_MAX_CLIENTS]; // client event mask - unsent messages
        variant8_t event_messages[MARLIN_MAX_CLIENTS]; // last Event::Message for clients, cannot use cvariant, destructor would free memory
        State print_state; // printing state (printing, paused, ...)
        bool print_is_serial = false; //< When true, current print is not from USB, but sent via gcode commands.
#if ENABLED(CRASH_RECOVERY) //
        bool aborting_did_crash_trigger = false; // To remember crash_s state when aborting
#endif /*ENABLED(CRASH_RECOVERY)*/
        uint32_t paused_ticks; // tick count in moment when printing paused
        resume_state_t resume; // resume data (state before pausing)
        bool enable_nozzle_temp_timeout; // enables nozzle temperature timeout in print pause
        struct {
            uint32_t usr32;
            uint16_t usr16;
        } last_mesh_evt;
        uint32_t warning_type;
        int request_len;
        uint32_t last_update; // last update tick count
        uint32_t command; // actually running command
        uint32_t command_begin; // variable for notification
        uint32_t command_end; // variable for notification
        uint32_t knob_click_counter;
        uint32_t knob_move_counter;
        uint16_t flags; // server flags (MARLIN_SFLG)
        char request[MARLIN_MAX_REQUEST];
        uint8_t idle_cnt; // idle call counter
        uint8_t pqueue_head; // copy of planner.block_buffer_head
        uint8_t pqueue_tail; // copy of planner.block_buffer_tail
        uint8_t pqueue; // calculated number of records in planner queue
        uint8_t gqueue; // copy of queue.length - number of commands in gcode queue
                        // Motion_Parameters motion_param;

#if ENABLED(AXIS_MEASURE)
        /// length of axes measured after crash
        /// negative numbers represent undefined length
        xy_float_t axis_length = { -1, -1 };
        Measure_axis *measure_axis = nullptr;
#endif // ENABLED(AXIS_MEASURE)

#if HAS_BED_PROBE || HAS_LOADCELL() && ENABLED(PROBE_CLEANUP_SUPPORT)
        bool mbl_failed;
#endif
    };

    server_t server; // server structure - initialize task to zero

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
    void pause_print(Pause_Type type = Pause_Type::Pause, uint32_t resume_pos = UINT32_MAX);

    fsm::QueueWrapper<MARLIN_MAX_CLIENTS> fsm_event_queues;

    template <WarningType p_warning, bool p_disableHotend>
    class ErrorChecker {
    public:
        ErrorChecker()
            : m_failed(false) {};

        void checkTrue(bool condition) {
            if (!condition && !m_failed) {
                set_warning(p_warning);
                if (server.print_state == State::Printing) {
                    pause_print(); // Must store current hotend temperatures before they are set to 0
                    server.print_state = State::Pausing_WaitIdle;
                }
                if (p_disableHotend) {
                    HOTEND_LOOP() {
                        thermalManager.setTargetHotend(0, e);
                    }
                }
                m_failed = true;
            }
        };
        void reset() { m_failed = false; }

    protected:
        bool m_failed;
    };

    class HotendErrorChecker : ErrorChecker<WarningType::HotendTempDiscrepancy, true> {
    public:
        HotendErrorChecker()
            : m_postponeFullPrintFan(false) {};

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
            ErrorChecker::checkTrue(condition);
            if (condition) {
                reset();
            }
        }
        bool runFullFan() {
            const bool retVal = m_postponeFullPrintFan;
            m_postponeFullPrintFan = false;
            return retVal;
        }
        bool isFailed() { return m_failed; }

    private:
        bool m_postponeFullPrintFan;
    };

    /// Check MCU temperature and trigger warning and redscreen
    template <WarningType p_warning>
    class MCUTempErrorChecker : public ErrorChecker<p_warning, true> {
        static constexpr const int32_t mcu_temp_warning = 80; ///< When to show warning and pause the print
        static constexpr const int32_t mcu_temp_hysteresis = 5; ///< Hysteresis to reset warning
        static constexpr const int32_t mcu_temp_redscreen = 100; ///< When to show redscreen error

        const char *name; ///< Name of board with the MCU

        int32_t ewma_buffer = 0; ///< Buffer for EWMA [1/8 degrees Celsius]
        bool warning = false; ///< True during warning state, enables hysteresis

    public:
        MCUTempErrorChecker(const char *name)
            : name(name) {};

        /**
         * @brief Check one MCU temperature.
         * @param temperature MCU temperature [degrees Celsius]
         */
        void check(int32_t temperature) {
            (void)temperature;
            return;
        }
    };

    ErrorChecker<WarningType::HotendFanError, true> hotendFanErrorChecker[HOTENDS];
    ErrorChecker<WarningType::PrintFanError, false> printFanErrorChecker;

#ifdef HAS_TEMP_HEATBREAK
    ErrorChecker<WarningType::HeatBreakThermistorFail, true> heatBreakThermistorErrorChecker[HOTENDS];
#endif
    HotendErrorChecker hotendErrorChecker;

    MCUTempErrorChecker<WarningType::BuddyMCUMaxTemp> mcuMaxTempErrorChecker("Buddy"); ///< Check Buddy MCU temperature
#if HAS_DWARF()
    /// Check Dwarf MCU temperature
    MCUTempErrorChecker<WarningType::DwarfMCUMaxTemp> dwarfMaxTempErrorChecker[HOTENDS] {
        "Dwarf 1", "Dwarf 2", "Dwarf 3", "Dwarf 4", "Dwarf 5", "Dwarf 6"
    };
#endif /*HAS_DWARF()*/
#if HAS_MODULARBED()
    MCUTempErrorChecker<WarningType::ModBedMCUMaxTemp> modbedMaxTempErrorChecker("Modular Bed"); ///< Check ModularBed MCU temperature
#endif /*HAS_MODULARBED()*/

    void pause_print(Pause_Type type, uint32_t resume_pos) {
        if (!server.print_is_serial) {
            switch (type) {
            case Pause_Type::Crash:
                media_print_quick_stop(resume_pos);
                break;
            case Pause_Type::Repeat_Last_Code:
                media_print_pause(true);
                break;
            default:
                media_print_pause(false);
            }
        }

        SerialPrinting::pause();

        print_job_timer.pause();
        HOTEND_LOOP() {
            server.resume.nozzle_temp[e] = marlin_vars()->hotend(e).target_nozzle; // save nozzle target temp
        }
        server.resume.nozzle_temp_paused = true;
        server.resume.fan_speed = marlin_vars()->print_fan_speed; // save fan speed
        server.resume.print_speed = marlin_vars()->print_speed;
#if FAN_COUNT > 0
        if (hotendErrorChecker.runFullFan()) {
            thermalManager.set_fan_speed(0, 255);
        } else {
            thermalManager.set_fan_speed(0, 0); // disable print fan
        }
#endif
    }
} // end anonymous namespace

//-----------------------------------------------------------------------------
// variables

osThreadId server_task = 0; // task handle
osMessageQId server_queue = 0; // input queue (uint8_t)
osSemaphoreId server_semaphore = 0; // semaphore handle

#ifdef DEBUG_FSENSOR_IN_HEADER
uint32_t *pCommand = &server.command;
#endif
idle_t *idle_cb = 0; // idle callback

void _add_status_msg(const char *const popup_msg) {
    // I could check client mask here
    for (size_t i = 0; i < MARLIN_MAX_CLIENTS; ++i) {
        variant8_t *pvar = &(server.event_messages[i]);
        variant8_set_type(pvar, VARIANT8_PCHAR);
        variant8_done(&pvar); // destroy unsent message - free dynamic memory
        server.event_messages[i] = variant8_pchar((char *)popup_msg, 0, 1); // variant malloc - detached on send
        variant8_set_type(&(server.event_messages[i]), VARIANT8_USER); // set user type so client can recognize it as event
        variant8_set_usr8(&(server.event_messages[i]), ftrstd::to_underlying(Event::Message));
    }
}

//-----------------------------------------------------------------------------
// forward declarations of private functions

static void _server_print_loop(void);
static int _send_notify_to_client(osMessageQId queue, variant8_t msg);
static bool _send_notify_event_to_client(int client_id, osMessageQId queue, Event evt_id, uint32_t usr32, uint16_t usr16);
static uint64_t _send_notify_events_to_client(int client_id, osMessageQId queue, uint64_t evt_msk);
static uint8_t _send_notify_event(Event evt_id, uint32_t usr32, uint16_t usr16);
static void _server_update_gqueue(void);
static void _server_update_pqueue(void);
static void _server_update_vars();
static bool _process_server_request(const char *request);
static void _server_set_var(const char *const name_val_str);

//-----------------------------------------------------------------------------
// server side functions

void init(void) {
    int i;
    server = server_t();
    osMessageQDef(serverQueue, MARLIN_SERVER_QUEUE, uint8_t);
    server_queue = osMessageCreate(osMessageQ(serverQueue), NULL);
    osSemaphoreDef(serverSema);
    server_semaphore = osSemaphoreCreate(osSemaphore(serverSema), 1);
    server.flags = MARLIN_SFLG_STARTED;
    for (i = 0; i < MARLIN_MAX_CLIENTS; i++) {
        server.notify_events[i] = make_mask(Event::Acknowledge) | make_mask(Event::Startup) | make_mask(Event::StartProcessing); // by default only ack, startup and processing
        server.notify_changes[i] = 0; // by default nothing
    }
    server_task = osThreadGetId();
    server.enable_nozzle_temp_timeout = true;
#if HAS_BED_PROBE || HAS_LOADCELL() && ENABLED(PROBE_CLEANUP_SUPPORT)
    server.mbl_failed = false;
#endif
    marlin_vars()->init();
}

void print_fan_spd() {
    static uint32_t last_fan_report = 0;
    uint32_t current_time = ticks_s();
    if (M123::fan_auto_report_delay && (current_time - last_fan_report) >= M123::fan_auto_report_delay) {
        M123::print_fan_speed();
        last_fan_report = current_time;
    }
}

#ifdef MINDA_BROKEN_CABLE_DETECTION
static void print_Z_probe_cnt() {
    if (DEBUGGING(INFO)) {
        static uint32_t last = 0;
        static uint32_t actual = 0;
        actual = get_Z_probe_endstop_hits();
        if (last != actual) {
            last = actual;
            serial_echopair_PGM("Z Endstop hit ", actual);
            serialprintPGM(" times.");
            SERIAL_EOL();
        }
    }
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
    osMessageQId queue;
    uint64_t msk = 0;
    for (int client_id = 0; client_id < MARLIN_MAX_CLIENTS; client_id++) {
        if ((queue = marlin_client::marlin_client_queue[client_id]) != 0) {
            if ((msk = server.client_events[client_id]) != 0) {
                server.client_events[client_id] &= ~_send_notify_events_to_client(client_id, queue, msk);
            }
        }
    }
}

int cycle(void) {

    static int processing = 0;
    if (processing) {
        return 0;
    }
    processing = 1;
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

    if (call_print_loop) {
        _server_print_loop(); // we need call print loop here because it must be processed while blocking commands (M109)
    }

    FSM_notifier::SendNotification();

    print_fan_spd();

#ifdef MINDA_BROKEN_CABLE_DETECTION
    print_Z_probe_cnt();
#endif

#if HAS_TOOLCHANGER()
    // Check if tool didn't fall off
    prusa_toolchanger.loop(!printer_idle(), printer_paused());
#endif /*HAS_TOOLCHANGER()*/

    int count = 0;
    if (server.flags & MARLIN_SFLG_PENDREQ) {
        if (_process_server_request(server.request)) {
            server.request_len = 0;
            count++;
            server.flags &= ~MARLIN_SFLG_PENDREQ;
        }
    }

    osEvent ose;
    if ((server.flags & MARLIN_SFLG_PENDREQ) == 0) {
        while ((ose = osMessageGet(server_queue, 0)).status == osEventMessage) {
            char ch = (char)((uint8_t)(ose.value.v));
            switch (ch) {
            case '\r':
            case '\n':
                ch = 0;
                break;
            }
            if (server.request_len < MARLIN_MAX_REQUEST) {
                server.request[server.request_len++] = ch;
            } else {
                // TODO: request too long
                server.request_len = 0;
            }
            if ((ch == 0) && (server.request_len > 1)) {
                if (_process_server_request(server.request)) {
                    server.request_len = 0;
                    count++;
                } else {
                    server.flags |= MARLIN_SFLG_PENDREQ;
                    break;
                }
            }
        }
    }
    // update gqueue (gcode queue)
    _server_update_gqueue();
    // update pqueue (planner queue)
    _server_update_pqueue();
    // update variables
    send_notifications_to_clients();
    server_update_vars();

    if ((server.flags & MARLIN_SFLG_PROCESS) == 0) {
        wdt_iwdg_refresh(); // this prevents iwdg reset while processing disabled
    }
    processing = 0;
    return count;
}

void static finalize_print() {
#if ENABLED(POWER_PANIC)
    power_panic::reset();
#endif
    Odometer_s::instance().add_time(marlin_vars()->print_duration);
    print_area.reset_bounding_rect();
#if ENABLED(PRUSA_TOOL_MAPPING)
    tool_mapper.reset();
    spool_join.reset();
#endif
#if ENABLED(GCODE_COMPATIBILITY_MK3)
    gcode.compatibility_mode = GcodeSuite::CompatibilityMode::NONE;
#endif
    // Reset IS at the end of the print
    input_shaper::init();

    server.print_is_serial = false; // reset flag about serial print

    marlin_vars()->print_end_time = time(nullptr);
}

static const uint8_t MARLIN_IDLE_CNT_BUSY = 1;

#if ANY(CRASH_RECOVERY, POWER_PANIC)
static void check_crash() {
    // reset the nested loop check once per main server iteration
    crash_s.loop = false;

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
        crash_s.loop = false; // Set again to prevent race when ISR happens during this function
        server.print_state = State::CrashRecovery_Begin;
        return;
    }
}
#endif // ENABLED(CRASH_RECOVERY)

int loop(void) {
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
    media_loop();
    return cycle();
}

int idle(void) {
    // TODO: avoid a re-entrant cycle caused by:
    // cycle -> loop -> idle -> MarlinUI::update() -> ExtUI::onIdle -> idle -> cycle
    // This is only a work-around: this should be avoided at a higher level
    if (planner.draining()) {
        return 1;
    }

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
    return cycle();
}

bool processing(void) {
    return server.flags & MARLIN_SFLG_PROCESS;
}

void start_processing(void) {
    server.flags |= MARLIN_SFLG_PROCESS;
    _send_notify_event(Event::StartProcessing, 0, 0);
}

void stop_processing(void) {
    server.flags &= ~MARLIN_SFLG_PROCESS;
    // TODO: disable heaters and safe state
    _send_notify_event(Event::StopProcessing, 0, 0);
}

void do_babystep_Z(float offs) {
    babystep.add_steps(Z_AXIS, offs * planner.settings.axis_steps_per_mm[Z_AXIS]);
    babystep.task();
}

extern void move_axis(float pos, float feedrate, size_t axis) {
    xyze_float_t position = current_position;
    position[axis] = pos;
    current_position[axis] = pos;
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

bool inject_gcode(const char *gcode) {
    queue.inject_P(gcode);
    return true;
}

void settings_save(void) {
#if HAS_BED_PROBE
    SteelSheets::SetZOffset(probe_offset.z);
#endif
#if ENABLED(PIDTEMPBED)
    config_store().pid_bed_p.set(Temperature::temp_bed.pid.Kp);
    config_store().pid_bed_i.set(Temperature::temp_bed.pid.Ki);
    config_store().pid_bed_d.set(Temperature::temp_bed.pid.Kd);
#endif
#if ENABLED(PIDTEMP)
    // Save only first nozzle PID
    config_store().pid_nozzle_p.set(Temperature::temp_hotend[0].pid.Kp);
    config_store().pid_nozzle_i.set(Temperature::temp_hotend[0].pid.Ki);
    config_store().pid_nozzle_d.set(Temperature::temp_hotend[0].pid.Kd);
#endif
}

void settings_load(void) {
    (void)settings.reset();
#if HAS_BED_PROBE
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

    marlin_vars()->fan_check_enabled = config_store().fan_check_enabled.get();
    marlin_vars()->fs_autoload_enabled = config_store().fs_autoload_enabled.get();

    job_id = config_store().job_id.get();

#if ENABLED(PRUSA_TOOLCHANGER)
    // TODO: This is temporary until better offset store method is implemented
    prusa_toolchanger.load_tool_offsets();
#endif
}

void settings_reset(void) {
    (void)settings.reset();
}

uint32_t get_command(void) {
    return server.command;
}

void set_command(uint32_t command) {
    server.command = command;
}

void test_start([[maybe_unused]] const uint64_t test_mask, [[maybe_unused]] const uint8_t tool_mask) {
#if HAS_SELFTEST()
    if (((server.print_state == State::Idle) || (server.print_state == State::Finished) || (server.print_state == State::Aborted)) && (!SelftestInstance().IsInProgress())) {
        SelftestInstance().Start(test_mask, tool_mask);
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
        || server.print_state == State::PrintPreviewToolsMapping
        || server.print_state == State::WaitGui;
}

bool aborting_or_aborted() {
    return (server.print_state >= State::Aborting_Begin && server.print_state <= State::Aborted);
}

bool printer_paused() {
    return server.print_state == State::Paused;
}

void serial_print_start() {
    server.print_state = State::SerialPrintInit;
}

void print_start(const char *filename, marlin_server::PreviewSkipIfAble skip_preview) {
#if HAS_SELFTEST()
    if (SelftestInstance().IsInProgress()) {
        return;
    }
#endif
    if (filename == nullptr) {
        return;
    }

    // handle preview / reprint
    if (server.print_state == State::Finished || server.print_state == State::Aborted) {
        // correctly end previous print
        finalize_print();
        if (fsm_event_queues.GetFsm0() == ClientFSM::Printing) {
            // exit from print screen, if opened
            FSM_DESTROY__LOGGING(Printing);
        }
    }

    switch (server.print_state) {

    case State::Idle:
    case State::Finished:
    case State::Aborted:
    case State::PrintPreviewInit:
    case State::PrintPreviewImage:
    case State::PrintPreviewConfirmed:
    case State::PrintPreviewQuestions:
    case State::PrintPreviewToolsMapping:
        media_print_start__prepare(filename);
        server.print_state = State::WaitGui;
        PrintPreview::Instance().set_skip_if_able(skip_preview);
        break;

    default:
        break;
    }
}

void gui_ready_to_print() {
    switch (server.print_state) {

    case State::WaitGui:
        server.print_state = State::PrintPreviewInit;
        break;

    default:
        log_error(MarlinServer, "Wrong print state, expected: %d, is: %d", State::WaitGui, server.print_state);
        break;
    }
}

void gui_cant_print() {
    switch (server.print_state) {

    case State::WaitGui:
        server.print_state = State::Idle;
        break;

    default:
        log_error(MarlinServer, "Wrong print state, expected: %d, is: %d", State::WaitGui, server.print_state);
        break;
    }
}

void serial_print_finalize(void) {
    switch (server.print_state) {

    case State::Printing:
    case State::Paused:
    case State::Resuming_Reheating:
    case State::Finishing_WaitIdle:
    case State::CrashRecovery_Tool_Pickup:
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
    case State::CrashRecovery_Tool_Pickup:
        server.print_state = State::Aborting_Begin;
        break;

    case State::PrintPreviewInit:
    case State::PrintPreviewImage:
    case State::PrintPreviewConfirmed:
    case State::PrintPreviewQuestions:
    case State::PrintPreviewToolsMapping:
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
        if ((marlin_vars()->hotend(e).target_nozzle > 0)) {
            thermalManager.setTargetHotend(0, e);
            set_temp_to_display(0, e);
        }
    }

    server.print_state = State::CrashRecovery_Tool_Pickup; // Continue with screen to wait for user to pick tools
}

/**
 * @brief Part of crash recovery begin when reason of crash is the toolchanger.
 * @note This has to call FSM_CREATE_WITH_DATA__LOGGING exactly once.
 * @return true on toolcrash when there is no parking and replay and when should break current switch case
 */
static bool crash_recovery_begin_toolchange() {
    Crash_recovery_tool_fsm cr_fsm(prusa_toolchanger.get_enabled_mask(), 0);
    FSM_CREATE_WITH_DATA__LOGGING(CrashRecovery, PhasesCrashRecovery::tool_recovery, cr_fsm.Serialize()); // Ask user to park all dwarves

    if (crash_s.get_state() == Crash_s::REPEAT_WAIT) {
        prepare_tool_pickup(); // If crash happens during toolchange, skip crash recovery and go directly to tool pickup
        return true;
    }
    return false;
}
    #endif /*HAS_TOOLCHANGER()*/

/**
 * @brief Part of crash recovery begin when reason of crash is failed homing.
 * @note This has to call FSM_CREATE_WITH_DATA__LOGGING exactly once.
 * @note Should break current switch case after this.
 */
static void crash_recovery_begin_home() {
    Crash_recovery_fsm cr_fsm(SelftestSubtestState_t::running, SelftestSubtestState_t::undef);
    FSM_CREATE_WITH_DATA__LOGGING(CrashRecovery, PhasesCrashRecovery::home, cr_fsm.Serialize());

    measure_axes_and_home(); // If crash happens during homing, skip crash recovery and go directly to measuring axes / homing
}

    #if ENABLED(AXIS_MEASURE)
/**
 * @brief Part of crash recovery begin when it is a regular crash, axis measure is enabled and this is a repeated crash.
 * @note This has to call FSM_CREATE_WITH_DATA__LOGGING exactly once.
 * @note Do not break current switch case after this, will park and replay.
 */
static void crash_recovery_begin_axis_measure() {
    Crash_recovery_fsm cr_fsm(SelftestSubtestState_t::running, SelftestSubtestState_t::undef);
    FSM_CREATE_WITH_DATA__LOGGING(CrashRecovery, PhasesCrashRecovery::check_X, cr_fsm.Serialize()); // check axes first
}
    #endif /*ENABLED(AXIS_MEASURE)*/

/**
 * @brief Part of crash recovery begin when it is a regular crash.
 * @note This has to call FSM_CREATE_WITH_DATA__LOGGING exactly once.
 * @note Do not break current switch case after this, will park and replay.
 */
static void crash_recovery_begin_crash() {
    Crash_recovery_fsm cr_fsm(SelftestSubtestState_t::running, SelftestSubtestState_t::undef);
    FSM_CREATE_WITH_DATA__LOGGING(CrashRecovery, PhasesCrashRecovery::home, cr_fsm.Serialize());
}
#endif /*ENABLED(CRASH_RECOVERY)*/

void print_resume(void) {
    if (server.print_state == State::Paused) {
        server.print_state = State::Resuming_Begin;
        // pause queuing commands from serial, until resume sequence is finished.
        GCodeQueue::pause_serial_commands = true;

#if ENABLED(POWER_PANIC)
    } else if (server.print_state == State::PowerPanic_AwaitingResume) {
        power_panic::resume_continue();
        server.print_state = State::PowerPanic_Resume;
#endif
    } else {
        print_start(nullptr, marlin_server::PreviewSkipIfAble::all);
    }
}

// Fast temperature recheck.
// Does not check stability of the temperature.
bool print_reheat_ready() {
    // check nozzles
    HOTEND_LOOP() {
        auto &extruder = marlin_vars()->hotend(e);
        if (extruder.target_nozzle != server.resume.nozzle_temp[e] || extruder.temp_nozzle < (extruder.target_nozzle - TEMP_HYSTERESIS)) {
            return false;
        }
    }

    // check bed
    if (marlin_vars()->temp_bed < (marlin_vars()->target_bed - TEMP_BED_HYSTERESIS)) {
        return false;
    }

    return true;
}

#if ENABLED(POWER_PANIC)
void powerpanic_resume_loop(const char *media_SFN_path, uint32_t pos, bool auto_recover) {
    // Open the file
    print_start(media_SFN_path, marlin_server::PreviewSkipIfAble::all);

    crash_s.set_state(Crash_s::PRINTING);

    // Immediately stop to set the print position
    media_print_quick_stop(pos);

    // open printing screen
    FSM_CREATE__LOGGING(Printing);

    // Warn user of possible print fail caused by cold heatbed during PP
    if (!auto_recover) {
        set_warning(WarningType::HeatbedColdAfterPP);
    }

    // enter the main powerpanic resume loop
    server.print_state = auto_recover ? State::PowerPanic_Resume : State::PowerPanic_AwaitingResume;
    static metric_t power = METRIC("power_panic", METRIC_VALUE_EVENT, 0, METRIC_HANDLER_ENABLE_ALL);
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

void nozzle_timeout_on() {
    server.enable_nozzle_temp_timeout = true;
};
void nozzle_timeout_off() {
    server.enable_nozzle_temp_timeout = false;
}
void nozzle_timeout_loop() {
    if ((ticks_ms() - server.paused_ticks > (1000 * PAUSE_NOZZLE_TIMEOUT)) && server.enable_nozzle_temp_timeout) {
        HOTEND_LOOP() {
            if ((marlin_vars()->hotend(e).target_nozzle > 0)) {
                thermalManager.setTargetHotend(0, e);
                set_temp_to_display(0, e);
            }
        }
    }
}

bool heatbreak_fan_check() {
    if (marlin_vars()->fan_check_enabled
#if HAS_TOOLCHANGER()
        && prusa_toolchanger.is_any_tool_active() // Nothing to check
#endif /*HAS_TOOLCHANGER()*/
    ) {
        // Allow fan check only if fan had time to build up RPM (after CFanClt::rpm_stabilization)
        // CFanClt error states are checked in the end of each _server_print_loop()
        auto hb_state = Fans::heat_break(active_extruder).getState();
        if ((hb_state == CFanCtl::running || hb_state == CFanCtl::error_running || hb_state == CFanCtl::error_starting) && !Fans::heat_break(active_extruder).getRPMIsOk()) {
            log_error(MarlinServer, "HeatBreak FAN RPM is not OK - Actual: %d rpm, PWM: %d",
                (int)Fans::heat_break(active_extruder).getActualRPM(),
                Fans::heat_break(active_extruder).getPWM());
            return true;
        }
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

    if (heatbreak_fan_check()) {
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
        oProgressData.oPercentDone.mSetValue(0, 0);
        PrintPreview::Instance().Init();
        server.print_state = State::PrintPreviewImage;
        break;

    case State::PrintPreviewImage:
    case State::PrintPreviewConfirmed:
    case State::PrintPreviewQuestions:
    case State::PrintPreviewToolsMapping: {
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
            FSM_DESTROY__LOGGING(PrintPreview);
            break;

        case PrintPreview::Result::ToolsMapping:
            new_state = State::PrintPreviewToolsMapping;
            break;

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
                enqueue_gcode("G1 E67 F6000"); // push filament into the nozzle - load distance from fsensor into nozzle tuned (hardcoded) for now
            }
#endif
            break;
        }

        server.print_state = new_state;

        break;
    }
    case State::PrintInit:
        server.print_is_serial = false;
        feedrate_percentage = 100;

        // Reset flow factor for all extruders
        HOTEND_LOOP() {
            planner.flow_percentage[e] = 100;
            planner.refresh_e_factor(e);
        }

#if ENABLED(CRASH_RECOVERY)
        crash_s.reset();
        crash_s.reset_crash_counter();
        endstops.enable_globally(true);
        crash_s.set_state(Crash_s::PRINTING);
#endif // ENABLED(CRASH_RECOVERY)
#if ENABLED(CANCEL_OBJECTS)
        cancelable.reset();
        for (auto &cancel_object_name : marlin_vars()->cancel_object_names) {
            cancel_object_name.set(""); // Erase object names
        }
#endif

#if HAS_LOADCELL()
        // Reset Live-Adjust-Z value before every print
        probe_offset.z = 0;
        SteelSheets::SetZOffset(probe_offset.z); // This updates marlin_vers()->z_offset

#endif // HAS_LOADCELL()

        media_print_start();

        print_job_timer.start();
        marlin_vars()->print_start_time = time(nullptr);
        server.print_state = State::Printing;
        switch (fsm_event_queues.GetFsm0()) {
        case ClientFSM::PrintPreview:
            FSM_DESTROY_AND_CREATE__LOGGING(PrintPreview, Printing);
            break;
        case ClientFSM::_none:
            // FIXME make this atomic change. It would require improvements in PrintScreen so that it can re-initialize upon phase change.
            // FYI the DESTROY invoke is in print_start()
            // NOTE this works surely thanks to State::WaitGui being in between the DESTROY and CREATE
            FSM_CREATE__LOGGING(Printing);
            break;
        default:
            log_error(MarlinServer, "Wrong FSM state %d", (int)fsm_event_queues.GetFsm0());
        }
#if HAS_BED_PROBE || HAS_LOADCELL() && ENABLED(PROBE_CLEANUP_SUPPORT)
        server.mbl_failed = false;
#endif
        break;
    case State::SerialPrintInit:
        server.print_is_serial = true;
#if ENABLED(CRASH_RECOVERY)
        crash_s.reset();
        crash_s.reset_crash_counter();
        endstops.enable_globally(true);
        // Crash Detection is disabled during serial printing, because it does not work
        // crash_s.set_state(Crash_s::PRINTING);
#endif // ENABLED(CRASH_RECOVERY)
#if ENABLED(CANCEL_OBJECTS)
        cancelable.reset();
        for (auto &cancel_object_name : marlin_vars()->cancel_object_names) {
            cancel_object_name.set(""); // Erase object names
        }
#endif
        print_job_timer.start();
        marlin_vars()->print_start_time = time(nullptr);
        FSM_CREATE__LOGGING(Serial_printing);
        server.print_state = State::Printing;
        break;

    case State::Printing:
        if (server.print_is_serial) {
            SerialPrinting::print_loop();
        } else {
            switch (media_print_get_state()) {
            case media_print_state_PRINTING:
                break;
            case media_print_state_PAUSED:
                /// TODO don't pause in pause/abort/crash etx.
                server.print_state = State::Pausing_Begin;
                break;
            case media_print_state_NONE:
                server.print_state = State::Finishing_WaitIdle;
                break;
            }
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
        break;
    case State::Resuming_Begin:
#if ENABLED(CRASH_RECOVERY)
    #if ENABLED(AXIS_MEASURE)
        if (crash_s.is_repeated_crash() && xy_axes_length_ok() != Axis_length_t::ok) {
            /// resuming after a crash but axes are not ok => check again
            FSM_CREATE__LOGGING(CrashRecovery);
            measure_axes_and_home();
            break;
        }
    #endif

        // forget the XYZ resume position if requested
        if (crash_s.inhibit_flags & Crash_s::INHIBIT_XYZ_REPOSITIONING) {
            LOOP_XYZ(i) {
                server.resume.pos[i] = current_position[i];
            }
        }
#endif
        resuming_begin();
        break;
    case State::Resuming_Reheating:
        resuming_reheating();
        break;
    case State::Resuming_UnparkHead_XY:
        if (heatbreak_fan_check()) {
            abort_resuming = true;
        }
        if (planner.processing()) {
            break;
        }
        unpark_head_ZE();
        server.print_state = State::Resuming_UnparkHead_ZE;
        break;
    case State::Resuming_UnparkHead_ZE:
        if (heatbreak_fan_check()) {
            abort_resuming = true;
        }

        if (!server.print_is_serial && (media_print_get_state() != media_print_state_PAUSED)) {
            break;
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
        if (!server.print_is_serial) {
            media_print_resume();
        }
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

#if HAS_HEATED_BED
        // Unstuck absorbing heat
        bed_preheat.skip_preheat();
#endif /*HAS_HEATED_BED*/

        media_print_stop();
        queue.clear();

        print_job_timer.stop();
        planner.quick_stop();
        wait_for_heatup = false; // This is necessary because M109/wait_for_hotend can be in progress, we need to abort it

#if ENABLED(CRASH_RECOVERY)
        // TODO: the following should be moved to State::Aborting_ParkHead once the "stopping"
        // state is handled properly
        endstops.enable_globally(false);
        crash_s.write_stat_to_eeprom();
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

        if (axes_need_homing()
#if ENABLED(CRASH_RECOVERY)
            || server.aborting_did_crash_trigger
#endif /*ENABLED(CRASH_RECOVERY)*/
        )
            lift_head(); // It would be dangerous to move XY
        else {
            park_head();
        }

#if ENABLED(PRUSA_MMU2)
        MMU2::mmu2.unload();
#endif

        thermalManager.disable_all_heaters();
#if FAN_COUNT > 0
        thermalManager.set_fan_speed(0, 0);
#endif
        HOTEND_LOOP() {
            set_temp_to_display(0, e);
        }

        server.print_state = State::Aborting_ParkHead;
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
                FSM_DESTROY__LOGGING(Serial_printing);
            }
            finalize_print();
        }
        break;
    case State::Aborting_Preview:
        // Wait for operations to finish
        if (queue.has_commands_queued() || planner.processing()) {
            break;
        }

        if (fsm_event_queues.GetFsm0() == ClientFSM::PrintPreview) { // the printing state can only occur in the Fsm0 queue
            if (fsm_event_queues.GetFsm1() != ClientFSM::_none) { // Cannot destroy FSM0 while FSM1 shows anything (would BSOD)
                break; // Wait for FSM1 to end
            }
        }

        if (PrintPreview::Instance().GetState() == PrintPreview::State::tools_mapping_wait_user) {
            PrintPreview::tools_mapping_cleanup();
        }

        // Can go directly to Idle because we didn't really start printing.
        server.print_state = State::Idle;
        PrintPreview::Instance().ChangeState(IPrintPreview::State::inactive);
        FSM_DESTROY__LOGGING(PrintPreview);
        break;

    case State::Finishing_WaitIdle:
        if (!queue.has_commands_queued() && !planner.processing()) {
#if ENABLED(CRASH_RECOVERY)
            // TODO: the following should be moved to State::Finishing_ParkHead once the "stopping"
            // state is handled properly
            endstops.enable_globally(false);
            crash_s.write_stat_to_eeprom();
            crash_s.reset();
#endif // ENABLED(CRASH_RECOVERY)

#ifdef PARK_HEAD_ON_PRINT_FINISH
            if (!server.print_is_serial) {
                // do not move head if printing via serial
                park_head();
            }
#endif // PARK_HEAD_ON_PRINT_FINISH
            if (print_job_timer.isRunning()) {
                print_job_timer.stop();
            }

            server.print_state = State::Finishing_ParkHead;
        }
        break;
    case State::Finishing_ParkHead:
        if (!queue.has_commands_queued() && !planner.processing()) {
            server.print_state = State::Finished;
            if (server.print_is_serial) {
                FSM_DESTROY__LOGGING(Serial_printing);
            }
            finalize_print();
        }
        break;
    case State::Exit:
        // make the State::Exit state more resilient to repeated calls (e.g. USB drive pulled out prematurely at the end-of-print screen)
        if (fsm_event_queues.GetFsm0() == ClientFSM::Printing) { // the printing state can only occur in the Fsm0 queue
            if (fsm_event_queues.GetFsm1() != ClientFSM::_none) { // Cannot destroy FSM0 while FSM1 shows anything (would BSOD)
                break; // Wait for FSM1 to end
            }
            finalize_print();
            FSM_DESTROY__LOGGING(Printing);
        }
        if (fsm_event_queues.GetFsm0() == ClientFSM::Serial_printing) {
            finalize_print();
        }
        server.print_state = State::Idle;
        break;

#if ENABLED(CRASH_RECOVERY)
    case State::CrashRecovery_Begin: {
        // pause and set correct resume position: this will stop media reading and clear the queue
        // TODO: this is completely broken for crashes coming from serial printing
        pause_print(Pause_Type::Crash, crash_s.sdpos);

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
         * Each of them calls FSM_CREATE_WITH_DATA__LOGGING exactly once.
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
        static metric_t crash_len = METRIC("crash_length", METRIC_VALUE_CUSTOM, 0, METRIC_HANDLER_ENABLE_ALL);
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

        if ((ClientResponseHandler::GetResponseFromPhase(PhasesCrashRecovery::tool_recovery) == Response::Continue)
            && (prusa_toolchanger.get_enabled_mask() == prusa_toolchanger.get_parked_mask())) {

            // Show homing screen, TODO: perhaps a new screen would be better
            Crash_recovery_fsm cr_fsm(SelftestSubtestState_t::running, SelftestSubtestState_t::undef);
            FSM_CHANGE_WITH_DATA__LOGGING(CrashRecovery, PhasesCrashRecovery::home, cr_fsm.Serialize());

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
                FSM_CHANGE_WITH_DATA__LOGGING(CrashRecovery, PhasesCrashRecovery::tool_recovery, cr_fsm.Serialize());

                prepare_tool_pickup();
                break;
            }

            server.print_state = State::CrashRecovery_XY_HOME; // Reheat and resume, unpark is skipped in later stages
        } else {
            Crash_recovery_tool_fsm cr_fsm(prusa_toolchanger.get_enabled_mask(), prusa_toolchanger.get_parked_mask());
            FSM_CHANGE_WITH_DATA__LOGGING(CrashRecovery, PhasesCrashRecovery::tool_recovery, cr_fsm.Serialize());
            gcode.reset_stepper_timeout(); // Prevent disable axis
        }
        break;
    }
    #endif /*HAS_TOOLCHANGER()*/
    case State::CrashRecovery_XY_HOME: {
        if (queue.has_commands_queued() || planner.processing()) {
            break;
        }

        if (axis_unhomed_error(_BV(X_AXIS) | _BV(Y_AXIS)
                | (crash_s.is_homefail_z() ? _BV(Z_AXIS) : 0))) { // Needs homing
            TemporaryBedLevelingState tbs(false); // Disable for the additional homing, keep previous state after homing
            if (!GcodeSuite::G28_no_parser(false, false, 0, false, true, true, crash_s.is_homefail_z())) {
                // Unsuccesfull rehome
                set_axis_is_not_at_home(X_AXIS);
                set_axis_is_not_at_home(Y_AXIS);
                crash_s.count_crash(); // Count as another crash

                if (crash_s.is_repeated_crash()) { // Cannot home repeatedly
                    disable_XY(); // Let user move the carriage
                    Crash_recovery_fsm cr_fsm(SelftestSubtestState_t::undef, SelftestSubtestState_t::undef);
                    FSM_CHANGE_WITH_DATA__LOGGING(CrashRecovery, PhasesCrashRecovery::home_fail, cr_fsm.Serialize()); // Retry screen
                    server.print_state = State::CrashRecovery_HOMEFAIL; // Ask to retry
                }
                break;
            }
        }

        if (!crash_s.is_repeated_crash()) {
            server.print_state = State::Resuming_Begin;
            FSM_DESTROY__LOGGING(CrashRecovery);
            break;
        }
        server.paused_ticks = ticks_ms(); // time when printing paused
    #if ENABLED(AXIS_MEASURE)
        Axis_length_t alok = xy_axes_length_ok();
        if (alok != Axis_length_t::ok) {
            server.print_state = State::CrashRecovery_Axis_NOK;
            Crash_recovery_fsm cr_fsm(axis_length_check(X_AXIS), axis_length_check(Y_AXIS));
            PhasesCrashRecovery pcr = (alok == Axis_length_t::shorter) ? PhasesCrashRecovery::axis_short : PhasesCrashRecovery::axis_long;
            FSM_CHANGE_WITH_DATA__LOGGING(CrashRecovery, pcr, cr_fsm.Serialize());
            break;
        }
    #endif
        Crash_recovery_fsm cr_fsm(SelftestSubtestState_t::undef, SelftestSubtestState_t::undef);
        FSM_CHANGE_WITH_DATA__LOGGING(CrashRecovery, PhasesCrashRecovery::repeated_crash, cr_fsm.Serialize());
        server.print_state = State::CrashRecovery_Repeated_Crash;
        break;
    }
    case State::CrashRecovery_HOMEFAIL: {
        nozzle_timeout_loop();
        switch (ClientResponseHandler::GetResponseFromPhase(PhasesCrashRecovery::home_fail)) {
        case Response::Retry: {
            Crash_recovery_fsm cr_fsm(SelftestSubtestState_t::running, SelftestSubtestState_t::undef);
            FSM_CHANGE_WITH_DATA__LOGGING(CrashRecovery, PhasesCrashRecovery::home, cr_fsm.Serialize()); // Homing screen
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
        switch (ClientResponseHandler::GetResponseFromPhase(PhasesCrashRecovery::axis_NOK)) {
        case Response::Retry:
            measure_axes_and_home();
            break;
        case Response::Resume: /// ignore wrong length of axes
            server.print_state = State::Resuming_Begin;
            FSM_DESTROY__LOGGING(CrashRecovery);
    #if ENABLED(AXIS_MEASURE)
            axes_length_set_ok(); /// ignore re-test of lengths
    #endif
            break;
        case Response::_none:
            break;
        default:
            server.print_state = State::Paused;
            FSM_DESTROY__LOGGING(CrashRecovery);
        }
        gcode.reset_stepper_timeout(); // prevent disable axis
        break;
    }
    case State::CrashRecovery_Repeated_Crash: {
        nozzle_timeout_loop();
        switch (ClientResponseHandler::GetResponseFromPhase(PhasesCrashRecovery::repeated_crash)) {
        case Response::Resume:
            server.print_state = State::Resuming_Begin;
            FSM_DESTROY__LOGGING(CrashRecovery);
            break;
        case Response::_none:
            break;
        default:
            server.print_state = State::Paused;
            FSM_DESTROY__LOGGING(CrashRecovery);
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

    if (marlin_vars()->fan_check_enabled) {
        HOTEND_LOOP() {
            const auto fan_state = Fans::heat_break(e).getState();
            hotendFanErrorChecker[e].checkTrue(fan_state != CFanCtl::error_running && fan_state != CFanCtl::error_starting);
        }
        const auto fan_state = Fans::print(active_extruder).getState();
        printFanErrorChecker.checkTrue(fan_state != CFanCtl::error_running && fan_state != CFanCtl::error_starting);
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
    if (ticks_s() >= 2) { // Start checking 2 seconds after system start
        // This gives 0 deg celsius MINTEMP for heat break temperature reading
        HOTEND_LOOP() {
    #if ENABLED(PRUSA_TOOLCHANGER)
            if (!prusa_toolchanger.is_tool_enabled(e)) {
                continue;
            }
    #endif

            heatBreakThermistorErrorChecker[e].checkTrue(!NEAR_ZERO(thermalManager.degHeatbreak(e)));
            if (thermalManager.degHeatbreak(e) > 10) {
                heatBreakThermistorErrorChecker[e].reset();
            }
        }
    }
#endif

    hotendErrorChecker.checkTrue(Temperature::saneTempReadingHotend(0));

    // Check MCU temperatures
    mcuMaxTempErrorChecker.check(AdcGet::getMCUTemp());
#if HAS_DWARF()
    HOTEND_LOOP() {
        if (prusa_toolchanger.is_tool_enabled(e)) {
            dwarfMaxTempErrorChecker[e].check(buddy::puppies::dwarfs[e].get_mcu_temperature());
        }
    }
#endif /*HAS_DWARF()*/
#if HAS_MODULARBED()
    modbedMaxTempErrorChecker.check(buddy::puppies::modular_bed.mcu_temperature.value);
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
    media_reset_usbh_error();
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
    const constexpr xyz_pos_t park = NOZZLE_PARK_POINT;
    plan_move_by(NOZZLE_PARK_Z_FEEDRATE, 0, 0, _MIN(park.z, Z_MAX_POS - current_position.z));
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

    xyz_pos_t park = NOZZLE_PARK_POINT;
    #ifdef NOZZLE_PARK_POINT_M600
    const xyz_pos_t park_clean = NOZZLE_PARK_POINT_M600;
    if (server.mbl_failed) {
        park = park_clean;
    }
    #endif // NOZZLE_PARK_POINT_M600
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
    marlin_vars()->target_bed = value;
    thermalManager.setTargetBed(value);
}

void set_temp_to_display(float value, uint8_t extruder) {
    marlin_vars()->hotend(extruder).display_nozzle = value;
}

bool get_media_inserted(void) {
    return marlin_vars()->media_inserted;
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

// send notify message (variant8_t) to client queue (called from server thread)
static int _send_notify_to_client(osMessageQId queue, variant8_t msg) {
    // synchronization not necessary because only server thread can write to this queue
    if (queue == 0) {
        return 0;
    }
    if (osMessageAvailableSpace(queue) < 2) {
        return 0;
    }
    osMessagePut(queue, (uint32_t)(msg & 0xFFFFFFFFU), osWaitForever);
    osMessagePut(queue, (uint32_t)(msg >> 32), osWaitForever);
    return 1;
}

// send all FSM messages from the FSM queue
static bool _send_FSM_event_to_client(int client_id, osMessageQId queue) {
    while (1) {
        std::optional<fsm::DequeStates> commands = fsm_event_queues.dequeue(client_id);
        if (!commands) {
            return true; // no event to send, return 'sent' to erase 'send' flag
        }
        marlin_vars()->set_last_fsm_state(commands->current);
        std::pair<uint32_t, uint16_t> data = commands->current.serialize();
        log_debug(FSM, "data sent u32 %d, u16 %d, client %d", data.first, data.second, client_id);

        if (!_send_notify_to_client(queue, variant8_user(data.first, data.second, ftrstd::to_underlying(Event::FSM)))) {
            // unable to send all messages
            return false;
        }
    }
}

// send event notification to client (called from server thread)
static bool _send_notify_event_to_client(int client_id, osMessageQId queue, Event evt_id, uint32_t usr32, uint16_t usr16) {
    variant8_t msg;
    switch (evt_id) {
    case Event::Message:
        msg = server.event_messages[client_id];
        break;
    case Event::FSM:
        return _send_FSM_event_to_client(client_id, queue);
    default:
        msg = variant8_user(usr32, usr16, ftrstd::to_underlying(evt_id));
    }

    bool ret = _send_notify_to_client(queue, msg);

    if (ret) {
        switch (evt_id) {
        case Event::Message:
            // clear sent client message
            server.event_messages[client_id] = variant8_empty();
            break;
        default:
            break;
        }
    }

    return ret;
}

// send event notification to client - multiple events (called from server thread)
// returns mask of successfully sent events
static uint64_t _send_notify_events_to_client(int client_id, osMessageQId queue, uint64_t evt_msk) {
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
            case Event::Startup:
            case Event::MediaInserted:
            case Event::MediaError:
            case Event::MediaRemoved:
            case Event::PrintTimerStarted:
            case Event::PrintTimerPaused:
            case Event::PrintTimerStopped:
            case Event::FilamentRunout:
            case Event::FactoryReset:
            case Event::LoadSettings:
            case Event::StoreSettings:
            case Event::StartProcessing:
            case Event::StopProcessing:
            case Event::FSM: // arguments handled elsewhere
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
            case Event::MeshUpdate:
                if (_send_notify_event_to_client(client_id, queue, evt_id,
                        server.last_mesh_evt.usr32, server.last_mesh_evt.usr16)) {
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
            case Event::PrinterKilled:
            case Event::Error:
            case Event::PlayTone:
            case Event::UserConfirmRequired:
            case Event::SafetyTimerExpired:
            case Event::Message:
            case Event::Reheat:
                sent |= msk; // fake event sent for unused and forced events
                break;
            case Event::Warning:
                if (_send_notify_event_to_client(client_id, queue, evt_id, server.warning_type, 0)) {
                    sent |= msk; // event sent, set bit
                }
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
                // save unsent data of the event for later retransmission
                if (evt_id == Event::MeshUpdate) {
                    server.last_mesh_evt.usr32 = usr32;
                    server.last_mesh_evt.usr16 = usr16;
                } else if (evt_id == Event::Warning) {
                    server.warning_type = usr32;
                }
            } else {
                // event sent, clear flag
                client_msk |= (1 << client_id);
            }
        }
    }
    return client_msk;
}

static void _server_update_gqueue(void) {
    if (server.gqueue != queue.length) {
        server.gqueue = queue.length;
        //		_dbg("gqueue: %2d", server.gqueue);
    }
}

static void _server_update_pqueue(void) {
    if ((server.pqueue_head != planner.block_buffer_head) || (server.pqueue_tail != planner.block_buffer_tail)) {
        server.pqueue_head = planner.block_buffer_head;
        server.pqueue_tail = planner.block_buffer_tail;
        server.pqueue = (server.pqueue_head >= server.pqueue_tail) ? (server.pqueue_head - server.pqueue_tail) : (BLOCK_BUFFER_SIZE + server.pqueue_head - server.pqueue_tail);
        //		_dbg("pqueue: %2d", server.pqueue);
    }
}

// update all server variables
static void _server_update_vars() {
    marlin_vars()->gqueue = server.gqueue;
    marlin_vars()->pqueue = server.pqueue;

    // Get native position
    xyze_pos_t pos_mm, curr_pos_mm;
    planner.get_axis_position_mm(pos_mm);
    curr_pos_mm = current_position;
    LOOP_XYZE(i) {
        marlin_vars()->native_pos[i] = pos_mm[i];
        marlin_vars()->native_curr_pos[i] = curr_pos_mm[i];
    }
    // Convert to logical position
    toLogical(pos_mm);
    toLogical(curr_pos_mm);
    LOOP_XYZE(i) {
        marlin_vars()->logical_pos[i] = pos_mm[i];
        marlin_vars()->logical_curr_pos[i] = curr_pos_mm[i];
    }

    HOTEND_LOOP() {
        auto &extruder = marlin_vars()->hotend(e);

        extruder.temp_nozzle = thermalManager.degHotend(e);
        extruder.target_nozzle = thermalManager.degTargetHotend(e);
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
    marlin_vars()->cancel_object_mask = cancelable.canceled; // Canceled objects
    marlin_vars()->cancel_object_count = cancelable.object_count; // Total number of objects
#endif /*ENABLED(CANCEL_OBJECTS)*/

    marlin_vars()->temp_bed = thermalManager.degBed();
    marlin_vars()->target_bed = thermalManager.degTargetBed();
#if ENABLED(MODULAR_HEATBED)
    marlin_vars()->enabled_bedlet_mask = thermalManager.getEnabledBedletMask();
#endif

    marlin_vars()->z_offset = probe_offset.z;
#if FAN_COUNT > 0
    marlin_vars()->print_fan_speed = thermalManager.fan_speed[0];
#endif
    marlin_vars()->print_speed = static_cast<uint16_t>(feedrate_percentage);

    if (!FirstLayer::isPrinting()) { /// push notifications used for first layer calibration
        uint8_t progress = 0;
        if (oProgressData.oPercentDone.mIsActual(marlin_vars()->print_duration)) {
            progress = static_cast<uint8_t>(oProgressData.oPercentDone.mGetValue());
        } else {
            progress = static_cast<uint8_t>(media_print_get_percent_done());
        }

        marlin_vars()->sd_percent_done = progress;
    }

    marlin_vars()->print_duration = print_job_timer.duration();

    uint8_t media = media_get_state() == media_state_INSERTED ? 1 : 0;
    if (marlin_vars()->media_inserted != media) {
        marlin_vars()->media_inserted = media;
        _send_notify_event(marlin_vars()->media_inserted ? Event::MediaInserted : Event::MediaRemoved, 0, 0);
    }

    uint32_t progress = TIME_TO_END_INVALID;
    if (oProgressData.oPercentDone.mIsActual(marlin_vars()->print_duration)) {
        progress = oProgressData.oTime2End.mGetValue();
    }

    if (marlin_vars()->print_speed == 100 || progress == TIME_TO_END_INVALID) {
        marlin_vars()->time_to_end = progress;
    } else {
        // multiply by 100 is safe, it limits time_to_end to ~21mil. seconds (248 days)
        marlin_vars()->time_to_end = (progress * 100) / marlin_vars()->print_speed;
    }

    if (server.print_state == State::Printing) {
        marlin_vars()->time_to_end.execute_with([&](const uint32_t &time_to_end) {
            if (time_to_end != TIME_TO_END_INVALID) {
                marlin_vars()->print_end_time = time(nullptr) + time_to_end;
            } else {
                marlin_vars()->print_end_time = TIMESTAMP_INVALID;
            }
        });
    }

    marlin_vars()->job_id = job_id;
    marlin_vars()->travel_acceleration = planner.settings.travel_acceleration;

    uint8_t mmu2State =
#if HAS_MMU2()
        uint8_t(MMU2::mmu2.State());
#else
        2;
#endif
    marlin_vars()->mmu2_state = mmu2State;

    bool mmu2FindaPressed =
#if HAS_MMU2()
        MMU2::mmu2.FindaDetectsFilament();
#else
        false;
#endif

    marlin_vars()->mmu2_finda = mmu2FindaPressed;

    marlin_vars()->active_extruder = active_extruder;

    // print state is updated last, to make sure other related variables (like job_id, filenames) are already set when we start print
    marlin_vars()->print_state = static_cast<State>(server.print_state);
}

void bsod_unknown_request(const char *request) {
    bsod("Unknown request %s", request);
}

// request must have 2 chars at least
bool _process_server_valid_request(const char *request, int client_id) {
    const char *data = request + 2;
    uint32_t msk32[2];
    uint32_t tool_mask;
    int ival;

    log_info(MarlinServer, "Processing %s (from %u)", request, client_id);

    switch (Msg(request[1])) {

    case Msg::Gcode:
        //@TODO return value depending on success of enqueueing gcode
        return enqueue_gcode(data);
    case Msg::InjectGcode: {
        unsigned long int iptr = strtoul(data, NULL, 0);
        return inject_gcode((const char *)iptr);
    }
    case Msg::Start:
        start_processing();
        return true;
    case Msg::Stop:
        stop_processing();
        return true;
    case Msg::SetVariable:
        _server_set_var(data);
        return true;
    case Msg::Babystep: {
        float offs;
        if (sscanf(data, "%f", &offs) != 1) {
            return false;
        }
        do_babystep_Z(offs);
        return true;
    }
#if ENABLED(CANCEL_OBJECTS)
    case Msg::CancelObjectID: {
        int obj_id;
        if (sscanf(data, "%d", &obj_id) != 1) {
            return false;
        }

        cancelable.cancel_object(obj_id);
        return true;
    }
    case Msg::UncancelObjectID: {
        int obj_id;
        if (sscanf(data, "%d", &obj_id) != 1) {
            return false;
        }
        cancelable.uncancel_object(obj_id);
        return true;
    }
    case Msg::CancelCurrentObject:
        cancelable.cancel_active_object();
        return true;
#else
    case Msg::CancelObjectID:
    case Msg::UncancelObjectID:
    case Msg::CancelCurrentObject:
        return false;
#endif
    case Msg::ConfigSave:
        settings_save();
        return true;
    case Msg::ConfigLoad:
        settings_load();
        return true;
    case Msg::ConfigReset:
        settings_reset();
        return true;
    case Msg::PrintStart: {
        auto skip_if_able = static_cast<marlin_server::PreviewSkipIfAble>(data[0] - '0');
        assert(skip_if_able < marlin_server::PreviewSkipIfAble::_count);
        print_start(data + 1, skip_if_able);
        return true;
    }
    case Msg::PrintReady:
        gui_ready_to_print();
        return true;
    case Msg::GuiCantPrint:
        gui_cant_print();
        return true;
    case Msg::PrintAbort:
        print_abort();
        return true;
    case Msg::PrintPause:
        print_pause();
        return true;
    case Msg::PrintResume:
        print_resume();
        return true;
    case Msg::PrintExit:
        print_exit();
        return true;
    case Msg::Park:
        park_head();
        return true;
    case Msg::KnobMove:
        ++server.knob_move_counter;
        return true;
    case Msg::KnobClick:
        ++server.knob_click_counter;
        return true;
    case Msg::FSM:
        if (sscanf(data, "%d", &ival) != 1) {
            return false;
        }
        ClientResponseHandler::SetResponse(ival);
        return true;
    case Msg::EventMask:
        if (sscanf(data, "%08" SCNx32 " %08" SCNx32, msk32 + 0, msk32 + 1) != 2) {
            return false;
        }
        server.notify_events[client_id] = msk32[0] + (((uint64_t)msk32[1]) << 32);
        // Send Event::MediaInserted event if media currently inserted
        // This is temporary solution, Event::MediaInserted and Event::MediaRemoved events are replaced
        // with variable media_inserted, but some parts of application still using the events.
        // We need this workaround for app startup.
        if ((server.notify_events[client_id] & make_mask(Event::MediaInserted)) && marlin_vars()->media_inserted) {
            server.client_events[client_id] |= make_mask(Event::MediaInserted);
        }
        return true;
    case Msg::TestStart:
        if (sscanf(data, "%08" SCNx32 " %08" SCNx32 " %08" SCNx32, msk32 + 0, msk32 + 1, &tool_mask) != 3) {
            return false;
        }
        // start selftest
        test_start(msk32[0] + (((uint64_t)msk32[1]) << 32), tool_mask);
        return true;
    case Msg::TestAbort:
        test_abort();
        return true;
    case Msg::Move: {
        float offs;
        float fval;
        unsigned int uival;
        if (sscanf(data, "%f %f %u", &offs, &fval, &uival) != 3) {
            return false;
        }
        move_axis(offs, MMM_TO_MMS(fval), uival);
        return true;
    }
    default:
        bsod_unknown_request(request);
    }
    return false;
}

// process request on server side
// Message consists of
//   1) client ID : char
//   2) '!'
//   3) message ID : char
//   4) data (rest of the message, optional)
// Example of messages: "2!R" or "0!PA546F18B 5D391C83"
static bool _process_server_request(const char *request) {
    if (request == nullptr) {
        return false;
    }
    int client_id = *(request++) - '0';
    if ((client_id < 0) || (client_id >= MARLIN_MAX_CLIENTS)) {
        return true;
    }

    if (strlen(request) < 2 || request[0] != '!') {
        bsod_unknown_request(request);
    }

    const bool processed = _process_server_valid_request(request, client_id);

    // force update of marlin variables after proecssing request -> to ensure client can read latest variables after request completion
    _server_update_vars();

    Event evt_result = processed ? Event::Acknowledge : Event::NotAcknowledge;
    if (!_send_notify_event_to_client(client_id, marlin_client::marlin_client_queue[client_id], evt_result, 0, 0)) {
        // FIXME: Take care of resending process elsewhere.
        server.client_events[client_id] |= make_mask(evt_result); // set bit if notification not sent
    }
    return processed;
}

// set variable from string request
static void _server_set_var(const char *const request) {
    const char *request_end = request + MARLIN_MAX_REQUEST; // todo: this is not exactly correct, but we don't know the correct length anymore

    uintptr_t variable_identifier;
    auto [request_value, error] = std::from_chars(request, request_end, variable_identifier);
    if (error != std::errc {}) {
        bsod("_server_set_var: identifier");
    }
    // skip space after variable_identifier
    request_value += 1;

    // Set normal (non-extruder) variables
    if (variable_identifier == reinterpret_cast<uintptr_t>(&marlin_vars()->target_bed)) {
        marlin_vars()->target_bed.from_string(request_value, request_end);
        thermalManager.setTargetBed(marlin_vars()->target_bed);
        return;
    }
    if (variable_identifier == reinterpret_cast<uintptr_t>(&marlin_vars()->z_offset)) {
        marlin_vars()->z_offset.from_string(request_value, request_end);
#if HAS_BED_PROBE
        probe_offset.z = marlin_vars()->z_offset;
#endif // HAS_BED_PROBE
        return;
    }
    if (variable_identifier == reinterpret_cast<uintptr_t>(&marlin_vars()->print_fan_speed)) {
        marlin_vars()->print_fan_speed.from_string(request_value, request_end);
#if FAN_COUNT > 0
        thermalManager.set_fan_speed(0, marlin_vars()->print_fan_speed);
#endif
        return;
    }
    if (variable_identifier == reinterpret_cast<uintptr_t>(&marlin_vars()->print_speed)) {
        marlin_vars()->print_speed.from_string(request_value, request_end);
        feedrate_percentage = (int16_t)marlin_vars()->print_speed;
        return;
    }
    if (variable_identifier == reinterpret_cast<uintptr_t>(&marlin_vars()->fan_check_enabled)) {
        marlin_vars()->fan_check_enabled.from_string(request_value, request_end);
        return;
    }
    if (variable_identifier == reinterpret_cast<uintptr_t>(&marlin_vars()->fs_autoload_enabled)) {
        marlin_vars()->fs_autoload_enabled.from_string(request_value, request_end);
        return;
    }

    // Now see if extruder variable is set
    HOTEND_LOOP() {
        auto &extruder = marlin_vars()->hotend(e);
        if (reinterpret_cast<uintptr_t>(&extruder.target_nozzle) == variable_identifier) {
            extruder.target_nozzle.from_string(request_value, request_end);

            // if print is paused we want to change the resume temp and turn off timeout
            // this prevents going back to temperature before pause and enables to heat nozzle during pause
            if (server.print_state == State::Paused) {
                nozzle_timeout_off();
                server.resume.nozzle_temp[e] = extruder.target_nozzle;
            }
            thermalManager.setTargetHotend(extruder.target_nozzle, e);
            return;
        } else if (reinterpret_cast<uintptr_t>(&extruder.flow_factor) == variable_identifier) {
            extruder.flow_factor.from_string(request_value, request_end);
            planner.flow_percentage[e] = (int16_t)extruder.flow_factor;
            planner.refresh_e_factor(e);
            return;
        } else if (reinterpret_cast<uintptr_t>(&extruder.display_nozzle) == variable_identifier) {
            extruder.display_nozzle.from_string(request_value, request_end);
            return;
        }
    }

    // if we got here, no variable was set, return error
    bsod("unimplemented _server_set_var for var_id %i", (int)variable_identifier);
}

#ifdef DEBUG_FSENSOR_IN_HEADER
int _is_in_M600_flg = 0;
#endif

void _fsm_create(ClientFSM type, fsm::BaseData data, const char *fnc, const char *file, int line) {
    fsm_event_queues.PushCreate(type, data, fnc, file, line);
    _send_notify_event(Event::FSM, 0, 0); // do not send data, _send_notify_event_to_client does not use them for this event
}

void fsm_destroy(ClientFSM type, const char *fnc, const char *file, int line) {
    fsm_event_queues.PushDestroy(type, fnc, file, line);
    _send_notify_event(Event::FSM, 0, 0); // do not send data, _send_notify_event_to_client does not use them for this event
}

void _fsm_change(ClientFSM type, fsm::BaseData data, const char *fnc, const char *file, int line) {
    fsm_event_queues.PushChange(type, data, fnc, file, line);
    _send_notify_event(Event::FSM, 0, 0); // do not send data, _send_notify_event_to_client does not use them for this event
}

void _fsm_destroy_and_create(ClientFSM old_type, ClientFSM new_type, fsm::BaseData data, const char *fnc, const char *file, int line) {
    fsm_event_queues.PushDestroyAndCreate(old_type, new_type, data, fnc, file, line);
    _send_notify_event(Event::FSM, 0, 0); // do not send data, _send_notify_event_to_client does not use them for this event
}

void set_warning(WarningType type) {
    _log_event(LOG_SEVERITY_WARNING, &LOG_COMPONENT(MarlinServer), "Warning type %d set", (int)type);

    const Event evt_id = Event::Warning;
    _send_notify_event(evt_id, uint32_t(type), 0);
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
        _fsm_change(s_data.type, fsm::BaseData(s_data.phase, activeInstance->serialize(progress)), __PRETTY_FUNCTION__, __FILE__, __LINE__);
    }
}

FSM_notifier::~FSM_notifier() {
    s_data = temp_data;
    activeInstance = nullptr;
}

/*****************************************************************************/
// ClientResponseHandler
// define static member
// UINT32_MAX is used as no response from client
std::atomic<uint32_t> ClientResponseHandler::server_side_encoded_response = UINT32_MAX;

uint8_t get_var_sd_percent_done() {
    return marlin_vars()->sd_percent_done;
}

void set_var_sd_percent_done(uint8_t value) {
    marlin_vars()->sd_percent_done = value;
}

void marlin_msg_to_str(const marlin_server::Msg id, char *str) {
    str[0] = '!';
    str[1] = ftrstd::to_underlying(id);
    str[2] = 0;
}

} // namespace marlin_server

#if _DEBUG && PRINTER_IS_PRUSA_XL
/// @note Hacky link for Marlin.cpp used for development.
/// @todo Remove when stepper timeout screen is solved properly.
void marlin_server_steppers_timeout_warning() {
    marlin_server::set_warning(WarningType::SteppersTimeout);
}
#endif /*devel XL*/

//-----------------------------------------------------------------------------
// ExtUI event handlers

namespace ExtUI {

using namespace marlin_server;

void onStartup() {
    _log_event(LOG_SEVERITY_INFO, &LOG_COMPONENT(MarlinServer), "ExtUI: onStartup");
    _send_notify_event(Event::Startup, 0, 0);
}

void onIdle() {
    idle();
    if (idle_cb) {
        idle_cb();
    }
}

void onPrinterKilled(PGM_P const msg, PGM_P const component) {
    _log_event(LOG_SEVERITY_INFO, &LOG_COMPONENT(MarlinServer), "Printer killed: %s", msg);
    vTaskEndScheduler();
    wdt_iwdg_refresh(); // watchdog reset
    fatal_error(msg, component);
}

void onMediaInserted() {
    _log_event(LOG_SEVERITY_INFO, &LOG_COMPONENT(MarlinServer), "ExtUI: onMediaInserted");
    _send_notify_event(Event::MediaInserted, 0, 0);
}

void onMediaError() {
    _log_event(LOG_SEVERITY_INFO, &LOG_COMPONENT(MarlinServer), "ExtUI: onMediaError");
    _send_notify_event(Event::MediaError, 0, 0);
}

void onMediaRemoved() {
    _log_event(LOG_SEVERITY_INFO, &LOG_COMPONENT(MarlinServer), "ExtUI: onMediaRemoved");
    _send_notify_event(Event::MediaRemoved, 0, 0);
}

void onPlayTone(const uint16_t frequency, const uint16_t duration) {
    _log_event(LOG_SEVERITY_INFO, &LOG_COMPONENT(MarlinServer), "ExtUI: onPlayTone");
    _send_notify_event(Event::PlayTone, frequency, duration);
}

void onPrintTimerStarted() {
    _log_event(LOG_SEVERITY_INFO, &LOG_COMPONENT(MarlinServer), "ExtUI: onPrintTimerStarted");
    _send_notify_event(Event::PrintTimerStarted, 0, 0);
}

void onPrintTimerPaused() {
    _log_event(LOG_SEVERITY_INFO, &LOG_COMPONENT(MarlinServer), "ExtUI: onPrintTimerPaused");
    _send_notify_event(Event::PrintTimerPaused, 0, 0);
}

void onPrintTimerStopped() {
    _log_event(LOG_SEVERITY_INFO, &LOG_COMPONENT(MarlinServer), "ExtUI: onPrintTimerStopped");
    _send_notify_event(Event::PrintTimerStopped, 0, 0);
}

void onFilamentRunout([[maybe_unused]] const extruder_t extruder) {
    _log_event(LOG_SEVERITY_INFO, &LOG_COMPONENT(MarlinServer), "ExtUI: onFilamentRunout");
    _send_notify_event(Event::FilamentRunout, 0, 0);
}

void onUserConfirmRequired(const char *const msg) {
    _log_event(LOG_SEVERITY_INFO, &LOG_COMPONENT(MarlinServer), "ExtUI: onUserConfirmRequired: %s", msg);
    _send_notify_event(Event::UserConfirmRequired, 0, 0);
}

#if HAS_BED_PROBE || HAS_LOADCELL() && ENABLED(PROBE_CLEANUP_SUPPORT)
static void mbl_error(int error_code) {
    if (server.print_state != State::Printing && server.print_state != State::Pausing_Begin) {
        return;
    }

    server.print_state = State::Pausing_Failed_Code;
    /// pause immediatelly to save current file position
    pause_print(Pause_Type::Repeat_Last_Code);
    server.mbl_failed = true;
    _send_notify_event(Event::Error, error_code, 0);
}
#endif

void onStatusChanged(const char *const msg) {
    if (!msg) {
        return; // ignore errorneous nullptr messages
    }

    static bool pending_err_msg = false;

    _log_event(LOG_SEVERITY_INFO, &LOG_COMPONENT(MarlinServer), "ExtUI: onStatusChanged: %s", msg);
    _send_notify_event(Event::StatusChanged, 0, 0); // this includes MMU:P progress messages - just plain textual information
    if (msg != nullptr && strcmp(msg, "Prusa-mini Ready.") == 0) {
    } // TODO
    else if (strcmp(msg, "TMC CONNECTION ERROR") == 0) {
        _send_notify_event(Event::Error, MARLIN_ERR_TMCDriverError, 0);
    } else {
        if (!is_abort_state(server.print_state)) {
            pending_err_msg = false;
        }
        if (!pending_err_msg) {
/// FIXME: Message through Marlin's UI could be delayed and we won't pause print at the MBL command
#if HAS_BED_PROBE
            if (strcmp(msg, MSG_ERR_PROBING_FAILED) == 0) {
                mbl_error(MARLIN_ERR_ProbingFailed);
                pending_err_msg = true;
            }
#endif
#if HAS_LOADCELL() && ENABLED(PROBE_CLEANUP_SUPPORT)
            if (strcmp(msg, MSG_ERR_NOZZLE_CLEANING_FAILED) == 0) {
                mbl_error(MARLIN_ERR_NozzleCleaningFailed);
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
    _log_event(LOG_SEVERITY_INFO, &LOG_COMPONENT(MarlinServer), "ExtUI: onFactoryReset");
    _send_notify_event(Event::FactoryReset, 0, 0);
}

void onLoadSettings(char const *) {
    _log_event(LOG_SEVERITY_INFO, &LOG_COMPONENT(MarlinServer), "ExtUI: onLoadSettings");
    _send_notify_event(Event::LoadSettings, 0, 0);
}

void onStoreSettings(char *) {
    _log_event(LOG_SEVERITY_INFO, &LOG_COMPONENT(MarlinServer), "ExtUI: onStoreSettings");
    _send_notify_event(Event::StoreSettings, 0, 0);
}

void onConfigurationStoreWritten([[maybe_unused]] bool success) {
    _log_event(LOG_SEVERITY_INFO, &LOG_COMPONENT(MarlinServer), "ExtUI: onConfigurationStoreWritten");
}

void onConfigurationStoreRead([[maybe_unused]] bool success) {
    _log_event(LOG_SEVERITY_INFO, &LOG_COMPONENT(MarlinServer), "ExtUI: onConfigurationStoreRead");
}

void onMeshUpdate(const uint8_t xpos, const uint8_t ypos, const float zval) {
    _log_event(LOG_SEVERITY_DEBUG, &LOG_COMPONENT(MarlinServer), "ExtUI: onMeshUpdate x: %u, y: %u, z: %.2f", xpos, ypos, (double)zval);
    uint32_t usr32 = variant8_get_ui32(variant8_flt(zval));
    uint16_t usr16 = xpos | ((uint16_t)ypos << 8);
    _send_notify_event(Event::MeshUpdate, usr32, usr16);
}

} // namespace ExtUI

alignas(std::max_align_t) uint8_t FSMExtendedDataManager::extended_data_buffer[FSMExtendedDataManager::buffer_size] = { 0 };
size_t FSMExtendedDataManager::identifier = { 0 };
