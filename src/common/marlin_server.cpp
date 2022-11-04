// marlin_server.cpp

#include "marlin_server.h"
#include "marlin_server.hpp"
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h> //strncmp
#include <assert.h>

#include "marlin_server.h"
#include "marlin_server.hpp"
#include "marlin_print_preview.hpp"
#include "app.h"
#include "bsod.h"
#include "timing.h"
#include "cmsis_os.h"
#include "log.h"

#include "../Marlin/src/lcd/extui/ui_api.h"
#include "../Marlin/src/gcode/queue.h"
#include "../Marlin/src/gcode/parser.h"
#include "../Marlin/src/module/planner.h"
#include "../Marlin/src/module/stepper.h"
#include "../Marlin/src/module/endstops.h"
#include "../Marlin/src/module/temperature.h"
#include "../Marlin/src/module/probe.h"
#include "../Marlin/src/module/settings.h"
#include "../Marlin/src/module/printcounter.h"
#include "../Marlin/src/feature/babystep.h"
#include "../Marlin/src/feature/bedlevel/bedlevel.h"
#include "../Marlin/src/feature/pause.h"
#include "../Marlin/src/feature/prusa/measure_axis.h"
#include "../Marlin/src/feature/prusa/homing.h"
#include "../Marlin/src/libs/nozzle.h"
#include "../Marlin/src/core/language.h" //GET_TEXT(MSG)
#include "../Marlin/src/gcode/gcode.h"
#include "../Marlin/src/gcode/lcd/M73_PE.h"
#include "../Marlin/src/feature/print_area.h"

#include "hwio.h"
#include "eeprom.h"
#include "media.h"
#include "wdt.h"
#include "../marlin_stubs/G26.hpp"
#include "fsm_types.hpp"
#include "odometer.hpp"
#include "metric.h"

#include <option/has_gui.h>

#if HAS_SELFTEST
    #include "printer_selftest.hpp"
#endif

#include "SteelSheets.hpp"
static_assert(MARLIN_VAR_MAX < 64, "MarlinAPI: Too many variables");

#ifdef MINDA_BROKEN_CABLE_DETECTION
    #include "Z_probe.hpp" //get_Z_probe_endstop_hits
#endif

#if ENABLED(CRASH_RECOVERY)
    #include "../Marlin/src/feature/prusa/crash_recovery.h"
    #include "crash_recovery_type.hpp"
    #include "selftest_axis.h"
#endif
#if ENABLED(POWER_PANIC)
    #include "power_panic.hpp"
#endif

static_assert(MARLIN_VAR_MAX < 64, "MarlinAPI: Too many variables");

namespace {

struct marlin_server_t {
    marlin_vars_t vars;                            // cached variables
    uint64_t notify_events[MARLIN_MAX_CLIENTS];    // event notification mask - message filter
    uint64_t notify_changes[MARLIN_MAX_CLIENTS];   // variable change notification mask - message filter
    uint64_t client_events[MARLIN_MAX_CLIENTS];    // client event mask - unsent messages
    uint64_t client_changes[MARLIN_MAX_CLIENTS];   // client variable change mask - unsent messages
    variant8_t event_messages[MARLIN_MAX_CLIENTS]; // last MARLIN_EVT_Message for clients, cannot use cvariant, destructor would free memory
    uint64_t update_vars;                          // variable update mask
    marlin_print_state_t print_state;              // printing state (printing, paused, ...)
    uint32_t paused_ticks;                         // tick count in moment when printing paused
    resume_state_t resume;                         // resume data (state before pausing)
    bool enable_nozzle_temp_timeout;               //enables nozzle temperature timeout in print pause
    struct {
        uint32_t usr32;
        uint16_t usr16;
    } last_mesh_evt;
    uint32_t warning_type;
    int request_len;
    uint32_t last_update;   // last update tick count
    uint32_t command;       // actually running command
    uint32_t command_begin; // variable for notification
    uint32_t command_end;   // variable for notification
    uint32_t knob_click_counter;
    uint32_t knob_move_counter;
    uint16_t flags; // server flags (MARLIN_SFLG)
    char request[MARLIN_MAX_REQUEST];
    uint8_t idle_cnt;    // idle call counter
    uint8_t pqueue_head; // copy of planner.block_buffer_head
    uint8_t pqueue_tail; // copy of planner.block_buffer_tail
    uint8_t pqueue;      // calculated number of records in planner queue
    uint8_t gqueue;      // copy of queue.length - number of commands in gcode queue
#if ENABLED(CRASH_RECOVERY)
    /// length of axes measured after crash
    /// negative numbers represent undefined length
    xy_float_t axis_length = { -1, -1 };
    Measure_axis *measure_axis = nullptr;
#endif // ENABLED(CRASH_RECOVERY)
#if HAS_BED_PROBE
    bool mbl_failed;
#endif
};

marlin_server_t marlin_server; // server structure - initialize task to zero

enum class Pause_Type {
    Pause,
    Repeat_Last_Code,
    Crash
};

fsm::SmartQueue fsm_event_queues[MARLIN_MAX_CLIENTS];

template <WarningType p_warning, bool p_disableHotend>
class ErrorChecker {
public:
    ErrorChecker()
        : m_failed(false) {};

    void checkTrue(bool condition) {
        if (!condition && !m_failed) {
            set_warning(p_warning);
            if (marlin_server.print_state == mpsPrinting) {
                marlin_server.print_state = mpsPausing_Begin;
            }
            if (p_disableHotend) {
                static_assert(1 == HOTENDS, "Unimplemented for more hotends.");
                thermalManager.setTargetHotend(0, 0);
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
            if (marlin_server.print_state == mpsPrinting) {
                m_postponeFullPrintFan = true;
            } else {
#if FAN_COUNT > 0
                thermalManager.set_fan_speed(0, 255);
#endif
            }
        }
        ErrorChecker::checkTrue(condition);
        if (condition)
            reset();
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

#ifdef NEW_FANCTL
ErrorChecker<WarningType::HotendFanError, true> hotendFanErrorChecker;
ErrorChecker<WarningType::PrintFanError, false> printFanErrorChecker;
#endif
HotendErrorChecker hotendErrorChecker;
} //end anonymous namespace

bool can_stop_wait_for_heatup_var = false;
bool can_stop_wait_for_heatup() { return can_stop_wait_for_heatup_var; }
void can_stop_wait_for_heatup(bool val) { can_stop_wait_for_heatup_var = val; }

extern "C" {

LOG_COMPONENT_DEF(MarlinServer, LOG_SEVERITY_INFO);

//-----------------------------------------------------------------------------
// variables

osThreadId marlin_server_task = 0;    // task handle
osMessageQId marlin_server_queue = 0; // input queue (uint8_t)
osSemaphoreId marlin_server_sema = 0; // semaphore handle

#ifdef DEBUG_FSENSOR_IN_HEADER
uint32_t *pCommand = &marlin_server.command;
#endif
marlin_server_idle_t *marlin_server_idle_cb = 0; // idle callback

void _add_status_msg(const char *const popup_msg) {
    //I could check client mask here
    for (size_t i = 0; i < MARLIN_MAX_CLIENTS; ++i) {
        variant8_t *pvar = &(marlin_server.event_messages[i]);
        variant8_set_type(pvar, VARIANT8_PCHAR);
        variant8_done(&pvar);                                                      //destroy unsent message - free dynamic memory
        marlin_server.event_messages[i] = variant8_pchar((char *)popup_msg, 0, 1); //variant malloc - detached on send
        variant8_set_type(&(marlin_server.event_messages[i]), VARIANT8_USER);      //set user type so client can recognize it as event
        variant8_set_usr8(&(marlin_server.event_messages[i]), MARLIN_EVT_Message);
    }
}

//-----------------------------------------------------------------------------
// external variables from marlin_client

extern osThreadId marlin_client_task[MARLIN_MAX_CLIENTS];    // task handles
extern osMessageQId marlin_client_queue[MARLIN_MAX_CLIENTS]; // input queue handles (uint32_t)

//-----------------------------------------------------------------------------
// forward declarations of private functions

static void _server_print_loop(void);
static int _send_notify_to_client(osMessageQId queue, variant8_t msg);
static bool _send_notify_event_to_client(int client_id, osMessageQId queue, MARLIN_EVT_t evt_id, uint32_t usr32, uint16_t usr16);
static uint64_t _send_notify_events_to_client(int client_id, osMessageQId queue, uint64_t evt_msk);
static uint8_t _send_notify_event(MARLIN_EVT_t evt_id, uint32_t usr32, uint16_t usr16);
static int _send_notify_change_to_client(osMessageQId queue, uint8_t var_id, variant8_t var);
static uint64_t _send_notify_changes_to_client(int client_id, osMessageQId queue, uint64_t var_msk);
static void _set_notify_change(uint8_t var_id);
static void _server_update_gqueue(void);
static void _server_update_pqueue(void);
static uint64_t _server_update_vars(uint64_t force_update_msk);
static bool _process_server_request(const char *request);
static int _server_set_var(const char *const name_val_str);
static void _server_update_and_notify(int client_id, uint64_t update);

//-----------------------------------------------------------------------------
// server side functions

void marlin_server_init(void) {
    int i;
    marlin_server = marlin_server_t();
    osMessageQDef(serverQueue, MARLIN_SERVER_QUEUE, uint8_t);
    marlin_server_queue = osMessageCreate(osMessageQ(serverQueue), NULL);
    osSemaphoreDef(serverSema);
    marlin_server_sema = osSemaphoreCreate(osSemaphore(serverSema), 1);
    marlin_server.flags = MARLIN_SFLG_STARTED;
    for (i = 0; i < MARLIN_MAX_CLIENTS; i++) {
        marlin_server.notify_events[i] = MARLIN_EVT_MSK(MARLIN_EVT_Acknowledge) | MARLIN_EVT_MSK(MARLIN_EVT_Startup) | MARLIN_EVT_MSK(MARLIN_EVT_StartProcessing); // by default only ack, startup and processing
        marlin_server.notify_changes[i] = 0;                                                                                                                       // by default nothing
    }
    marlin_server_task = osThreadGetId();
    marlin_server.update_vars = MARLIN_VAR_MSK_DEF;
    marlin_server.vars.media_LFN = media_print_filename();
    marlin_server.vars.media_SFN_path = media_print_filepath();
    marlin_server.enable_nozzle_temp_timeout = true;
    can_stop_wait_for_heatup(false);
#if HAS_BED_PROBE
    marlin_server.mbl_failed = false;
#endif
}

void print_fan_spd() {
#ifdef NEW_FANCTL
    if (DEBUGGING(INFO)) {
        static int time = 0;
        static int last_prt = 0;
        time = ticks_ms();
        int timediff = time - last_prt;
        if (timediff >= 1000) {
            SERIAL_ECHOPGM("Tacho_FANPR ", fanCtlPrint.getActualRPM());
            serial_print_P("rpm ");
            SERIAL_EOL();
            SERIAL_ECHOPGM("Tacho_FANHB ", fanCtlHeatBreak.getActualRPM());
            serial_print_P("rpm ");
            SERIAL_EOL();
            last_prt = time;
        }
    }
#endif // NEW_FANCTL
}

#ifdef MINDA_BROKEN_CABLE_DETECTION
static void print_Z_probe_cnt() {
    if (DEBUGGING(INFO)) {
        static uint32_t last = 0;
        static uint32_t actual = 0;
        actual = get_Z_probe_endstop_hits();
        if (last != actual) {
            last = actual;
            SERIAL_ECHOPGM("Z Endstop hit ", actual);
            serial_print_P(" times.");
            SERIAL_EOL();
        }
    }
}
#endif
int marlin_server_cycle(void) {

    static int processing = 0;
    if (processing)
        return 0;
    processing = 1;
    bool call_print_loop = true;
#if HAS_SELFTEST
    if (SelftestInstance().IsInProgress()) {
        SelftestInstance().Loop();
        call_print_loop = false;
    }
#endif

    if (call_print_loop)
        _server_print_loop(); // we need call print loop here because it must be processed while blocking commands (M109)

    FSM_notifier::SendNotification();

    print_fan_spd();

#ifdef MINDA_BROKEN_CABLE_DETECTION
    print_Z_probe_cnt();
#endif

    int count = 0;
    int client_id;
    uint64_t msk = 0;
    uint64_t changes = 0;
    osMessageQId queue;
    osEvent ose;
    uint32_t tick;
    char ch;
    if (marlin_server.flags & MARLIN_SFLG_PENDREQ) {
        if (_process_server_request(marlin_server.request)) {
            marlin_server.request_len = 0;
            count++;
            marlin_server.flags &= ~MARLIN_SFLG_PENDREQ;
        }
    }
    if ((marlin_server.flags & MARLIN_SFLG_PENDREQ) == 0)
        while ((ose = osMessageGet(marlin_server_queue, 0)).status == osEventMessage) {
            ch = (char)((uint8_t)(ose.value.v));
            switch (ch) {
            case '\r':
            case '\n':
                ch = 0;
                break;
            }
            if (marlin_server.request_len < MARLIN_MAX_REQUEST)
                marlin_server.request[marlin_server.request_len++] = ch;
            else {
                //TODO: request too long
                marlin_server.request_len = 0;
            }
            if ((ch == 0) && (marlin_server.request_len > 1)) {
                if (_process_server_request(marlin_server.request)) {
                    marlin_server.request_len = 0;
                    count++;
                } else {
                    marlin_server.flags |= MARLIN_SFLG_PENDREQ;
                    break;
                }
            }
        }
    // update gqueue (gcode queue)
    _server_update_gqueue();
    // update pqueue (planner queue)
    _server_update_pqueue();
    // update variables
    tick = ticks_ms();
    if ((tick - marlin_server.last_update) > MARLIN_UPDATE_PERIOD) {
        marlin_server.last_update = tick;
        changes = _server_update_vars(marlin_server.update_vars);
    }

    // send notifications to clients
    for (client_id = 0; client_id < MARLIN_MAX_CLIENTS; client_id++)
        if ((queue = marlin_client_queue[client_id]) != 0) {
            marlin_server.client_changes[client_id] |= (changes & marlin_server.notify_changes[client_id]);
            // send change notifications, clear bits for successful sent notification
            if ((msk = marlin_server.client_changes[client_id]) != 0)
                marlin_server.client_changes[client_id] &= ~_send_notify_changes_to_client(client_id, queue, msk);
            // send events to client only if all variables were sent already, otherwise, the message buffer is full
            // clear bits for successful sent notification
            if ((marlin_server.client_changes[client_id]) == 0)
                if ((msk = marlin_server.client_events[client_id]) != 0)
                    marlin_server.client_events[client_id] &= ~_send_notify_events_to_client(client_id, queue, msk);
        }
    if ((marlin_server.flags & MARLIN_SFLG_PROCESS) == 0)
        wdt_iwdg_refresh(); // this prevents iwdg reset while processing disabled
    processing = 0;
    return count;
}

void static marlin_server_finalize_print() {
#if ENABLED(POWER_PANIC)
    power_panic::reset();
#endif
    Odometer_s::instance().add_time(marlin_server.vars.print_duration);
    print_area.reset_bounding_rect();
}

static const uint8_t MARLIN_IDLE_CNT_BUSY = 1;

#if ANY(CRASH_RECOVERY, POWER_PANIC)
static void marlin_server_check_crash() {
    // reset the nested loop check once per main server iteration
    crash_s.loop = false;

    #if ENABLED(POWER_PANIC)
    // handle server state-change overrides happening in the ISRs here (and nowhere else)
    if (power_panic::ac_fault_state == power_panic::AcFaultState::Triggered) {
        marlin_server.print_state = mpsPowerPanic_acFault;
        return;
    }
    #endif
    if (crash_s.get_state() == Crash_s::TRIGGERED_ISR) {
        marlin_server.print_state = mpsCrashRecovery_Begin;
        return;
    }
}
#endif // ENABLED(CRASH_RECOVERY)

int marlin_server_loop(void) {
#if ANY(CRASH_RECOVERY, POWER_PANIC)
    marlin_server_check_crash();
#endif
    if (marlin_server.idle_cnt >= MARLIN_IDLE_CNT_BUSY)
        if (marlin_server.flags & MARLIN_SFLG_BUSY) {
            log_debug(MarlinServer, "State: Ready");
            marlin_server.flags &= ~MARLIN_SFLG_BUSY;
            if ((marlin_server.command != MARLIN_CMD_NONE) && (marlin_server.command != MARLIN_CMD_M600)) {
                _send_notify_event(MARLIN_EVT_CommandEnd, marlin_server.command, 0);
                marlin_server.command = MARLIN_CMD_NONE;
            }
        }
    marlin_server.idle_cnt = 0;
    media_loop();
    return marlin_server_cycle();
}

int marlin_server_idle(void) {
    // TODO: avoid a re-entrant cycle caused by:
    // marlin_server_cycle -> loop -> idle -> MarlinUI::update() -> ExtUI::onIdle -> marlin_server_idle -> marlin_server_cycle
    // This is only a work-around: this should be avoided at a higher level
    if (planner.draining())
        return 1;

    if (marlin_server.idle_cnt < MARLIN_IDLE_CNT_BUSY)
        marlin_server.idle_cnt++;
    else if ((marlin_server.flags & MARLIN_SFLG_BUSY) == 0) {

        log_debug(MarlinServer, "State: Busy");
        marlin_server.flags |= MARLIN_SFLG_BUSY;
        if (parser.command_letter == 'G')
            switch (parser.codenum) {
            case 28:
            case 29:
                marlin_server.command = MARLIN_CMD_G + parser.codenum;
                break;
            }
        else if (parser.command_letter == 'M')
            switch (parser.codenum) {
            case 109:
            case 190:
            case 303:
            //case 600: // hacked in gcode (_force_M600_notify)
            case 701:
            case 702:
                marlin_server.command = MARLIN_CMD_M + parser.codenum;
                break;
            }
        if (marlin_server.command != MARLIN_CMD_NONE) {
            marlin_server.command_begin = marlin_server.command;
            marlin_server.command_end = marlin_server.command;
            _send_notify_event(MARLIN_EVT_CommandBegin, marlin_server.command, 0);
        }
    }
    return marlin_server_cycle();
}

int marlin_server_processing(void) {
    return (marlin_server.flags & MARLIN_SFLG_PROCESS) ? 1 : 0;
}

void marlin_server_start_processing(void) {
    marlin_server.flags |= MARLIN_SFLG_PROCESS;
    _send_notify_event(MARLIN_EVT_StartProcessing, 0, 0);
}

void marlin_server_stop_processing(void) {
    marlin_server.flags &= ~MARLIN_SFLG_PROCESS;
    //TODO: disable heaters and safe state
    _send_notify_event(MARLIN_EVT_StopProcessing, 0, 0);
}

marlin_vars_t *marlin_server_vars(void) {
    return &(marlin_server.vars);
}

void marlin_server_do_babystep_Z(float offs) {
    babystep.add_steps(Z_AXIS, offs * planner.settings.axis_steps_per_mm[Z_AXIS]);
    babystep.task();
}

extern void marlin_server_move_axis(float pos, float feedrate, size_t axis) {
    xyze_float_t position = current_position;
    position[axis] = pos;
    current_position[axis] = pos;
    line_to_current_position(feedrate);
}

bool marlin_server_enqueue_gcode(const char *gcode) {
    return queue.enqueue_one(gcode);
}

bool marlin_server_inject_gcode(const char *gcode) {
    queue.inject_P(gcode);
    return true;
}

void marlin_server_settings_save(void) {
#if HAS_BED_PROBE
    if (!SteelSheets::SetZOffset(probe.offset.z)) {
        assert(0 /* Z offset write failed */);
    }
#endif
#if ENABLED(PIDTEMPBED)
    eeprom_set_flt(EEVAR_PID_BED_P, Temperature::temp_bed.pid.Kp);
    eeprom_set_flt(EEVAR_PID_BED_I, Temperature::temp_bed.pid.Ki);
    eeprom_set_flt(EEVAR_PID_BED_D, Temperature::temp_bed.pid.Kd);
#endif
#if ENABLED(PIDTEMP)
    eeprom_set_flt(EEVAR_PID_NOZ_P, Temperature::temp_hotend[0].pid.Kp);
    eeprom_set_flt(EEVAR_PID_NOZ_I, Temperature::temp_hotend[0].pid.Ki);
    eeprom_set_flt(EEVAR_PID_NOZ_D, Temperature::temp_hotend[0].pid.Kd);
#endif
}

void marlin_server_settings_load(void) {
    (void)settings.reset();
#if HAS_BED_PROBE
    probe.offset.z = SteelSheets::GetZOffset();
#endif
#if ENABLED(PIDTEMPBED)
    Temperature::temp_bed.pid.Kp = eeprom_get_flt(EEVAR_PID_BED_P);
    Temperature::temp_bed.pid.Ki = eeprom_get_flt(EEVAR_PID_BED_I);
    Temperature::temp_bed.pid.Kd = eeprom_get_flt(EEVAR_PID_BED_D);
#endif
#if ENABLED(PIDTEMP)
    Temperature::temp_hotend[0].pid.Kp = eeprom_get_flt(EEVAR_PID_NOZ_P);
    Temperature::temp_hotend[0].pid.Ki = eeprom_get_flt(EEVAR_PID_NOZ_I);
    Temperature::temp_hotend[0].pid.Kd = eeprom_get_flt(EEVAR_PID_NOZ_D);
    thermalManager.updatePID();
#endif
    marlin_server.vars.fan_check_enabled = eeprom_get_bool(EEVAR_FAN_CHECK_ENABLED);
    marlin_server.vars.fs_autoload_enabled = eeprom_get_bool(EEVAR_FS_AUTOLOAD_ENABLED);

    job_id = variant8_get_ui16(eeprom_get_var(EEVAR_JOB_ID));
}

void marlin_server_settings_reset(void) {
    (void)settings.reset();
}

void marlin_server_manage_heater(void) {
    thermalManager.task();
}

void marlin_server_quick_stop(void) {
    planner.quick_stop();
}

uint32_t marlin_server_get_command(void) {
    return marlin_server.command;
}

void marlin_server_set_command(uint32_t command) {
    marlin_server.command = command;
}

void marlin_server_test_start(uint64_t mask) {
#if HAS_SELFTEST
    if (((marlin_server.print_state == mpsIdle) || (marlin_server.print_state == mpsFinished) || (marlin_server.print_state == mpsAborted)) && (!SelftestInstance().IsInProgress())) {
        SelftestInstance().Start(mask);
    }
#endif
}

void marlin_server_test_abort(void) {
#if HAS_SELFTEST
    if (SelftestInstance().IsInProgress()) {
        SelftestInstance().Abort();
    }
#endif
}

bool marlin_server_printer_idle() {
    return marlin_server.print_state == mpsIdle
        || marlin_server.print_state == mpsPaused
        || marlin_server.print_state == mpsAborted
        || marlin_server.print_state == mpsFinished;
}

bool marlin_server_printer_paused() {
    return marlin_server.print_state == mpsPaused;
}

void marlin_server_print_start(const char *filename, bool skip_preview) {
#if HAS_SELFTEST
    if (SelftestInstance().IsInProgress())
        return;
#endif
    if (filename == nullptr)
        return;

    // handle preview / reprint
    switch (marlin_server.print_state) {
    case mpsFinished:
    case mpsAborted:
        // correctly end previous print
        marlin_server_finalize_print();
        fsm_destroy(ClientFSM::Printing);
        break;
    case mpsPrintPreviewInit:
    case mpsPrintPreviewLoop:
        PrintPreview::Instance().ChangeState(IPrintPreview::State::inactive); // close preview
        break;
    default:
        break;
    }

    switch (marlin_server.print_state) {
    case mpsIdle:
    case mpsFinished:
    case mpsAborted:
    case mpsPrintPreviewInit:
    case mpsPrintPreviewLoop:
        media_print_start__prepare(filename);
        marlin_server.print_state = mpsWaitGui;
        _set_notify_change(MARLIN_VAR_FILEPATH);
        _set_notify_change(MARLIN_VAR_FILENAME);

        skip_preview ? PrintPreview::Instance().SkipIfAble() : PrintPreview::Instance().DontSkip();
        break;
    default:
        break;
    }
}

void marlin_server_gui_ready_to_print() {
    switch (marlin_server.print_state) {
    case mpsWaitGui:
        marlin_server.print_state = mpsPrintPreviewInit;
        break;
    default:
        log_error(MarlinServer, "Wrong print state, expected: %d, is: %d", mpsWaitGui, marlin_server.print_state);
        break;
    }
}

void marlin_server_print_abort(void) {
    switch (marlin_server.print_state) {
#if ENABLED(POWER_PANIC)
    case mpsPowerPanic_Resume:
    case mpsPowerPanic_AwaitingResume:
#endif
    case mpsPrinting:
    case mpsPaused:
    case mpsResuming_Reheating:
    case mpsFinishing_WaitIdle:
        marlin_server.print_state = mpsAborting_Begin;
        break;
    default:
        break;
    }
}

void marlin_server_print_exit(void) {
    switch (marlin_server.print_state) {
#if ENABLED(POWER_PANIC)
    case mpsPowerPanic_Resume:
    case mpsPowerPanic_AwaitingResume:
#endif
    case mpsPrinting:
    case mpsPaused:
    case mpsResuming_Reheating:
    case mpsFinishing_WaitIdle:
        // do nothing
        break;
    default:
        marlin_server.print_state = mpsExit;
        break;
    }
}

void marlin_server_print_pause(void) {
    if (marlin_server.print_state == mpsPrinting) {
        marlin_server.print_state = mpsPausing_Begin;
    }
}

/// Pauses reading from a file, stops watch, saves temperatures, disables fan
static void pause_print(Pause_Type type = Pause_Type::Pause, uint32_t resume_pos = UINT32_MAX) {
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

    print_job_timer.pause();
    marlin_server.resume.nozzle_temp = marlin_server.vars.target_nozzle; //save nozzle target temp
    marlin_server.resume.fan_speed = marlin_server.vars.print_fan_speed; //save fan speed
#if FAN_COUNT > 0
    if (hotendErrorChecker.runFullFan())
        thermalManager.set_fan_speed(0, 255);
    else
        thermalManager.set_fan_speed(0, 0); //disable print fan
#endif
}

void marlin_server_print_resume(void) {
    if (marlin_server.print_state == mpsPaused) {
        marlin_server.print_state = mpsResuming_Begin;
#if ENABLED(POWER_PANIC)
    } else if (marlin_server.print_state == mpsPowerPanic_AwaitingResume) {
        marlin_server.print_state = mpsPowerPanic_Resume;
#endif
    } else
        marlin_server_print_start(nullptr, true);
}

void marlin_server_print_reheat_start(void) {
    if ((marlin_server.print_state == mpsPaused) && marlin_server_print_reheat_ready()) {
        thermalManager.setTargetHotend(marlin_server.resume.nozzle_temp, 0);
        // No need to set bed temperature because we keep it on all the time.
    }
}

// Fast temperature recheck.
// Does not check stability of the temperature.
bool marlin_server_print_reheat_ready() {
    // check nozzle
    if (marlin_server.vars.target_nozzle != marlin_server.resume.nozzle_temp
        || marlin_server.vars.temp_nozzle < (marlin_server.vars.target_nozzle - TEMP_HYSTERESIS)) {
        return false;
    }
    // check bed
    if (marlin_server.vars.temp_bed < (marlin_server.vars.target_bed - TEMP_BED_HYSTERESIS))
        return false;

    return true;
}

#if ENABLED(POWER_PANIC)
void marlin_server_powerpanic_resume_loop(const char *media_SFN_path, uint32_t pos, bool start_paused) {
    // Open the file and immediately stop to set the print position
    marlin_server_print_start(media_SFN_path, true);
    media_print_quick_stop(pos);

    // enter the main powerpanic resume loop
    marlin_server.print_state = start_paused ? mpsPowerPanic_AwaitingResume : mpsPowerPanic_Resume;
    static metric_t power = METRIC("power_panic", METRIC_VALUE_EVENT, 0, METRIC_HANDLER_ENABLE_ALL);
    metric_record_event(&power);
}

void marlin_server_powerpanic_finish(bool paused) {
    // WARNING: this sequence needs to _just_ set the marlin_server state and exit
    // perform any higher-level operation inside power_panic::atomic_finish

    if (paused) {
        // restore leveling state and planner position (mind the order!)
        planner.leveling_active = crash_s.leveling_active;
        current_position = crash_s.start_current_position;
        planner.set_position_mm(current_position);
        marlin_server.print_state = mpsPaused;
    } else {
        // setup for replay and start recovery
        crash_s.set_state(Crash_s::RECOVERY);
        marlin_server.print_state = mpsResuming_UnparkHead_ZE;
    }
}
#endif

#if ENABLED(CRASH_RECOVERY)
enum class Axis_length_t {
    shorter,
    longer,
    ok,
};

static Axis_length_t axis_length_ok(AxisEnum axis) {
    const float len = marlin_server.axis_length.pos[axis];

    switch (axis) {
    case X_AXIS:
        return len < selftest::Config_XAxis.length_min ? Axis_length_t::shorter : (len > selftest::Config_XAxis.length_max ? Axis_length_t::longer : Axis_length_t::ok);
    case Y_AXIS:
        return len < selftest::Config_YAxis.length_min ? Axis_length_t::shorter : (len > selftest::Config_YAxis.length_max ? Axis_length_t::longer : Axis_length_t::ok);
    default:;
    }
    return Axis_length_t::shorter;
}

/// \returns true if X and Y axes have correct lengths.
/// You have to measure the length of the axes before this.
static Axis_length_t xy_axes_length_ok() {
    Axis_length_t alx = axis_length_ok(X_AXIS);
    Axis_length_t aly = axis_length_ok(Y_AXIS);
    if (alx == aly && aly == Axis_length_t::ok)
        return Axis_length_t::ok;
    // shorter is worse than longer
    if (alx == Axis_length_t::shorter || aly == Axis_length_t::shorter)
        return Axis_length_t::shorter;
    return Axis_length_t::longer;
}

static SelftestSubtestState_t axis_length_check(AxisEnum axis) {
    return axis_length_ok(axis) == Axis_length_t::ok ? SelftestSubtestState_t::ok : SelftestSubtestState_t::not_good;
}

/// Sets lengths of axes to "by-pass" xy_axes_length_ok()
static void axes_length_set_ok() {
    const int axis_len[2] = { X_MAX_POS - X_MIN_POS, Y_MAX_POS - Y_MIN_POS };
    LOOP_XY(axis) {
        const int gap = axis == X_AXIS ? X_END_GAP : Y_END_GAP;
        marlin_server.axis_length.pos[axis] = axis_len[axis] + gap;
    }
}
#endif // ENABLED(CRASH_RECOVERY)

void marlin_server_nozzle_timeout_on() {
    marlin_server.enable_nozzle_temp_timeout = true;
};
void marlin_server_nozzle_timeout_off() {
    marlin_server.enable_nozzle_temp_timeout = false;
}
void marlin_server_nozzle_timeout_loop() {
    if ((marlin_server.vars.target_nozzle > 0) && (ticks_ms() - marlin_server.paused_ticks > (1000 * PAUSE_NOZZLE_TIMEOUT)) && marlin_server.enable_nozzle_temp_timeout) {
        thermalManager.setTargetHotend(0, 0);
        marlin_server_set_temp_to_display(0);
    }
}

static void marlin_server_resuming_reheating() {
    if (hotendErrorChecker.isFailed()) {
        set_warning(WarningType::HotendTempDiscrepancy);
        thermalManager.setTargetHotend(0, 0);
#if FAN_COUNT > 0
        thermalManager.set_fan_speed(0, 255);
#endif
        marlin_server.print_state = mpsPaused;
    }

    if (!marlin_server_print_reheat_ready())
        return;

#ifdef NEW_FANCTL
    if (marlin_server.vars.fan_check_enabled) {
        if (!fanCtlHeatBreak.getRPMIsOk()) {
            set_warning(WarningType::HotendFanError);
            thermalManager.setTargetHotend(0, 0);
            marlin_server.print_state = mpsPaused;
            return;
        }
    }
#endif

#if HAS_BED_PROBE
    // There's homing after MBL fail so no need to unpark at all
    if (marlin_server.mbl_failed) {
        marlin_server.print_state = mpsResuming_UnparkHead_ZE;
        return;
    }
#endif
    marlin_server_unpark_head_XY();
    marlin_server.print_state = mpsResuming_UnparkHead_XY;
}

static void _server_print_loop(void) {
    static bool did_not_start_print = true;
    switch (marlin_server.print_state) {
    case mpsIdle:
        break;
    case mpsWaitGui:
        // without gui just act as if state == mpsPrintPreviewInit
#if HAS_GUI()
        break;
#endif
    case mpsPrintPreviewInit:
        did_not_start_print = true;
        if (media_print_filepath()) {
            PrintPreview::Instance().Init(media_print_filepath());
        }
        marlin_server.print_state = mpsPrintPreviewLoop;
        break;
        /*
        TODO thia used to be in original implamentation, but we dont do that anymore
        bool ScreenPrintPreview::gcode_file_exists() {
            return access(gcode.GetGcodeFilepath(), F_OK) == 0;
        }

        ...

        if (!gcode_file_exists()) {
            Screens::Access()->Close(); //if an dialog is opened, it will be closed first
        */
    case mpsPrintPreviewLoop: // button evaluation
        switch (PrintPreview::Instance().Loop()) {
        case PrintPreview::Result::InProgress:
            break;
        case PrintPreview::Result::Abort:
            marlin_server.print_state = did_not_start_print ? mpsIdle : mpsFinishing_WaitIdle;
            break;
        case PrintPreview::Result::Print:
        case PrintPreview::Result::Inactive:
            did_not_start_print = false;
            marlin_server.print_state = mpsPrintInit;
            break;
        }
        break;
    case mpsPrintInit:
        feedrate_percentage = 100;
        // First, reserve the job_id in eeprom. In case we get reset, we need
        // that to not get reused by accident.
        eeprom_set_var(EEVAR_JOB_ID, variant8_ui16(job_id + 1));
        // And increment the job ID before we actually stop printing.
        job_id++;
        _set_notify_change(MARLIN_VAR_JOB_ID);
#if ENABLED(CRASH_RECOVERY)
        crash_s.reset();
        endstops.enable_globally(true);
        crash_s.set_state(Crash_s::PRINTING);
#endif // ENABLED(CRASH_RECOVERY)
        media_print_start();

        print_job_timer.start();
        marlin_server.print_state = mpsPrinting;
        fsm_create(ClientFSM::Printing);
#if HAS_BED_PROBE
        marlin_server.mbl_failed = false;
#endif
        break;
    case mpsPrinting:
        switch (media_print_get_state()) {
        case media_print_state_PRINTING:
            break;
        case media_print_state_PAUSED:
            /// TODO don't pause in pause/abort/crash etx.
            marlin_server.print_state = mpsPausing_Begin;
            break;
        case media_print_state_NONE:
            marlin_server.print_state = mpsFinishing_WaitIdle;
            break;
        case media_print_state_DRAINING:
            media_reset_USB_host();
            break;
        }
        break;
    case mpsPausing_Begin:
        pause_print();
        // no break
    case mpsPausing_Failed_Code:
        marlin_server.print_state = mpsPausing_WaitIdle;
        break;
    case mpsPausing_WaitIdle:
        if ((planner.movesplanned() == 0) && !queue.has_commands_queued() && gcode.busy_state == GcodeSuite::NOT_BUSY) {
            marlin_server_park_head();
            marlin_server.print_state = mpsPausing_ParkHead;
        }
        break;
    case mpsPausing_ParkHead:
        if (planner.movesplanned() == 0) {
            marlin_server.paused_ticks = ticks_ms(); //time when printing paused
            marlin_server.print_state = mpsPaused;
        }
        break;
    case mpsPaused:
        marlin_server_nozzle_timeout_loop();
        gcode.reset_stepper_timeout(); //prevent disable axis
        break;
    case mpsResuming_Begin:
#if ENABLED(CRASH_RECOVERY)
        if (crash_s.is_repeated_crash() && xy_axes_length_ok() != Axis_length_t::ok) {
            /// resuming after a crash but axes are not ok => check again
            fsm_create(ClientFSM::CrashRecovery);
            marlin_server.print_state = mpsCrashRecovery_Lifting;
            break;
        }

        // forget the XYZ resume position if requested
        if (crash_s.inhibit_flags & Crash_s::INHIBIT_XYZ_REPOSITIONING) {
            LOOP_LOGICAL_AXES(i) {
                marlin_server.resume.pos[i] = current_position[i];
            }
        }
#endif
        marlin_server_resuming_begin();
        break;
    case mpsResuming_Reheating:
        marlin_server_resuming_reheating();
        break;
    case mpsResuming_UnparkHead_XY:
        if (planner.movesplanned() != 0)
            break;
        marlin_server_unpark_head_ZE();
        marlin_server.print_state = mpsResuming_UnparkHead_ZE;
        break;
    case mpsResuming_UnparkHead_ZE:
        if ((planner.movesplanned() != 0) || queue.has_commands_queued() || (media_print_get_state() != media_print_state_PAUSED))
            break;
#if ENABLED(CRASH_RECOVERY)
        if (crash_s.get_state() == Crash_s::RECOVERY) {
            endstops.enable_globally(true);
            crash_s.set_state(Crash_s::REPLAY);
        } else {
            // UnparkHead can be called after a pause, in which case crash handling should already
            // be active and we don't need to change any other setting
            assert(crash_s.get_state() == Crash_s::PRINTING);
        }
#endif
#if HAS_BED_PROBE
        if (marlin_server.mbl_failed) {
            gcode.process_subcommands_now(F("G28"));
            marlin_server.mbl_failed = false;
        }
#endif
        //marlin_server.motion_param.load();  // TODO: currently disabled (see Crash_s::save_parameters())
        media_print_resume();
        if (print_job_timer.isPaused())
            print_job_timer.start();
#if FAN_COUNT > 0
        thermalManager.set_fan_speed(0, marlin_server.resume.fan_speed); // restore fan speed
#endif
        marlin_server.print_state = mpsPrinting;
        break;
    case mpsAborting_Begin:
        media_print_stop();
        queue.clear();
        thermalManager.disable_all_heaters();
#if FAN_COUNT > 0
        thermalManager.set_fan_speed(0, 0);
#endif
        marlin_server_set_temp_to_display(0);
        print_job_timer.stop();
        planner.quick_stop();
        wait_for_heatup = false; // This is necessary because M109/wait_for_hotend can be in progress, we need to abort it

#if ENABLED(CRASH_RECOVERY)
        // TODO: the following should be moved to mpsAborting_ParkHead once the "stopping"
        // state is handled properly
        endstops.enable_globally(false);
        crash_s.reset();
#endif // ENABLED(CRASH_RECOVERY)

        marlin_server.print_state = mpsAborting_WaitIdle;
        break;
    case mpsAborting_WaitIdle:
        if ((planner.movesplanned() != 0) || queue.has_commands_queued())
            break;

        // allow movements again
        planner.resume_queuing();
        set_current_from_steppers();
        sync_plan_position();
        report_current_position();

        if (crash_s.did_trigger()) {
            marlin_server_lift_head();
        } else {
            marlin_server_park_head();
        }

        marlin_server.print_state = mpsAborting_ParkHead;
        break;
    case mpsAborting_ParkHead:
        if ((planner.movesplanned() == 0) && !queue.has_commands_queued()) {
            stepper.disable_axis(X_AXIS);
            stepper.disable_axis(Y_AXIS);
#ifndef Z_ALWAYS_ON
            stepper.disable_axis(Z_AXIS);
#endif // Z_ALWAYS_ON
            stepper.disable_e_steppers();
            marlin_server.print_state = mpsAborted;
            marlin_server_finalize_print();
        }
        break;
    case mpsFinishing_WaitIdle:
        if ((planner.movesplanned() == 0) && !queue.has_commands_queued()) {
#if ENABLED(CRASH_RECOVERY)
            // TODO: the following should be moved to mpsFinishing_ParkHead once the "stopping"
            // state is handled properly
            endstops.enable_globally(false);
            crash_s.reset();
#endif // ENABLED(CRASH_RECOVERY)

#ifdef PARK_HEAD_ON_PRINT_FINISH
            marlin_server_park_head();
#endif // PARK_HEAD_ON_PRINT_FINISH
            if (print_job_timer.isRunning())
                print_job_timer.stop();
            marlin_server.print_state = mpsFinishing_ParkHead;
        }
        break;
    case mpsFinishing_ParkHead:
        if ((planner.movesplanned() == 0) && !queue.has_commands_queued()) {
            marlin_server.print_state = mpsFinished;
            marlin_server_finalize_print();
        }
        break;
    case mpsExit:
        marlin_server_finalize_print();
        fsm_destroy(ClientFSM::Printing);
        marlin_server.print_state = mpsIdle;
        break;

#if ENABLED(CRASH_RECOVERY)
    case mpsCrashRecovery_Begin: {
        // pause and set correct resume position: this will stop media reading and clear the queue
        // TODO: this is completely broken for crashes coming from serial printing
        pause_print(Pause_Type::Crash, crash_s.sdpos);

        endstops.enable_globally(false);
        crash_s.send_reports();
        crash_s.count_crash();
        crash_s.set_state(Crash_s::RECOVERY);

        /// TODO: create FSM with different state
        fsm_create(ClientFSM::CrashRecovery);
        Crash_recovery_fsm cr_fsm(SelftestSubtestState_t::running, SelftestSubtestState_t::undef);
        if (crash_s.is_repeated_crash()) {
            fsm_change(ClientFSM::CrashRecovery, PhasesCrashRecovery::check_X, cr_fsm.Serialize()); // check axes first
        } else {
            fsm_change(ClientFSM::CrashRecovery, PhasesCrashRecovery::home, cr_fsm.Serialize());
        }

        // save the current resume position
        marlin_server.resume.pos = current_position;

    #if ENABLED(ADVANCED_PAUSE_FEATURE)
        /// retract and save E stepper position
        marlin_server_retract();
    #endif // ENABLED(ADVANCED_PAUSE_FEATURE)

        marlin_server.print_state = mpsCrashRecovery_Retracting;
        break;
    }
    case mpsCrashRecovery_Retracting: {
        if (planner.movesplanned() != 0)
            break;

        marlin_server_lift_head();
        marlin_server.print_state = mpsCrashRecovery_Lifting;
        break;
    }
    case mpsCrashRecovery_Lifting: {
        if (planner.movesplanned() != 0)
            break;

        if (crash_s.is_repeated_crash())
            marlin_server_enqueue_gcode("G163 X Y S" STRINGIFY(AXIS_MEASURE_STALL_GUARD) " P" STRINGIFY(AXIS_MEASURE_CRASH_PERIOD));
        marlin_server.print_state = mpsCrashRecovery_XY_Measure;
        break;
    }
    case mpsCrashRecovery_XY_Measure: {
        if (queue.has_commands_queued() || planner.movesplanned() != 0)
            break;

        static metric_t crash_len = METRIC("crash_length", METRIC_VALUE_CUSTOM, 0, METRIC_HANDLER_ENABLE_ALL);
        metric_record_custom(&crash_len, " x=%.3f,y=%.3f", (double)marlin_server.axis_length[X_AXIS], (double)marlin_server.axis_length[Y_AXIS]);

        marlin_server_enqueue_gcode("G28 X Y R0 D");
        marlin_server.print_state = mpsCrashRecovery_XY_HOME;
        break;
    }
    case mpsCrashRecovery_XY_HOME: {
        if (queue.has_commands_queued() || planner.movesplanned() != 0)
            break;

        if (!crash_s.is_repeated_crash()) {
            marlin_server.print_state = mpsResuming_Begin;
            fsm_destroy(ClientFSM::CrashRecovery);
            break;
        }
        marlin_server.paused_ticks = ticks_ms(); //time when printing paused
        Axis_length_t alok = xy_axes_length_ok();
        if (alok != Axis_length_t::ok) {
            marlin_server.print_state = mpsCrashRecovery_Axis_NOK;
            Crash_recovery_fsm cr_fsm(axis_length_check(X_AXIS), axis_length_check(Y_AXIS));
            PhasesCrashRecovery pcr = (alok == Axis_length_t::shorter) ? PhasesCrashRecovery::axis_short : PhasesCrashRecovery::axis_long;
            fsm_change(ClientFSM::CrashRecovery, pcr, cr_fsm.Serialize());
            break;
        }
        Crash_recovery_fsm cr_fsm(SelftestSubtestState_t::undef, SelftestSubtestState_t::undef);
        fsm_change(ClientFSM::CrashRecovery, PhasesCrashRecovery::repeated_crash, cr_fsm.Serialize());
        marlin_server.print_state = mpsCrashRecovery_Repeated_Crash;
        break;
    }
    case mpsCrashRecovery_Axis_NOK: {
        marlin_server_nozzle_timeout_loop();
        switch (ClientResponseHandler::GetResponseFromPhase(PhasesCrashRecovery::axis_NOK)) {
        case Response::Retry:
            marlin_server.print_state = mpsCrashRecovery_Lifting;
            break;
        case Response::Resume: /// ignore wrong length of axes
            marlin_server.print_state = mpsResuming_Begin;
            fsm_destroy(ClientFSM::CrashRecovery);
            axes_length_set_ok(); /// ignore re-test of lengths
            break;
        case Response::_none:
            break;
        default:
            marlin_server.print_state = mpsPaused;
            fsm_destroy(ClientFSM::CrashRecovery);
        }
        gcode.reset_stepper_timeout(); //prevent disable axis
        break;
    }
    case mpsCrashRecovery_Repeated_Crash: {
        marlin_server_nozzle_timeout_loop();
        switch (ClientResponseHandler::GetResponseFromPhase(PhasesCrashRecovery::repeated_crash)) {
        case Response::Resume:
            marlin_server.print_state = mpsResuming_Begin;
            fsm_destroy(ClientFSM::CrashRecovery);
            break;
        case Response::_none:
            break;
        default:
            marlin_server.print_state = mpsPaused;
            fsm_destroy(ClientFSM::CrashRecovery);
        }
        gcode.reset_stepper_timeout(); //prevent disable axis
        break;
    }
#endif // ENABLED(CRASH_RECOVERY)
#if ENABLED(POWER_PANIC)
    case mpsPowerPanic_acFault:
        power_panic::ac_fault_loop();
        break;
    case mpsPowerPanic_Resume:
        power_panic::resume_loop();
        break;
#endif // ENABLED(POWER_PANIC)
    default:
        break;
    }

#ifdef NEW_FANCTL
    if (marlin_server.vars.fan_check_enabled) {
        hotendFanErrorChecker.checkTrue(fanCtlHeatBreak.getState() != CFanCtl::error_running);
        printFanErrorChecker.checkTrue(fanCtlPrint.getState() != CFanCtl::error_running);
    }
    if (fanCtlHeatBreak.getRPMIsOk())
        hotendFanErrorChecker.reset();
    if (fanCtlPrint.getRPMIsOk())
        printFanErrorChecker.reset();
#endif //NEW_FANCTL

    hotendErrorChecker.checkTrue(Temperature::saneTempReadingHotend(0));
}

void marlin_server_resuming_begin(void) {
    marlin_server_nozzle_timeout_on(); // could be turned off after pause by changing temperature.
    if (marlin_server_print_reheat_ready()) {
        marlin_server_unpark_head_XY();
        marlin_server.print_state = mpsResuming_UnparkHead_XY;
    } else {
        thermalManager.setTargetHotend(marlin_server.resume.nozzle_temp, 0);
        marlin_server_set_temp_to_display(marlin_server.resume.nozzle_temp);
#if FAN_COUNT > 0
        thermalManager.set_fan_speed(0, 0); //disable print fan
#endif
        marlin_server.print_state = mpsResuming_Reheating;
    }
}

void marlin_server_retract() {
    //marlin_server.motion_param.save_reset();  // TODO: currently disabled (see Crash_s::save_parameters())
#if ENABLED(ADVANCED_PAUSE_FEATURE)
    float mm = PAUSE_PARK_RETRACT_LENGTH / planner.e_factor[active_extruder];
    #if BOTH(CRASH_RECOVERY, LIN_ADVANCE)
    if (crash_s.did_trigger())
        mm += crash_s.advance_mm;
    #endif
    plan_move_by(PAUSE_PARK_RETRACT_FEEDRATE, 0, 0, 0, -mm);
#endif // ENABLED(ADVANCED_PAUSE_FEATURE)
}

void marlin_server_lift_head() {
#if ENABLED(NOZZLE_PARK_FEATURE)
    const constexpr xyz_pos_t park = NOZZLE_PARK_POINT;
    plan_move_by(NOZZLE_PARK_Z_FEEDRATE, 0, 0, _MIN(park.z, Z_MAX_POS - current_position.z));
#endif // ENABLED(NOZZLE_PARK_FEATURE)
}

void marlin_server_park_head() {
#if ENABLED(NOZZLE_PARK_FEATURE)
    if (!all_axes_homed())
        return;

    marlin_server.resume.pos = current_position;
    marlin_server_retract();
    marlin_server_lift_head();
    xyz_pos_t park = NOZZLE_PARK_POINT;
    #ifdef NOZZLE_PARK_POINT_M600
    const xyz_pos_t park_clean = NOZZLE_PARK_POINT_M600;
    if (marlin_server.mbl_failed)
        park = park_clean;
    #endif // NOZZLE_PARK_POINT_M600
    park.z = current_position.z;
    plan_park_move_to_xyz(park, NOZZLE_PARK_XY_FEEDRATE, NOZZLE_PARK_Z_FEEDRATE);
#endif //NOZZLE_PARK_FEATURE
}

void marlin_server_unpark_head_XY(void) {
#if ENABLED(NOZZLE_PARK_FEATURE)
    // TODO: double check this condition: when recovering from a crash, Z is not known, but we *can*
    // unpark, so we bypass this check as we need to move back
    if (!crash_s.did_trigger() && !all_axes_homed())
        return;

    current_position.x = marlin_server.resume.pos.x;
    current_position.y = marlin_server.resume.pos.y;
    line_to_current_position(NOZZLE_PARK_XY_FEEDRATE);
#endif //NOZZLE_PARK_FEATURE
}

void marlin_server_unpark_head_ZE(void) {
#if ENABLED(NOZZLE_PARK_FEATURE)
    // TODO: see comment above on unparking: if axes are not known, lift is skipped, but not this
    if (!all_axes_homed())
        return;

    // Move Z
    current_position.z = marlin_server.resume.pos.z;
    destination = current_position;
    prepare_internal_move_to_destination(NOZZLE_PARK_Z_FEEDRATE);

    #if ENABLED(ADVANCED_PAUSE_FEATURE)
    // Undo E retract
    plan_move_by(PAUSE_PARK_RETRACT_FEEDRATE, 0, 0, 0, marlin_server.resume.pos.e - current_position.e);
    #endif // ENABLED(ADVANCED_PAUSE_FEATURE)
#endif     //NOZZLE_PARK_FEATURE
}

int marlin_all_axes_homed(void) {
    return all_axes_homed() ? 1 : 0;
}

int marlin_all_axes_known(void) {
    return all_axes_trusted() ? 1 : 0;
}

int marlin_server_get_exclusive_mode(void) {
    return (marlin_server.flags & MARLIN_SFLG_EXCMODE) ? 1 : 0;
}

void marlin_server_set_exclusive_mode(int exclusive) {
    if (exclusive) {
        SerialUSB.setIsWriteOnly(true);
        marlin_server.flags |= MARLIN_SFLG_EXCMODE; // enter exclusive mode
    } else {
        marlin_server.flags &= ~MARLIN_SFLG_EXCMODE; // exit exclusive mode
        SerialUSB.setIsWriteOnly(false);
    }
}

void marlin_server_set_temp_to_display(float value) {
    marlin_server.vars.display_nozzle = value;
    _set_notify_change(MARLIN_VAR_DTEM_NOZ); //set change flag
}
float marlin_server_get_temp_to_display(void) {
    return marlin_server.vars.display_nozzle;
}

float marlin_server_get_temp_nozzle(void) {
    return marlin_server.vars.temp_nozzle;
}

resume_state_t *marlin_server_get_resume_data() {
    return &marlin_server.resume;
}

void marlin_server_set_resume_data(const resume_state_t *data) {
    // ensure this is called only from the marlin thread
    assert(osThreadGetId() == marlin_server_task);
    marlin_server.resume = *data;
}

extern uint32_t marlin_server_get_user_click_count(void) {
    return marlin_server.knob_click_counter;
}

extern uint32_t marlin_server_get_user_move_count(void) {
    return marlin_server.knob_move_counter;
}

//-----------------------------------------------------------------------------
// private functions

// send notify message (variant8_t) to client queue (called from server thread)
static int _send_notify_to_client(osMessageQId queue, variant8_t msg) {
    //synchronization not necessary because only server thread can write to this queue
    if (queue == 0)
        return 0;
    if (osMessageAvailableSpace(queue) < 2)
        return 0;
    osMessagePut(queue, (uint32_t)(msg & 0xFFFFFFFFU), osWaitForever);
    osMessagePut(queue, (uint32_t)(msg >> 32), osWaitForever);
    return 1;
}

// send all FSM messages from the FSM queue
static bool _send_FSM_event_to_client(int client_id, osMessageQId queue) {
    while (1) {
        fsm::variant_t variant = fsm_event_queues[client_id].Front();
        if (variant.GetCommand() == ClientFSM_Command::none)
            return true; // no event to send, return 'sent' to erase 'send' flag

        if (!_send_notify_to_client(queue, variant8_user(variant.u32, variant.u16, MARLIN_EVT_FSM)))
            // unable to send all messages
            return false;

        //erase sent item from queue
        fsm_event_queues[client_id].Pop();
    }
}

// send event notification to client (called from server thread)
static bool _send_notify_event_to_client(int client_id, osMessageQId queue, MARLIN_EVT_t evt_id, uint32_t usr32, uint16_t usr16) {
    variant8_t msg;
    switch (evt_id) {
    case MARLIN_EVT_Message:
        msg = marlin_server.event_messages[client_id];
        break;
    case MARLIN_EVT_FSM:
        return _send_FSM_event_to_client(client_id, queue);
    default:
        msg = variant8_user(usr32, usr16, evt_id);
    }

    bool ret = _send_notify_to_client(queue, msg);

    if (ret) {
        switch (evt_id) {
        case MARLIN_EVT_Message:
            // clear sent client message
            marlin_server.event_messages[client_id] = variant8_empty();
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
    if (evt_msk == 0)
        return 0;
    uint64_t sent = 0;
    uint64_t msk = 1;
    for (uint8_t evt_int = 0; evt_int <= MARLIN_EVT_MAX; evt_int++) {
        MARLIN_EVT_t evt_id = (MARLIN_EVT_t)evt_int;
        if (msk & evt_msk) {
            switch ((MARLIN_EVT_t)evt_id) {
                // Events without arguments
                // TODO: send all these in a single message as a bitfield
            case MARLIN_EVT_Startup:
            case MARLIN_EVT_MediaInserted:
            case MARLIN_EVT_MediaError:
            case MARLIN_EVT_MediaRemoved:
            case MARLIN_EVT_PrintTimerStarted:
            case MARLIN_EVT_PrintTimerPaused:
            case MARLIN_EVT_PrintTimerStopped:
            case MARLIN_EVT_FilamentRunout:
            case MARLIN_EVT_FactoryReset:
            case MARLIN_EVT_LoadSettings:
            case MARLIN_EVT_StoreSettings:
            case MARLIN_EVT_StartProcessing:
            case MARLIN_EVT_StopProcessing:
            case MARLIN_EVT_FSM: //arguments handled elsewhere
            // StatusChanged event - one string argument
            case MARLIN_EVT_StatusChanged:
                if (_send_notify_event_to_client(client_id, queue, evt_id, 0, 0))
                    sent |= msk; // event sent, set bit
                break;
            // CommandBegin/End - one ui32 argument (CMD)
            case MARLIN_EVT_CommandBegin:
                if (_send_notify_event_to_client(client_id, queue, evt_id, marlin_server.command_begin, 0))
                    sent |= msk; // event sent, set bit
                break;
            case MARLIN_EVT_CommandEnd:
                if (_send_notify_event_to_client(client_id, queue, evt_id, marlin_server.command_end, 0))
                    sent |= msk; // event sent, set bit
                break;
            case MARLIN_EVT_MeshUpdate:
                if (_send_notify_event_to_client(client_id, queue, evt_id,
                        marlin_server.last_mesh_evt.usr32, marlin_server.last_mesh_evt.usr16))
                    sent |= msk; // event sent, set bit
                break;
            case MARLIN_EVT_Acknowledge:
                if (_send_notify_event_to_client(client_id, queue, evt_id, 0, 0))
                    sent |= msk; // event sent, set bit
                break;
            //unused events
            case MARLIN_EVT_PrinterKilled:
            case MARLIN_EVT_Error:
            case MARLIN_EVT_PlayTone:
            case MARLIN_EVT_UserConfirmRequired:
            case MARLIN_EVT_SafetyTimerExpired:
            case MARLIN_EVT_Message:
            case MARLIN_EVT_Reheat:
                sent |= msk; // fake event sent for unused and forced events
                break;
            case MARLIN_EVT_Warning:
                if (_send_notify_event_to_client(client_id, queue, evt_id, marlin_server.warning_type, 0))
                    sent |= msk; // event sent, set bit
                break;
            }
            if ((sent & msk) == 0)
                break; //skip sending if queue is full
        }
        msk <<= 1;
    }
    return sent;
}

// send event notification to all clients (called from server thread)
// returns bitmask - bit0 = notify for client0 successfully send, bit1 for client1...
static uint8_t _send_notify_event(MARLIN_EVT_t evt_id, uint32_t usr32, uint16_t usr16) {
    uint8_t client_msk = 0;
    for (int client_id = 0; client_id < MARLIN_MAX_CLIENTS; client_id++)
        if (marlin_server.notify_events[client_id] & ((uint64_t)1 << evt_id)) {
            if (_send_notify_event_to_client(client_id, marlin_client_queue[client_id], evt_id, usr32, usr16) == 0) {
                marlin_server.client_events[client_id] |= ((uint64_t)1 << evt_id); // event not sent, set bit
                // save unsent data of the event for later retransmission
                if (evt_id == MARLIN_EVT_MeshUpdate) {
                    marlin_server.last_mesh_evt.usr32 = usr32;
                    marlin_server.last_mesh_evt.usr16 = usr16;
                } else if (evt_id == MARLIN_EVT_Warning)
                    marlin_server.warning_type = usr32;
            } else {
                // event sent, clear flag
                client_msk |= (1 << client_id);
            }
        }
    return client_msk;
}

// send variable change notification to client (called from server thread)
static int _send_notify_change_to_client(osMessageQId queue, uint8_t var_id, variant8_t var) {
    variant8_set_usr8(&var, var_id | MARLIN_USR8_VAR_FLG);
    return _send_notify_to_client(queue, var);
}

// send variable change notification to client - multiple vars (called from server thread)
static uint64_t _send_notify_changes_to_client(int client_id, osMessageQId queue, uint64_t var_msk) {
    variant8_t var;
    uint64_t sent = 0;
    uint64_t msk = 1;
    for (int var_id = 0; var_id <= MARLIN_VAR_MAX; var_id++) {
        if (msk & var_msk) {
            var = marlin_vars_get_var(&(marlin_server.vars), (marlin_var_id_t)var_id);
            // if the variable is readable then send else try next time
            if (variant8_get_type(var) != VARIANT8_EMPTY) {
                if (_send_notify_change_to_client(queue, var_id, var))
                    sent |= msk;
                else
                    break; //skip sending if queue is full
            }
        }
        msk <<= 1;
    }
    return sent;
}

static void _set_notify_change(uint8_t var_id) {
    uint64_t msk = MARLIN_VAR_MSK(var_id);
    for (int id = 0; id < MARLIN_MAX_CLIENTS; id++)
        marlin_server.client_changes[id] |= msk;
}

static void _server_update_gqueue(void) {
    if (marlin_server.gqueue != queue.ring_buffer.length) {
        marlin_server.gqueue = queue.ring_buffer.length;
        //		_dbg("gqueue: %2d", marlin_server.gqueue);
    }
}

static void _server_update_pqueue(void) {
    if ((marlin_server.pqueue_head != planner.block_buffer_head) || (marlin_server.pqueue_tail != planner.block_buffer_tail)) {
        marlin_server.pqueue_head = planner.block_buffer_head;
        marlin_server.pqueue_tail = planner.block_buffer_tail;
        marlin_server.pqueue = (marlin_server.pqueue_head >= marlin_server.pqueue_tail) ? (marlin_server.pqueue_head - marlin_server.pqueue_tail) : (BLOCK_BUFFER_SIZE + marlin_server.pqueue_head - marlin_server.pqueue_tail);
        //		_dbg("pqueue: %2d", marlin_server.pqueue);
    }
}

// update server variables defined by 'update', returns changed variables mask (called from server thread)
static uint64_t _server_update_vars(uint64_t update) {
    int i;
    uint64_t changes = 0;

    if (update == 0)
        return 0;

    if (update & MARLIN_VAR_MSK(MARLIN_VAR_GQUEUE))
        if (marlin_server.vars.gqueue != marlin_server.gqueue) {
            marlin_server.vars.gqueue = marlin_server.gqueue;
            changes |= MARLIN_VAR_MSK(MARLIN_VAR_GQUEUE);
        }

    if (update & MARLIN_VAR_MSK(MARLIN_VAR_PQUEUE))
        if (marlin_server.vars.pqueue != marlin_server.pqueue) {
            marlin_server.vars.pqueue = marlin_server.pqueue;
            changes |= MARLIN_VAR_MSK(MARLIN_VAR_PQUEUE);
        }

    if (update & MARLIN_VAR_MSK(MARLIN_VAR_MOTION)) {
        uint8_t flags = 0;
        if (stepper.axis_is_moving(X_AXIS))
            flags |= 0x01;
        if (stepper.axis_is_moving(Y_AXIS))
            flags |= 0x02;
        if (stepper.axis_is_moving(Z_AXIS))
            flags |= 0x04;
        if (stepper.axis_is_moving(E_AXIS))
            flags |= 0x08;
        if (marlin_server.vars.motion != flags) {
            marlin_server.vars.motion = flags;
            changes |= MARLIN_VAR_MSK(MARLIN_VAR_MOTION);
        }
    }

    if (update & MARLIN_VAR_MSK_IPOS_XYZE) {
        for (i = 0; i < 4; i++)
            if (update & MARLIN_VAR_MSK(MARLIN_VAR_IPOS_X + i)) {
                int32_t pos = stepper.position((AxisEnum)i);
                if (marlin_server.vars.ipos[i] != pos) {
                    marlin_server.vars.ipos[i] = pos;
                    changes |= MARLIN_VAR_MSK(MARLIN_VAR_IPOS_X + i);
                }
            }
    }

    if (update & MARLIN_VAR_MSK_POS_XYZE) {
        for (i = 0; i < 4; i++)
            if (update & MARLIN_VAR_MSK(MARLIN_VAR_POS_X + i)) {
                float pos_mm;
                if (i < 3) {
                    pos_mm = planner.get_axis_position_mm((AxisEnum)i) + workspace_offset.pos[i];
                } else {
                    pos_mm = planner.get_axis_position_mm((AxisEnum)i);
                }
                if (marlin_server.vars.pos[i] != pos_mm) {
                    marlin_server.vars.pos[i] = pos_mm;
                    changes |= MARLIN_VAR_MSK(MARLIN_VAR_POS_X + i);
                }
            }
    }
    if (update & MARLIN_VAR_MSK_CURR_POS_XYZE) {
        for (i = 0; i < 4; i++)
            if (update & MARLIN_VAR_MSK(MARLIN_VAR_CURR_POS_X + i)) {
                float pos_mm = current_position[i];
                if (marlin_server.vars.curr_pos[i] != pos_mm) {
                    marlin_server.vars.curr_pos[i] = pos_mm;
                    changes |= MARLIN_VAR_MSK(MARLIN_VAR_CURR_POS_X + i);
                }
            }
    }

    if (update & MARLIN_VAR_MSK(MARLIN_VAR_TEMP_NOZ)) {
        float temp = thermalManager.temp_hotend[0].celsius;
        if (marlin_server.vars.temp_nozzle != temp) {
            marlin_server.vars.temp_nozzle = temp;
            changes |= MARLIN_VAR_MSK(MARLIN_VAR_TEMP_NOZ);
        }
    }

    if (update & MARLIN_VAR_MSK(MARLIN_VAR_TTEM_NOZ)) {
        float temp = thermalManager.temp_hotend[0].target;
        if (marlin_server.vars.target_nozzle != temp) {
            marlin_server.vars.target_nozzle = temp;
            changes |= MARLIN_VAR_MSK(MARLIN_VAR_TTEM_NOZ);
        }
    }

    if (update & MARLIN_VAR_MSK(MARLIN_VAR_TEMP_BED)) {
        float temp = thermalManager.temp_bed.celsius;
        if (marlin_server.vars.temp_bed != temp) {
            marlin_server.vars.temp_bed = temp;
            changes |= MARLIN_VAR_MSK(MARLIN_VAR_TEMP_BED);
        }
    }

    if (update & MARLIN_VAR_MSK(MARLIN_VAR_TTEM_BED)) {
        float temp = thermalManager.temp_bed.target;
        if (marlin_server.vars.target_bed != temp) {
            marlin_server.vars.target_bed = temp;
            changes |= MARLIN_VAR_MSK(MARLIN_VAR_TTEM_BED);
        }
    }

    if (update & MARLIN_VAR_MSK(MARLIN_VAR_Z_OFFSET)) {
        float z = probe.offset.z;
        if (marlin_server.vars.z_offset != z) {
            marlin_server.vars.z_offset = z;
            changes |= MARLIN_VAR_MSK(MARLIN_VAR_Z_OFFSET);
        }
    }

    if (update & MARLIN_VAR_MSK(MARLIN_VAR_FANSPEED)) {
        uint8_t speed = 0;
#if FAN_COUNT > 0
        speed = thermalManager.fan_speed[0];
#endif
        if (marlin_server.vars.print_fan_speed != speed) {
            marlin_server.vars.print_fan_speed = speed;
            changes |= MARLIN_VAR_MSK(MARLIN_VAR_FANSPEED);
        }
    }

    if (update & MARLIN_VAR_MSK(MARLIN_VAR_PRNSPEED)) {
        uint16_t feed = static_cast<uint16_t>(feedrate_percentage);
        if (marlin_server.vars.print_speed != feed) {
            marlin_server.vars.print_speed = feed;
            changes |= MARLIN_VAR_MSK(MARLIN_VAR_PRNSPEED);
        }
    }

    if (update & MARLIN_VAR_MSK(MARLIN_VAR_FLOWFACT)) {
        uint16_t flow = static_cast<uint16_t>(planner.flow_percentage[0]);
        if (marlin_server.vars.flow_factor != flow) {
            marlin_server.vars.flow_factor = flow;
            changes |= MARLIN_VAR_MSK(MARLIN_VAR_FLOWFACT);
        }
    }

    if (update & MARLIN_VAR_MSK(MARLIN_VAR_WAITHEAT)) {
        uint8_t wait = wait_for_heatup ? 1 : 0;
        if (marlin_server.vars.wait_heat != wait) {
            marlin_server.vars.wait_heat = wait;
            changes |= MARLIN_VAR_MSK(MARLIN_VAR_WAITHEAT);
        }
    }

    if (update & MARLIN_VAR_MSK(MARLIN_VAR_WAITUSER)) {
        uint8_t wait = wait_for_user ? 1 : 0;
        if (marlin_server.vars.wait_user != wait) {
            marlin_server.vars.wait_user = wait;
            changes |= MARLIN_VAR_MSK(MARLIN_VAR_WAITUSER);
        }
    }

    if (update & MARLIN_VAR_MSK(MARLIN_VAR_SD_PDONE)) {
        if (!FirstLayer::isPrinting()) { /// push notifications used for first layer calibration

            uint8_t progress = 0;
            if (oProgressData.oPercentDone.mIsActual(marlin_server.vars.print_duration))
                progress = static_cast<uint8_t>(oProgressData.oPercentDone.mGetValue());
            else
                progress = static_cast<uint8_t>(media_print_get_percent_done());
            if (marlin_server.vars.sd_percent_done != progress) {
                marlin_server.vars.sd_percent_done = progress;
                changes |= MARLIN_VAR_MSK(MARLIN_VAR_SD_PDONE);
            }
        }
    }

    if (update & MARLIN_VAR_MSK(MARLIN_VAR_DURATION)) {
        uint32_t timer = print_job_timer.duration();
        if (marlin_server.vars.print_duration != timer) {
            marlin_server.vars.print_duration = timer;
            changes |= MARLIN_VAR_MSK(MARLIN_VAR_DURATION);
        }
    }

    if (update & MARLIN_VAR_MSK(MARLIN_VAR_MEDIAINS)) {
        uint8_t media = media_get_state() == media_state_INSERTED ? 1 : 0;
        if (marlin_server.vars.media_inserted != media) {
            marlin_server.vars.media_inserted = media;
            changes |= MARLIN_VAR_MSK(MARLIN_VAR_MEDIAINS);
            _send_notify_event(marlin_server.vars.media_inserted ? MARLIN_EVT_MediaInserted : MARLIN_EVT_MediaRemoved, 0, 0);
        }
    }

    if (update & MARLIN_VAR_MSK(MARLIN_VAR_PRNSTATE)) {
        uint8_t print = marlin_server.print_state;
        if (marlin_server.vars.print_state != print) {
            marlin_server.vars.print_state = static_cast<marlin_print_state_t>(print);
            changes |= MARLIN_VAR_MSK(MARLIN_VAR_PRNSTATE);
        }
    }

    if (update & MARLIN_VAR_MSK(MARLIN_VAR_TIMTOEND)) {
        uint32_t progress = -1;
        if (oProgressData.oPercentDone.mIsActual(marlin_server.vars.print_duration))
            progress = oProgressData.oTime2End.mGetValue();
        if (marlin_server.vars.time_to_end != progress) {
            marlin_server.vars.time_to_end = progress;
            changes |= MARLIN_VAR_MSK(MARLIN_VAR_TIMTOEND);
        }
    }
#ifdef NEW_FANCTL
    if (update & MARLIN_VAR_MSK(MARLIN_VAR_PRINT_FAN_RPM)) {
        uint16_t rpm = fanctl_get_rpm(0);
        if (marlin_server.vars.print_fan_rpm != rpm) {
            marlin_server.vars.print_fan_rpm = rpm;
            changes |= MARLIN_VAR_MSK(MARLIN_VAR_PRINT_FAN_RPM);
        }
    }

    if (update & MARLIN_VAR_MSK(MARLIN_VAR_HEATBREAK_FAN_RPM)) {
        uint16_t rpm = fanctl_get_rpm(1);
        if (marlin_server.vars.heatbreak_fan_rpm != rpm) {
            marlin_server.vars.heatbreak_fan_rpm = rpm;
            changes |= MARLIN_VAR_MSK(MARLIN_VAR_HEATBREAK_FAN_RPM);
        }
    }
#endif
    if (update & MARLIN_VAR_MSK(MARLIN_VAR_JOB_ID)) {
        if (marlin_server.vars.job_id != job_id) {
            marlin_server.vars.job_id = job_id;
            changes |= MARLIN_VAR_MSK(MARLIN_VAR_JOB_ID);
        }
    }

    if (update & MARLIN_VAR_MSK(MARLIN_VAR_TRAVEL_ACCEL)) {
        if (marlin_server.vars.travel_acceleration != planner.settings.travel_acceleration) {
            marlin_server.vars.travel_acceleration = planner.settings.travel_acceleration;
        }
        changes |= MARLIN_VAR_MSK(MARLIN_VAR_TRAVEL_ACCEL);
    }

#define ES_STATE(S) READ(S##_PIN) // != S##_ENDSTOP_INVERTING

    if (update & MARLIN_VAR_MSK(MARLIN_VAR_ENDSTOPS)) {
        uint32_t endstops = 0;
#if HAS_X_MIN
        endstops = endstops | (ES_STATE(X_MIN) << 0);
#endif
#if HAS_Y_MIN
        endstops = endstops | (ES_STATE(Y_MIN) << 1);
#endif
#if HAS_Z_MIN
        endstops = endstops | (ES_STATE(Z_MIN) << 2);
#endif
#if HAS_X_MAX
        endstops = endstops | (ES_STATE(X_MAX) << 4);
#endif
#if HAS_Y_MAX
        endstops = endstops | (ES_STATE(Y_MAX) << 5);
#endif
#if HAS_Z_MAX
        endstops = endstops | (ES_STATE(Z_MAX) << 6);
#endif
#if HAS_X2_MIN
        endstops = endstops | (ES_STATE(X2_MIN) << 7);
#endif
#if HAS_X2_MAX
        endstops = endstops | (ES_STATE(X2_MAX) << 8);
#endif
#if HAS_Y2_MIN
        endstops = endstops | (ES_STATE(Y2_MIN) << 9);
#endif
#if HAS_Y2_MAX
        endstops = endstops | (ES_STATE(Y2_MAX) << 10);
#endif
#if HAS_Z2_MIN
        endstops = endstops | (ES_STATE(Z2_MIN) << 11);
#endif
#if HAS_Z2_MAX
        endstops = endstops | (ES_STATE(Z2_MAX) << 12);
#endif
#if HAS_Z3_MIN
        endstops = endstops | (ES_STATE(Z3_MIN) << 13);
#endif
#if HAS_Z3_MAX
        endstops = endstops | (ES_STATE(Z3_MAX) << 14);
#endif
        if (marlin_server.vars.endstops != endstops) {
            marlin_server.vars.endstops = endstops;
            changes |= MARLIN_VAR_MSK(MARLIN_VAR_ENDSTOPS);
        }
    }

    return changes;
}

void bsod_unknown_request(const char *request) {
    bsod("Unknown request %s", request);
}

// request must have 2 chars at least
bool _process_server_valid_request(const char *request, int client_id) {
    const char *data = request + 2;
    uint32_t msk32[2];
    int ival;

    // Log everything except !update
    if (request[1] != MARLIN_MSG_UPDATE_VARIABLE) {
        log_info(MarlinServer, "Processing %s (from %u)", request, client_id);
    }

    switch (request[1]) {

    case MARLIN_MSG_GCODE:
        //@TODO return value depending on success of enqueueing gcode
        return marlin_server_enqueue_gcode(data);
    case MARLIN_MSG_INJECT_GCODE: {
        unsigned long int iptr = strtoul(data, NULL, 0);
        return marlin_server_inject_gcode((const char *)iptr);
    }
    case MARLIN_MSG_START:
        marlin_server_start_processing();
        return true;
    case MARLIN_MSG_STOP:
        marlin_server_stop_processing();
        return true;
    case MARLIN_MSG_SET_VARIABLE:
        _server_set_var(data);
        return true;
    case MARLIN_MSG_UPDATE_VARIABLE:
        if (sscanf(data, "%08" SCNx32 " %08" SCNx32, msk32 + 0, msk32 + 1) != 2)
            return 0;
        _server_update_and_notify(client_id, msk32[0] + (((uint64_t)msk32[1]) << 32));
        return true;
    case MARLIN_MSG_BABYSTEP: {
        float offs;
        if (sscanf(data, "%f", &offs) != 1)
            return false;
        marlin_server_do_babystep_Z(offs);
        return true;
    }
    case MARLIN_MSG_CONFIG_SAVE:
        marlin_server_settings_save();
        return true;
    case MARLIN_MSG_CONFIG_LOAD:
        marlin_server_settings_load();
        return true;
    case MARLIN_MSG_CONFIG_RESET:
        marlin_server_settings_reset();
        return true;
    case MARLIN_MSG_UPDATE:
        marlin_server_manage_heater();
        return true;
    case MARLIN_MSG_QUICK_STOP:
        marlin_server_quick_stop();
        return true;
    case MARLIN_MSG_PRINT_START:
        marlin_server_print_start(data + 1, data[0] == '1');
        return true;
    case MARLIN_MSG_GUI_PRINT_READY:
        marlin_server_gui_ready_to_print();
        return true;
    case MARLIN_MSG_PRINT_ABORT:
        marlin_server_print_abort();
        return true;
    case MARLIN_MSG_PRINT_PAUSE:
        marlin_server_print_pause();
        return true;
    case MARLIN_MSG_PRINT_RESUME:
        marlin_server_print_resume();
        return true;
    case MARLIN_MSG_PRINT_EXIT:
        marlin_server_print_exit();
        return true;
    case MARLIN_MSG_PARK:
        marlin_server_park_head();
        return true;
    case MARLIN_MSG_KNOB_MOVE:
        ++marlin_server.knob_move_counter;
        return true;
    case MARLIN_MSG_KNOB_CLICK:
        ++marlin_server.knob_click_counter;
        return true;
    case MARLIN_MSG_FSM:
        if (sscanf(data, "%d", &ival) != 1)
            return false;
        // Allows to interrupt blocking waiting for heating/cooling
        // Remove once the Marlin's heating is non-blocking
        if (can_stop_wait_for_heatup() && ival >= 0) {
            marlin_server.vars.wait_heat = false;
            wait_for_heatup = false;
        }
        ClientResponseHandler::SetResponse(ival);
        return true;
    case MARLIN_MSG_EVENT_MASK:
        if (sscanf(data, "%08" SCNx32 " %08" SCNx32, msk32 + 0, msk32 + 1) != 2)
            return false;
        marlin_server.notify_events[client_id] = msk32[0] + (((uint64_t)msk32[1]) << 32);
        // Send MARLIN_EVT_MediaInserted event if media currently inserted
        // This is temporary solution, MARLIN_EVT_MediaInserted and MARLIN_EVT_MediaRemoved events are replaced
        // with variable media_inserted, but some parts of application still using the events.
        // We need this workaround for app startup.
        if ((marlin_server.notify_events[client_id] & MARLIN_EVT_MSK(MARLIN_EVT_MediaInserted)) && marlin_server.vars.media_inserted)
            marlin_server.client_events[client_id] |= MARLIN_EVT_MSK(MARLIN_EVT_MediaInserted);
        return true;
    case MARLIN_MSG_CHANGE_MASK:
        if (sscanf(data, "%08" SCNx32 " %08" SCNx32, msk32 + 0, msk32 + 1) != 2)
            return false;
        marlin_server.notify_changes[client_id] = msk32[0] + (((uint64_t)msk32[1]) << 32);
        marlin_server.client_changes[client_id] = msk32[0] + (((uint64_t)msk32[1]) << 32);
        return true;
    case MARLIN_MSG_EXCLUSIVE:
        if (sscanf(data, "%d", &ival) != 1)
            return false;
        //set exclusive mode
        if (ival) {
            marlin_server.flags |= MARLIN_SFLG_EXCMODE;
            queue.clear();
        } else
            marlin_server.flags &= ~MARLIN_SFLG_EXCMODE;
        return true;
    case MARLIN_MSG_TEST_START:
        if (sscanf(data, "%08" SCNx32 " %08" SCNx32, msk32 + 0, msk32 + 1) != 2)
            return false;
        //start selftest
        marlin_server_test_start(msk32[0] + (((uint64_t)msk32[1]) << 32));
        return true;
    case MARLIN_MSG_TEST_ABORT:
        marlin_server_test_abort();
        return true;
    case MARLIN_MSG_MOVE: {
        float offs;
        float fval;
        unsigned int uival;
        if (sscanf(data, "%f %f %u", &offs, &fval, &uival) != 3)
            return false;
        marlin_server_move_axis(offs, MMM_TO_MMS(fval), uival);
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
    if (request == nullptr)
        return false;
    int client_id = *(request++) - '0';
    if ((client_id < 0) || (client_id >= MARLIN_MAX_CLIENTS))
        return true;

    if (strlen(request) < 2 || request[0] != '!') {
        bsod_unknown_request(request);
    }

    const bool processed = _process_server_valid_request(request, client_id);
    if (processed)
        if (!_send_notify_event_to_client(client_id, marlin_client_queue[client_id], MARLIN_EVT_Acknowledge, 0, 0))
            // FIXME: Take care of resending process elsewhere.
            marlin_server.client_events[client_id] |= MARLIN_EVT_MSK(MARLIN_EVT_Acknowledge); // set bit if notification not sent immediately
    return processed;
}

// set variable from string request
static int _server_set_var(const char *const name_val_str) {
    if (name_val_str == nullptr)
        return 0;
    marlin_var_id_t var_id;
    bool changed = false;
    char *val_str = strchr(name_val_str, ' ');
    *(val_str++) = 0;
    if ((var_id = marlin_vars_get_id_by_name(name_val_str)) <= MARLIN_VAR_MAX) {
        if (marlin_vars_str_to_value(&(marlin_server.vars), var_id, val_str) == 1) {
            switch (var_id) {
            case MARLIN_VAR_TTEM_NOZ:
                changed = (thermalManager.temp_hotend[0].target != marlin_server.vars.target_nozzle);
                // if print is paused we want to change the resume temp and turn off timeout
                // this prevents going back to temperature before pause and enables to heat nozzle during pause
                if (marlin_server.print_state == mpsPaused) {
                    marlin_server_nozzle_timeout_off();
                    marlin_server.resume.nozzle_temp = marlin_server.vars.target_nozzle;
                }
                thermalManager.setTargetHotend(marlin_server.vars.target_nozzle, 0);
                break;
            case MARLIN_VAR_TTEM_BED:
                changed = (thermalManager.temp_bed.target != marlin_server.vars.target_bed);
                thermalManager.setTargetBed(marlin_server.vars.target_bed);
                break;
            case MARLIN_VAR_Z_OFFSET:
#if HAS_BED_PROBE
                changed = (probe.offset.z != marlin_server.vars.z_offset);
                probe.offset.z = marlin_server.vars.z_offset;
#endif //HAS_BED_PROBE
                break;
            case MARLIN_VAR_FANSPEED:
#if FAN_COUNT > 0
                changed = (thermalManager.fan_speed[0] != marlin_server.vars.print_fan_speed);
                thermalManager.set_fan_speed(0, marlin_server.vars.print_fan_speed);
#endif
                break;
            case MARLIN_VAR_PRNSPEED:
                changed = (feedrate_percentage != (int16_t)marlin_server.vars.print_speed);
                feedrate_percentage = (int16_t)marlin_server.vars.print_speed;
                break;
            case MARLIN_VAR_FLOWFACT:
                changed = (planner.flow_percentage[0] != (int16_t)marlin_server.vars.flow_factor);
                planner.flow_percentage[0] = (int16_t)marlin_server.vars.flow_factor;
                planner.refresh_e_factor(0);
                break;
            case MARLIN_VAR_WAITHEAT:
                changed = true;
                wait_for_heatup = marlin_server.vars.wait_heat ? true : false;
                break;
            case MARLIN_VAR_WAITUSER:
                changed = true;
                wait_for_user = marlin_server.vars.wait_user ? true : false;
                break;
            default:
                log_error(MarlinServer, "unimplemented _server_set_var for var_id %i", (int)var_id);
                break;
            }
            if (changed) {
                int client_id;
                uint64_t var_msk = MARLIN_VAR_MSK(var_id);
                for (client_id = 0; client_id < MARLIN_MAX_CLIENTS; client_id++)
                    marlin_server.client_changes[client_id] |= (var_msk & marlin_server.notify_changes[client_id]);
            }
        } else {
            log_error(MarlinServer, "Unable to parse var-value pair %s", val_str);
        }
    }
    //	_dbg("_server_set_var %d %s %s", var_id, name_val_str, val_str);
    return 1;
}

// update variables defined by 'update' mask and send notification to client that requested updating
// other clients will receive notification in next cycle
static void _server_update_and_notify(int client_id, uint64_t update) {
    int id;
    osMessageQId queue;
    uint64_t changes = _server_update_vars(update);
    for (id = 0; id < MARLIN_MAX_CLIENTS; id++)
        if (id == client_id) {
            marlin_server.client_changes[id] |= changes;
            if ((queue = marlin_client_queue[id]) != 0)
                marlin_server.client_changes[id] &= ~_send_notify_changes_to_client(id, queue, update);
        } else
            marlin_server.client_changes[id] |= (changes & marlin_server.notify_changes[id]);
}

} // extern "C"

#ifdef DEBUG_FSENSOR_IN_HEADER
int _is_in_M600_flg = 0;
#endif

//-----------------------------------------------------------------------------
// ExtUI event handlers

namespace ExtUI {

void onStartup() {
    _log_event(LOG_SEVERITY_INFO, &LOG_COMPONENT(MarlinServer), "ExtUI: onStartup");
    _send_notify_event(MARLIN_EVT_Startup, 0, 0);
}

void onIdle() {
    marlin_server_idle();
    if (marlin_server_idle_cb)
        marlin_server_idle_cb();
}

void onPrinterKilled(PGM_P const msg, PGM_P const component) {
    _log_event(LOG_SEVERITY_INFO, &LOG_COMPONENT(MarlinServer), "Printer killed: %s", msg);
    vTaskEndScheduler();
    wdt_iwdg_refresh(); //watchdog reset
    fatal_error(msg, component);
}

void onMediaInserted() {
    _log_event(LOG_SEVERITY_INFO, &LOG_COMPONENT(MarlinServer), "ExtUI: onMediaInserted");
    _send_notify_event(MARLIN_EVT_MediaInserted, 0, 0);
}

void onMediaError() {
    _log_event(LOG_SEVERITY_INFO, &LOG_COMPONENT(MarlinServer), "ExtUI: onMediaError");
    _send_notify_event(MARLIN_EVT_MediaError, 0, 0);
}

void onMediaRemoved() {
    _log_event(LOG_SEVERITY_INFO, &LOG_COMPONENT(MarlinServer), "ExtUI: onMediaRemoved");
    _send_notify_event(MARLIN_EVT_MediaRemoved, 0, 0);
}

void onPlayTone(const uint16_t frequency, const uint16_t duration) {
    _log_event(LOG_SEVERITY_INFO, &LOG_COMPONENT(MarlinServer), "ExtUI: onPlayTone");
    _send_notify_event(MARLIN_EVT_PlayTone, frequency, duration);
}

void onPrintTimerStarted() {
    _log_event(LOG_SEVERITY_INFO, &LOG_COMPONENT(MarlinServer), "ExtUI: onPrintTimerStarted");
    _send_notify_event(MARLIN_EVT_PrintTimerStarted, 0, 0);
}

void onPrintTimerPaused() {
    _log_event(LOG_SEVERITY_INFO, &LOG_COMPONENT(MarlinServer), "ExtUI: onPrintTimerPaused");
    _send_notify_event(MARLIN_EVT_PrintTimerPaused, 0, 0);
}

void onPrintTimerStopped() {
    _log_event(LOG_SEVERITY_INFO, &LOG_COMPONENT(MarlinServer), "ExtUI: onPrintTimerStopped");
    _send_notify_event(MARLIN_EVT_PrintTimerStopped, 0, 0);
}

void onFilamentRunout(const extruder_t extruder) {
    _log_event(LOG_SEVERITY_INFO, &LOG_COMPONENT(MarlinServer), "ExtUI: onFilamentRunout");
    _send_notify_event(MARLIN_EVT_FilamentRunout, 0, 0);
}

void onUserConfirmRequired(const char *const msg) {
    _log_event(LOG_SEVERITY_INFO, &LOG_COMPONENT(MarlinServer), "ExtUI: onUserConfirmRequired: %s", msg);
    _send_notify_event(MARLIN_EVT_UserConfirmRequired, 0, 0);
}

#if HAS_BED_PROBE
static void mbl_error(int error_code) {
    if (marlin_server.print_state != mpsPrinting && marlin_server.print_state != mpsPausing_Begin)
        return;

    marlin_server.print_state = mpsPausing_Failed_Code;
    /// pause immediatelly to save current file position
    pause_print(Pause_Type::Repeat_Last_Code);
    marlin_server.mbl_failed = true;
    _send_notify_event(MARLIN_EVT_Error, error_code, 0);
}
#endif

void onStatusChanged(const char *const msg) {
    if (!msg)
        return; // ignore errorneous nullptr messages

    static bool pending_err_msg = false;

    _log_event(LOG_SEVERITY_INFO, &LOG_COMPONENT(MarlinServer), "ExtUI: onStatusChanged: %s", msg);
    _send_notify_event(MARLIN_EVT_StatusChanged, 0, 0);
    if (msg != nullptr && strcmp(msg, "Prusa-mini Ready.") == 0) {
    } //TODO
    else if (msg != nullptr && strcmp(msg, "TMC CONNECTION ERROR") == 0)
        _send_notify_event(MARLIN_EVT_Error, MARLIN_ERR_TMCDriverError, 0);
    else {
        if (!is_abort_state(marlin_server.print_state))
            pending_err_msg = false;
        if (!pending_err_msg) {
/// FIXME: Message through Marlin's UI could be delayed and we won't pause print at the MBL command
#if HAS_BED_PROBE
            if (strcmp(msg, MSG_ERR_PROBING_FAILED) == 0) {
                mbl_error(MARLIN_ERR_ProbingFailed);
                pending_err_msg = true;
            }
#endif
            if (msg[0] != 0) { //empty message filter
                _add_status_msg(msg);
                _send_notify_event(MARLIN_EVT_Message, 0, 0);
            }
        }
    }
}

void onFactoryReset() {
    _log_event(LOG_SEVERITY_INFO, &LOG_COMPONENT(MarlinServer), "ExtUI: onFactoryReset");
    _send_notify_event(MARLIN_EVT_FactoryReset, 0, 0);
}

void onLoadSettings(char const *) {
    _log_event(LOG_SEVERITY_INFO, &LOG_COMPONENT(MarlinServer), "ExtUI: onLoadSettings");
    _send_notify_event(MARLIN_EVT_LoadSettings, 0, 0);
}

void onStoreSettings(char *) {
    _log_event(LOG_SEVERITY_INFO, &LOG_COMPONENT(MarlinServer), "ExtUI: onStoreSettings");
    _send_notify_event(MARLIN_EVT_StoreSettings, 0, 0);
}

void onConfigurationStoreWritten(bool success) {
    _log_event(LOG_SEVERITY_INFO, &LOG_COMPONENT(MarlinServer), "ExtUI: onConfigurationStoreWritten");
}

void onConfigurationStoreRead(bool success) {
    _log_event(LOG_SEVERITY_INFO, &LOG_COMPONENT(MarlinServer), "ExtUI: onConfigurationStoreRead");
}

void onMeshUpdate(const uint8_t xpos, const uint8_t ypos, const float zval) {
    _log_event(LOG_SEVERITY_DEBUG, &LOG_COMPONENT(MarlinServer), "ExtUI: onMeshUpdate x: %u, y: %u, z: %.2f", xpos, ypos, (double)zval);
    uint32_t usr32 = variant8_get_ui32(variant8_flt(zval));
    uint16_t usr16 = xpos | ((uint16_t)ypos << 8);
    _send_notify_event(MARLIN_EVT_MeshUpdate, usr32, usr16);
}

}

const marlin_vars_t &marlin_server_read_vars() {
    return marlin_server.vars;
}

/// Array of the last phase per fsm
/// Used for better logging experience in fsm_change
static int fsm_last_phase[int(ClientFSM::_count)];

void fsm_create(ClientFSM type, uint8_t data) {
    _log_event(LOG_SEVERITY_INFO, &LOG_COMPONENT(MarlinServer), "Creating state machine [%d]", int(type));
    fsm_last_phase[static_cast<int>(type)] = -1;

    for (size_t i = 0; i < MARLIN_MAX_CLIENTS; ++i) {
        fsm_event_queues[i].PushCreate(type, data);
    }

    _send_notify_event(MARLIN_EVT_FSM, 0, 0); // do not send data, _send_notify_event_to_client does not use them for this event
}

void fsm_destroy(ClientFSM type) {
    _log_event(LOG_SEVERITY_INFO, &LOG_COMPONENT(MarlinServer), "Destroying state machine [%d]", int(type));

    for (size_t i = 0; i < MARLIN_MAX_CLIENTS; ++i) {
        fsm_event_queues[i].PushDestroy(type);
    }
    _send_notify_event(MARLIN_EVT_FSM, 0, 0); // do not send data, _send_notify_event_to_client does not use them for this event
}

void _fsm_change(ClientFSM type, fsm::BaseData data) {
    if (fsm_last_phase[static_cast<int>(type)] != static_cast<int>(data.GetPhase())) {
        _log_event(LOG_SEVERITY_INFO, &LOG_COMPONENT(MarlinServer), "Change state of [%i] to %" PRIu8, static_cast<int>(type), data.GetPhase());
        fsm_last_phase[static_cast<int>(type)] = static_cast<int>(data.GetPhase());
    }

    for (size_t i = 0; i < MARLIN_MAX_CLIENTS; ++i) {
        fsm_event_queues[i].PushChange(type, data);
    }
    _send_notify_event(MARLIN_EVT_FSM, 0, 0); // do not send data, _send_notify_event_to_client does not use them for this event
}

void set_warning(WarningType type) {
    _log_event(LOG_SEVERITY_WARNING, &LOG_COMPONENT(MarlinServer), "Warning type %d set", (int)type);

    const MARLIN_EVT_t evt_id = MARLIN_EVT_Warning;
    _send_notify_event(evt_id, uint32_t(type), 0);
}

/*****************************************************************************/
//FSM_notifier
FSM_notifier::data FSM_notifier::s_data;
FSM_notifier *FSM_notifier::activeInstance = nullptr;

FSM_notifier::FSM_notifier(ClientFSM type, uint8_t phase, variant8_t min, variant8_t max,
    uint8_t progress_min, uint8_t progress_max, marlin_var_id_t var_id)
    : temp_data(s_data) {
    s_data.type = type;
    s_data.phase = phase;
    s_data.scale = static_cast<float>(progress_max - progress_min) / static_cast<float>(variant8_get_flt(max) - variant8_get_flt(min));
    s_data.offset = -variant8_get_flt(min) * s_data.scale + static_cast<float>(progress_min);
    s_data.progress_min = progress_min;
    s_data.progress_max = progress_max;
    s_data.var_id = var_id;
    s_data.last_progress_sent = std::nullopt;
    activeInstance = this;
}

//static method
//notifies clients about progress rise
//scales "bound" variable via following formula to calculate progress
//x = (actual - s_data.min) * s_data.scale + s_data.progress_min;
//x = actual * s_data.scale - s_data.min * s_data.scale + s_data.progress_min;
//s_data.offset == -s_data.min * s_data.scale + s_data.progress_min
//simplified formula
//x = actual * s_data.scale + s_data.offset;
void FSM_notifier::SendNotification() {
    if (!activeInstance)
        return;
    if (s_data.type == ClientFSM::_none)
        return;
    activeInstance->preSendNotification();
    variant8_t temp = marlin_vars_get_var(&(marlin_server.vars), s_data.var_id);

    float actual = variant8_get_flt(temp);
    actual = actual * s_data.scale + s_data.offset;

    int progress = static_cast<int>(actual); //int - must be signed
    if (progress < s_data.progress_min)
        progress = s_data.progress_min;
    if (progress > s_data.progress_max)
        progress = s_data.progress_max;

    // after first sent, progress can only rise
    // no value: comparison returns true
    if (progress > s_data.last_progress_sent) {
        s_data.last_progress_sent = progress;
        ProgressSerializer serializer(progress);
        _fsm_change(s_data.type, fsm::BaseData(s_data.phase, serializer.Serialize()));
    }
    activeInstance->postSendNotification();
}

FSM_notifier::~FSM_notifier() {
    s_data = temp_data;
    activeInstance = nullptr;
}

/*****************************************************************************/
//ClientResponseHandler
//define static member
//UINT32_MAX is used as no response from client
std::atomic<uint32_t> ClientResponseHandler::server_side_encoded_response = UINT32_MAX;

uint8_t get_var_sd_percent_done() {
    return marlin_server.vars.sd_percent_done;
}

void set_var_sd_percent_done(uint8_t value) {
    marlin_server.vars.sd_percent_done = value;
    _set_notify_change(MARLIN_VAR_SD_PDONE);
}

#if ENABLED(CRASH_RECOVERY)
void set_length(xy_float_t xy) {
    marlin_server.axis_length = xy;
}
#endif
