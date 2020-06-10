// marlin_server.cpp

#include "marlin_server.h"
#include "marlin_server.hpp"
#include <stdarg.h>
#include <stdio.h>
#include "dbg.h"
#include "app.h"
#include "bsod.h"
#include "cmsis_os.h"
#include <string.h> //strncmp
#include <assert.h>

#include "../Marlin/src/lcd/extensible_ui/ui_api.h"
#include "../Marlin/src/gcode/queue.h"
#include "../Marlin/src/gcode/parser.h"
#include "../Marlin/src/module/planner.h"
#include "../Marlin/src/module/stepper.h"
#include "../Marlin/src/module/temperature.h"
#include "../Marlin/src/module/probe.h"
#include "../Marlin/src/module/configuration_store.h"
#include "../Marlin/src/module/printcounter.h"
#include "../Marlin/src/feature/babystep.h"
#include "../Marlin/src/feature/pause.h"
#include "../Marlin/src/libs/nozzle.h"
#include "../Marlin/src/core/language.h" //GET_TEXT(MSG)
#include "../Marlin/src/gcode/gcode.h"
#include "../Marlin/src/gcode/lcd/M73_PE.h"

#include "hwio.h"
#include "eeprom.h"
#include "media.h"
#include "filament_sensor.h"
#include "wdt.h"

static_assert(MARLIN_VAR_MAX < 64, "MarlinAPI: Too many variables");

#ifdef MINDA_BROKEN_CABLE_DETECTION
    #include "Z_probe.h" //get_Z_probe_endstop_hits
#endif

#define DBG _dbg1 //enabled level 1
//#define DBG(...)

//#define DBG_XUI DBG //trace ExtUI events
#define DBG_XUI(...) //disable trace

//#define DBG_REQ DBG //trace requests
#define DBG_REQ(...) //disable trace

//#define DBG_FSM DBG //trace fsm
#define DBG_FSM(...) //disable trace

#pragma pack(push)
#pragma pack(1)

typedef struct _marlin_server_t {
    uint16_t flags;                              // server flags (MARLIN_SFLG)
    uint64_t notify_events[MARLIN_MAX_CLIENTS];  // event notification mask
    uint64_t notify_changes[MARLIN_MAX_CLIENTS]; // variable change notification mask
    marlin_vars_t vars;                          // cached variables
    char request[MARLIN_MAX_REQUEST];
    int request_len;
    uint64_t client_events[MARLIN_MAX_CLIENTS];      // client event mask
    uint64_t client_changes[MARLIN_MAX_CLIENTS];     // client variable change mask
    uint32_t last_update;                            // last update tick count
    uint8_t idle_cnt;                                // idle call counter
    uint8_t pqueue_head;                             // copy of planner.block_buffer_head
    uint8_t pqueue_tail;                             // copy of planner.block_buffer_tail
    uint8_t pqueue;                                  // calculated number of records in planner queue
    uint8_t gqueue;                                  // copy of queue.length - number of commands in gcode queue
    uint32_t command;                                // actually running command
    uint32_t command_begin;                          // variable for notification
    uint32_t command_end;                            // variable for notification
    marlin_mesh_t mesh;                              // meshbed leveling
    uint64_t mesh_point_notsent[MARLIN_MAX_CLIENTS]; // mesh point mask (points that are not sent)
    uint64_t update_vars;                            // variable update mask
    marlin_print_state_t print_state;                // printing state (printing, paused, ...)
    float resume_pos[4];                             // resume position for unpark_head
    float resume_nozzle_temp;                        // resume nozzle temperature
    uint8_t resume_fan_speed;                        // resume fan speed
    uint32_t paused_ticks;                           // tick count in moment when printing paused
    uint32_t fsmCreate;                              // fsm create ui32 argument for resend
    uint32_t fsmDestroy;                             // fsm destroy ui32 argument for resend
    uint32_t fsmChange;                              // fsm change ui32 argument for resend
} marlin_server_t;

#pragma pack(pop)

extern "C" {

//-----------------------------------------------------------------------------
// variables
extern uint32_t Tacho_FAN0;
extern uint32_t Tacho_FAN1;

osThreadId marlin_server_task = 0;    // task handle
osMessageQId marlin_server_queue = 0; // input queue (uint8_t)
osSemaphoreId marlin_server_sema = 0; // semaphore handle

marlin_server_t marlin_server; // server structure - initialize task to zero
#ifdef DEBUG_FSENSOR_IN_HEADER
uint32_t *pCommand = &marlin_server.command;
#endif
marlin_server_idle_t *marlin_server_idle_cb = 0; // idle callback

//==========MSG_STACK===================
//	top of the stack is at [0]

msg_stack_t msg_stack = { '\0', 0 };

void _add_status_msg(const char *const popup_msg) {
    char message[MSG_MAX_LENGTH];
    memset(message, '\0', sizeof(message) * sizeof(char)); // set to zeros to be on the safe side

    strlcpy(message, popup_msg, MSG_MAX_LENGTH);

    for (uint8_t i = msg_stack.count; i; i--) {
        if (i == MSG_STACK_SIZE)
            i--; // last place of the limited stack will be always overwritten

        memset(msg_stack.msg_data[i], '\0', sizeof(msg_stack.msg_data[i]) * sizeof(char)); // set to zeros to be on the safe side
        strlcpy(msg_stack.msg_data[i], msg_stack.msg_data[i - 1], sizeof(msg_stack.msg_data[i]));
    }

    memset(msg_stack.msg_data[0], '\0', sizeof(msg_stack.msg_data[0]) * sizeof(char)); // set to zeros to be on the safe side
    strlcpy(msg_stack.msg_data[0], message, sizeof(msg_stack.msg_data[0]));

    if (msg_stack.count < MSG_STACK_SIZE)
        msg_stack.count++;
}

//-----------------------------------------------------------------------------
// external variables from marlin_client

extern osThreadId marlin_client_task[MARLIN_MAX_CLIENTS];    // task handles
extern osMessageQId marlin_client_queue[MARLIN_MAX_CLIENTS]; // input queue handles (uint32_t)

//-----------------------------------------------------------------------------
// forward declarations of private functions

static void _server_print_loop(void);
static int _send_notify_to_client(osMessageQId queue, variant8_t msg);
static int _send_notify_event_to_client(int client_id, osMessageQId queue, MARLIN_EVT_t evt_id, uint32_t usr32, uint16_t usr16);
static uint64_t _send_notify_events_to_client(int client_id, osMessageQId queue, uint64_t evt_msk);
static uint8_t _send_notify_event(MARLIN_EVT_t evt_id, uint32_t usr32, uint16_t usr16);
static int _send_notify_change_to_client(osMessageQId queue, uint8_t var_id, variant8_t var);
static uint64_t _send_notify_changes_to_client(int client_id, osMessageQId queue, uint64_t var_msk);
static void _set_notify_change(uint8_t var_id);
static void _server_update_gqueue(void);
static void _server_update_pqueue(void);
static uint64_t _server_update_vars(uint64_t force_update_msk);
static int _process_server_request(char *request);
static int _server_set_var(char *name_val_str);
static void _server_update_and_notify(int client_id, uint64_t update);

//-----------------------------------------------------------------------------
// server side functions

void marlin_server_init(void) {
    int i;
    memset(&marlin_server, 0, sizeof(marlin_server_t));
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
    marlin_server.mesh.xc = 4;
    marlin_server.mesh.yc = 4;
    marlin_server.update_vars = MARLIN_VAR_MSK_DEF;
    marlin_server.vars.media_LFN = media_print_filename();
    marlin_server.vars.media_SFN_path = media_print_filepath();
}

void print_fan_spd() {
    if (DEBUGGING(INFO)) {
        static int time = 0;
        static int last_prt = 0;
        time = HAL_GetTick();
        int timediff = time - last_prt;
        if (timediff >= 1000) {

            serial_echopair_PGM("Tacho_FAN0 ", (30 * 1000 * Tacho_FAN0) / timediff); //60s / 2 pulses per rotation
            serialprintPGM("rpm ");
            SERIAL_EOL();
            serial_echopair_PGM("Tacho_FAN1 ", (30 * 1000 * Tacho_FAN1) / timediff);
            serialprintPGM("rpm ");
            SERIAL_EOL();
            Tacho_FAN0 = 0;
            Tacho_FAN1 = 0;
            last_prt = time;
        }
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
int marlin_server_cycle(void) {

    static int processing = 0;
    if (processing)
        return 0;
    processing = 1;

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
    tick = HAL_GetTick();
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
            // send events to client only when all changes already sent, clear bits for successful sent notification
            if ((marlin_server.client_changes[client_id]) == 0)
                if ((msk = marlin_server.client_events[client_id]) != 0)
                    marlin_server.client_events[client_id] &= ~_send_notify_events_to_client(client_id, queue, msk);
        }
    if ((marlin_server.flags & MARLIN_SFLG_PROCESS) == 0)
        wdt_iwdg_refresh(); // this prevents iwdg reset while processing disabled
    processing = 0;
    return count;
}

#define MARLIN_IDLE_CNT_BUSY 1

int marlin_server_loop(void) {
    if (marlin_server.idle_cnt >= MARLIN_IDLE_CNT_BUSY)
        if (marlin_server.flags & MARLIN_SFLG_BUSY) {
            //_dbg("SVR: READY");
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
    if (marlin_server.idle_cnt < MARLIN_IDLE_CNT_BUSY)
        marlin_server.idle_cnt++;
    else if ((marlin_server.flags & MARLIN_SFLG_BUSY) == 0) {
        //_dbg("SVR: BUSY");
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

int marlin_server_enqueue_gcode(const char *gcode) {
    return queue.enqueue_one(gcode) ? 1 : 0;
}

int marlin_server_inject_gcode(const char *gcode) {
    queue.inject_P(gcode);
    return 1;
}

void marlin_server_settings_save(void) {
    eeprom_set_var(EEVAR_ZOFFSET, variant8_flt(probe_offset.z));
    eeprom_set_var(EEVAR_PID_BED_P, variant8_flt(Temperature::temp_bed.pid.Kp));
    eeprom_set_var(EEVAR_PID_BED_I, variant8_flt(Temperature::temp_bed.pid.Ki));
    eeprom_set_var(EEVAR_PID_BED_D, variant8_flt(Temperature::temp_bed.pid.Kd));
    eeprom_set_var(EEVAR_PID_NOZ_P, variant8_flt(Temperature::temp_hotend[0].pid.Kp));
    eeprom_set_var(EEVAR_PID_NOZ_I, variant8_flt(Temperature::temp_hotend[0].pid.Ki));
    eeprom_set_var(EEVAR_PID_NOZ_D, variant8_flt(Temperature::temp_hotend[0].pid.Kd));
}

void marlin_server_settings_load(void) {
    (void)settings.reset();
#if HAS_BED_PROBE
    probe_offset.z = eeprom_get_var(EEVAR_ZOFFSET).flt;
#endif
    Temperature::temp_bed.pid.Kp = eeprom_get_var(EEVAR_PID_BED_P).flt;
    Temperature::temp_bed.pid.Ki = eeprom_get_var(EEVAR_PID_BED_I).flt;
    Temperature::temp_bed.pid.Kd = eeprom_get_var(EEVAR_PID_BED_D).flt;
    Temperature::temp_hotend[0].pid.Kp = eeprom_get_var(EEVAR_PID_NOZ_P).flt;
    Temperature::temp_hotend[0].pid.Ki = eeprom_get_var(EEVAR_PID_NOZ_I).flt;
    Temperature::temp_hotend[0].pid.Kd = eeprom_get_var(EEVAR_PID_NOZ_D).flt;
    thermalManager.updatePID();
}

void marlin_server_settings_reset(void) {
    (void)settings.reset();
}

void marlin_server_manage_heater(void) {
    thermalManager.manage_heater();
}

void marlin_server_quick_stop(void) {
    planner.quick_stop();
}

void marlin_server_print_start(const char *filename) {
    if ((marlin_server.print_state == mpsIdle) || (marlin_server.print_state == mpsFinished) || (marlin_server.print_state == mpsAborted)) {
        media_print_start(filename);
        _set_notify_change(MARLIN_VAR_FILEPATH);
        _set_notify_change(MARLIN_VAR_FILENAME);
        print_job_timer.start();
        marlin_server.print_state = mpsPrinting;
    }
}

void marlin_server_print_abort(void) {
    if ((marlin_server.print_state == mpsPrinting) || (marlin_server.print_state == mpsPaused)) {
        marlin_server.print_state = mpsAborting_Begin;
    }
}

void marlin_server_print_pause(void) {
    if (marlin_server.print_state == mpsPrinting) {
        marlin_server.print_state = mpsPausing_Begin;
    }
}

void marlin_server_print_resume(void) {
    if (marlin_server.print_state == mpsPaused) {
        marlin_server.print_state = mpsResuming_Begin;
    }
}

void marlin_server_print_reheat_start(void) {
    if ((marlin_server.print_state == mpsPaused) && (marlin_server_print_reheat_ready() == 0)) {
        thermalManager.setTargetHotend(marlin_server.resume_nozzle_temp, 0);
    }
}

int marlin_server_print_reheat_ready(void) {
    if (marlin_server.vars.target_nozzle == marlin_server.resume_nozzle_temp)
        if (marlin_server.vars.temp_nozzle >= (marlin_server.vars.target_nozzle - 5))
            return 1;
    return 0;
}

static void _server_print_loop(void) {
    switch (marlin_server.print_state) {
    case mpsIdle:
        break;
    case mpsPrinting:
        switch (media_print_get_state()) {
        case media_print_state_PRINTING:
            break;
        case media_print_state_PAUSED:
            marlin_server.print_state = mpsPausing_Begin;
            break;
        case media_print_state_NONE:
            marlin_server.print_state = mpsFinishing_WaitIdle;
            break;
        }
        break;
    case mpsPausing_Begin:
        media_print_pause();
        print_job_timer.pause();
        marlin_server.resume_nozzle_temp = marlin_server.vars.target_nozzle; //save nozzle target temp
        marlin_server.resume_fan_speed = marlin_server.vars.fan_speed;       //save fan speed
#if FAN_COUNT > 0
        thermalManager.set_fan_speed(0, 0); //disable print fan
#endif
        marlin_server.print_state = mpsPausing_WaitIdle;
        break;
    case mpsPausing_WaitIdle:
        if ((planner.movesplanned() == 0) && (queue.length == 0)) {
            marlin_server_park_head();
            marlin_server.print_state = mpsPausing_ParkHead;
        }
        break;
    case mpsPausing_ParkHead:
        if (planner.movesplanned() == 0) {
            marlin_server.paused_ticks = HAL_GetTick(); //time when printing paused
            marlin_server.print_state = mpsPaused;
        }
        break;
    case mpsPaused:
        if ((marlin_server.vars.target_nozzle > 0) && (HAL_GetTick() - marlin_server.paused_ticks > (1000 * PAUSE_NOZZLE_TIMEOUT)))
            thermalManager.setTargetHotend(0, 0);
        gcode.reset_stepper_timeout(); //prevent disable axis
        break;
    case mpsResuming_Begin:
        if (marlin_server_print_reheat_ready()) {
            marlin_server_unpark_head();
            marlin_server.print_state = mpsResuming_UnparkHead;
        } else {
            thermalManager.setTargetHotend(marlin_server.resume_nozzle_temp, 0);
            marlin_server.print_state = mpsResuming_Reheating;
        }
        break;
    case mpsResuming_Reheating:
        if (marlin_server_print_reheat_ready()) {
            marlin_server_unpark_head();
            marlin_server.print_state = mpsResuming_UnparkHead;
        }
        break;
    case mpsResuming_UnparkHead:
        if (planner.movesplanned() == 0) {
            media_print_resume();
            print_job_timer.resume(0);
#if FAN_COUNT > 0
            thermalManager.set_fan_speed(0, marlin_server.resume_fan_speed); // restore fan speed
#endif
            marlin_server.print_state = mpsPrinting;
        }
        break;
    case mpsAborting_Begin:
        media_print_stop();
        thermalManager.disable_all_heaters();
#if FAN_COUNT > 0
        thermalManager.set_fan_speed(0, 0);
#endif
        marlin_server_set_temp_to_display(0);
        print_job_timer.stop();
        planner.quick_stop();
        marlin_server.print_state = mpsAborting_WaitIdle;
        break;
    case mpsAborting_WaitIdle:
        if (planner.movesplanned() == 0) {
            float x = ((float)stepper.position(X_AXIS)) / planner.settings.axis_steps_per_mm[X_AXIS];
            float y = ((float)stepper.position(Y_AXIS)) / planner.settings.axis_steps_per_mm[Y_AXIS];
            float z = ((float)stepper.position(Z_AXIS)) / planner.settings.axis_steps_per_mm[Z_AXIS];
            current_position.x = x;
            current_position.y = y;
            current_position.z = z;
            current_position.e = 0;
            planner.set_position_mm(x, y, z, 0);
            marlin_server_park_head();
            marlin_server.print_state = mpsAborting_ParkHead;
        }
        break;
    case mpsAborting_ParkHead:
        if (planner.movesplanned() == 0) {
            marlin_server.print_state = mpsAborted;
        }
        break;
    case mpsFinishing_WaitIdle:
        if (planner.movesplanned() == 0) {
            marlin_server_park_head();
            marlin_server.print_state = mpsFinishing_ParkHead;
        }
        break;
    case mpsFinishing_ParkHead:
        if (planner.movesplanned() == 0) {
            marlin_server.print_state = mpsFinished;
        }
        break;
    default:
        break;
    }
}

void marlin_server_park_head(void) {
    constexpr feedRate_t fr_xy = NOZZLE_PARK_XY_FEEDRATE, fr_z = NOZZLE_PARK_Z_FEEDRATE;
    constexpr xyz_pos_t park = NOZZLE_PARK_POINT;
    //homed check
    if (all_axes_homed() && all_axes_known()) {
        planner.synchronize();
        marlin_server.resume_pos[0] = current_position.x;
        marlin_server.resume_pos[1] = current_position.y;
        marlin_server.resume_pos[2] = current_position.z;
        marlin_server.resume_pos[3] = current_position.e;
        current_position.e -= (float)PAUSE_PARK_RETRACT_LENGTH / planner.e_factor[active_extruder];
        line_to_current_position(PAUSE_PARK_RETRACT_FEEDRATE);
        current_position.z = _MIN(current_position.z + park.z, Z_MAX_POS);
        line_to_current_position(fr_z);
        current_position.set(park.x, park.y);
        line_to_current_position(fr_xy);
    }
}

void marlin_server_unpark_head(void) {
    constexpr feedRate_t fr_xy = NOZZLE_PARK_XY_FEEDRATE, fr_z = NOZZLE_PARK_Z_FEEDRATE;
    if (all_axes_homed() && all_axes_known()) {
        planner.synchronize();
        current_position.set(marlin_server.resume_pos[0], marlin_server.resume_pos[1]);
        line_to_current_position(fr_xy);
        current_position.z = marlin_server.resume_pos[2];
        line_to_current_position(fr_z);
        current_position.e += (float)PAUSE_PARK_RETRACT_LENGTH / planner.e_factor[active_extruder];
        line_to_current_position(PAUSE_PARK_RETRACT_FEEDRATE);
        current_position.e = marlin_server.resume_pos[3];
    }
}

int marlin_all_axes_homed(void) {
    return all_axes_homed() ? 1 : 0;
}

int marlin_all_axes_known(void) {
    return all_axes_known() ? 1 : 0;
}

void marlin_server_set_temp_to_display(float value) {
    marlin_server.vars.display_nozzle = value;
    _set_notify_change(MARLIN_VAR_DTEM_NOZ); //set change flag
}
float marlin_server_get_temp_to_display(void) {
    return marlin_server.vars.display_nozzle;
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
    osMessagePut(queue, *(((uint32_t *)(&msg)) + 0), osWaitForever);
    osMessagePut(queue, *(((uint32_t *)(&msg)) + 1), osWaitForever);
    return 1;
}

// send event notification to client (called from server thread)
static int _send_notify_event_to_client(int client_id, osMessageQId queue, MARLIN_EVT_t evt_id, uint32_t usr32, uint16_t usr16) {
    variant8_t msg;
    msg = variant8_user(usr32, usr16, evt_id);
    return _send_notify_to_client(queue, msg);
}

// send event notification to client - multiple events (called from server thread)
// returns mask of succesfull sent events
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
                if (marlin_server.mesh_point_notsent[client_id]) {
                    uint8_t x;
                    uint8_t y;
                    uint64_t mask = 1;
                    for (y = 0; y < marlin_server.mesh.yc; y++)
                        for (x = 0; x < marlin_server.mesh.xc; x++) {
                            if (mask & marlin_server.mesh_point_notsent[client_id]) {
                                uint8_t index = x + marlin_server.mesh.xc * y;
                                float z = marlin_server.mesh.z[index];
                                uint32_t usr32 = variant8_flt(z).ui32;
                                uint16_t usr16 = x | ((uint16_t)y << 8);
                                if (_send_notify_event_to_client(client_id, queue, evt_id, usr32, usr16))
                                    marlin_server.mesh_point_notsent[client_id] &= ~mask;
                            }
                            mask <<= 1;
                        }
                    if (marlin_server.mesh_point_notsent[client_id] == 0)
                        sent |= msk; // event sent, set bit
                }
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
            //resend open close dialog (fsm)
            case MARLIN_EVT_FSM_Create:
                if (_send_notify_event_to_client(client_id, queue, evt_id, marlin_server.fsmCreate, 0))
                    sent |= msk; // event sent, set bit
                break;
            case MARLIN_EVT_FSM_Destroy:
                if (_send_notify_event_to_client(client_id, queue, evt_id, marlin_server.fsmDestroy, 0))
                    sent |= msk; // event sent, set bit
                break;
            case MARLIN_EVT_FSM_Change:
                if (_send_notify_event_to_client(client_id, queue, evt_id, marlin_server.fsmChange, 0))
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
                if (evt_id == MARLIN_EVT_MeshUpdate) {
                    uint8_t x = usr16 & 0xff;                      // x index
                    uint8_t y = usr16 >> 8;                        // y index
                    uint8_t index = x + marlin_server.mesh.xc * y; // index
                    uint64_t mask = ((uint64_t)1 << index);        // mask
                    marlin_server.mesh_point_notsent[client_id] |= mask;
                } else if (evt_id == MARLIN_EVT_FSM_Create)
                    marlin_server.fsmCreate = usr32;
                else if (evt_id == MARLIN_EVT_FSM_Destroy)
                    marlin_server.fsmDestroy = usr32;
                else if (evt_id == MARLIN_EVT_FSM_Change)
                    marlin_server.fsmChange = usr32;
            } else
                client_msk |= (1 << client_id);
        }
    return client_msk;
}

// send variable change notification to client (called from server thread)
static int _send_notify_change_to_client(osMessageQId queue, uint8_t var_id, variant8_t var) {
    var.usr8 = var_id | MARLIN_USR8_VAR_FLG;
    return _send_notify_to_client(queue, var);
}

// send variable change notification to client - multiple vars (called from server thread)
static uint64_t _send_notify_changes_to_client(int client_id, osMessageQId queue, uint64_t var_msk) {
    variant8_t var;
    uint64_t sent = 0;
    uint64_t msk = 1;
    for (uint8_t var_id = 0; var_id < 64; var_id++) {
        if (msk & var_msk) {
            var = marlin_vars_get_var(&(marlin_server.vars), var_id);
            if (var.type != VARIANT8_EMPTY) {
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
    if (marlin_server.gqueue != queue.length) {
        marlin_server.gqueue = queue.length;
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
    variant8_t v;
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
        v.ui8 = 0;
        if (stepper.axis_is_moving(X_AXIS))
            v.ui8 |= 0x01;
        if (stepper.axis_is_moving(Y_AXIS))
            v.ui8 |= 0x02;
        if (stepper.axis_is_moving(Z_AXIS))
            v.ui8 |= 0x04;
        if (stepper.axis_is_moving(E_AXIS))
            v.ui8 |= 0x08;
        if (marlin_server.vars.motion != v.ui8) {
            marlin_server.vars.motion = v.ui8;
            changes |= MARLIN_VAR_MSK(MARLIN_VAR_MOTION);
        }
    }

    if (update & MARLIN_VAR_MSK_IPOS_XYZE) {
        for (i = 0; i < 4; i++)
            if (update & MARLIN_VAR_MSK(MARLIN_VAR_IPOS_X + i)) {
                v.i32 = stepper.position((AxisEnum)i);
                if (marlin_server.vars.ipos[i] != v.i32) {
                    marlin_server.vars.ipos[i] = v.i32;
                    changes |= MARLIN_VAR_MSK(MARLIN_VAR_IPOS_X + i);
                }
            }
    }

    if (update & MARLIN_VAR_MSK_POS_XYZE) {
        for (i = 0; i < 4; i++)
            if (update & MARLIN_VAR_MSK(MARLIN_VAR_POS_X + i)) {
                v.flt = planner.get_axis_position_mm((AxisEnum)i);
                if (marlin_server.vars.pos[i] != v.flt) {
                    marlin_server.vars.pos[i] = v.flt;
                    changes |= MARLIN_VAR_MSK(MARLIN_VAR_POS_X + i);
                }
            }
    }

    if (update & MARLIN_VAR_MSK(MARLIN_VAR_TEMP_NOZ)) {
        v.flt = thermalManager.temp_hotend[0].celsius;
        if (marlin_server.vars.temp_nozzle != v.flt) {
            marlin_server.vars.temp_nozzle = v.flt;
            changes |= MARLIN_VAR_MSK(MARLIN_VAR_TEMP_NOZ);
        }
    }

    if (update & MARLIN_VAR_MSK(MARLIN_VAR_TTEM_NOZ)) {
        v.flt = thermalManager.temp_hotend[0].target;
        if (marlin_server.vars.target_nozzle != v.flt) {
            marlin_server.vars.target_nozzle = v.flt;
            changes |= MARLIN_VAR_MSK(MARLIN_VAR_TTEM_NOZ);
        }
    }

    if (update & MARLIN_VAR_MSK(MARLIN_VAR_TEMP_BED)) {
        v.flt = thermalManager.temp_bed.celsius;
        if (marlin_server.vars.temp_bed != v.flt) {
            marlin_server.vars.temp_bed = v.flt;
            changes |= MARLIN_VAR_MSK(MARLIN_VAR_TEMP_BED);
        }
    }

    if (update & MARLIN_VAR_MSK(MARLIN_VAR_TTEM_BED)) {
        v.flt = thermalManager.temp_bed.target;
        if (marlin_server.vars.target_bed != v.flt) {
            marlin_server.vars.target_bed = v.flt;
            changes |= MARLIN_VAR_MSK(MARLIN_VAR_TTEM_BED);
        }
    }

    if (update & MARLIN_VAR_MSK(MARLIN_VAR_Z_OFFSET)) {
        v.flt = probe_offset.z;
        if (marlin_server.vars.z_offset != v.flt) {
            marlin_server.vars.z_offset = v.flt;
            changes |= MARLIN_VAR_MSK(MARLIN_VAR_Z_OFFSET);
        }
    }

    if (update & MARLIN_VAR_MSK(MARLIN_VAR_FANSPEED)) {
#if FAN_COUNT > 0
        v.ui8 = thermalManager.fan_speed[0];
#endif
        if (marlin_server.vars.fan_speed != v.ui8) {
            marlin_server.vars.fan_speed = v.ui8;
            changes |= MARLIN_VAR_MSK(MARLIN_VAR_FANSPEED);
        }
    }

    if (update & MARLIN_VAR_MSK(MARLIN_VAR_PRNSPEED)) {
        v.ui16 = (unsigned)feedrate_percentage;
        if (marlin_server.vars.print_speed != v.ui16) {
            marlin_server.vars.print_speed = v.ui16;
            changes |= MARLIN_VAR_MSK(MARLIN_VAR_PRNSPEED);
        }
    }

    if (update & MARLIN_VAR_MSK(MARLIN_VAR_FLOWFACT)) {
        v.ui16 = (unsigned)planner.flow_percentage[0];
        if (marlin_server.vars.flow_factor != v.ui16) {
            marlin_server.vars.flow_factor = v.ui16;
            changes |= MARLIN_VAR_MSK(MARLIN_VAR_FLOWFACT);
        }
    }

    if (update & MARLIN_VAR_MSK(MARLIN_VAR_WAITHEAT)) {
        v.ui8 = wait_for_heatup ? 1 : 0;
        if (marlin_server.vars.wait_heat != v.ui8) {
            marlin_server.vars.wait_heat = v.ui8;
            changes |= MARLIN_VAR_MSK(MARLIN_VAR_WAITHEAT);
        }
    }

    if (update & MARLIN_VAR_MSK(MARLIN_VAR_WAITUSER)) {
        v.ui8 = wait_for_user ? 1 : 0;
        if (marlin_server.vars.wait_user != v.ui8) {
            marlin_server.vars.wait_user = v.ui8;
            changes |= MARLIN_VAR_MSK(MARLIN_VAR_WAITUSER);
        }
    }

    if (update & MARLIN_VAR_MSK(MARLIN_VAR_SD_PRINT)) {
        v.ui8 = (media_print_get_state() != media_print_state_NONE);
        if (marlin_server.vars.sd_printing != v.ui8) {
            marlin_server.vars.sd_printing = v.ui8;
            changes |= MARLIN_VAR_MSK(MARLIN_VAR_SD_PRINT);
        }
    }

    if (update & MARLIN_VAR_MSK(MARLIN_VAR_SD_PDONE)) {
        if (oProgressData.oPercentDone.mIsActual(marlin_server.vars.print_duration))
            v.ui8 = (uint8_t)oProgressData.oPercentDone.mGetValue();
        else
            v.ui8 = (uint8_t)media_print_get_percent_done();
        if (marlin_server.vars.sd_percent_done != v.ui8) {
            marlin_server.vars.sd_percent_done = v.ui8;
            changes |= MARLIN_VAR_MSK(MARLIN_VAR_SD_PDONE);
        }
    }

    if (update & MARLIN_VAR_MSK(MARLIN_VAR_DURATION)) {
        v.ui32 = print_job_timer.duration();
        if (marlin_server.vars.print_duration != v.ui32) {
            marlin_server.vars.print_duration = v.ui32;
            changes |= MARLIN_VAR_MSK(MARLIN_VAR_DURATION);
        }
    }

    if (update & MARLIN_VAR_MSK(MARLIN_VAR_MEDIAINS)) {
        v.ui8 = (media_get_state() == media_state_INSERTED) ? 1 : 0;
        if (marlin_server.vars.media_inserted != v.ui8) {
            marlin_server.vars.media_inserted = v.ui8;
            changes |= MARLIN_VAR_MSK(MARLIN_VAR_MEDIAINS);
            _send_notify_event(marlin_server.vars.media_inserted ? MARLIN_EVT_MediaInserted : MARLIN_EVT_MediaRemoved, 0, 0);
        }
    }

    if (update & MARLIN_VAR_MSK(MARLIN_VAR_PRNSTATE)) {
        v.ui8 = marlin_server.print_state;
        if (marlin_server.vars.print_state != v.ui8) {
            marlin_server.vars.print_state = (marlin_print_state_t)(v.ui8);
            changes |= MARLIN_VAR_MSK(MARLIN_VAR_PRNSTATE);
        }
    }

    if (update & MARLIN_VAR_MSK(MARLIN_VAR_TIMTOEND)) {
        if (oProgressData.oPercentDone.mIsActual(marlin_server.vars.print_duration))
            v.ui32 = oProgressData.oTime2End.mGetValue();
        else
            v.ui32 = -1;
        if (marlin_server.vars.time_to_end != v.ui32) {
            marlin_server.vars.time_to_end = v.ui32;
            changes |= MARLIN_VAR_MSK(MARLIN_VAR_TIMTOEND);
        }
    }

    return changes;
}

// process request on server side
static int _process_server_request(char *request) {
    int processed = 0;
    uint32_t msk32[2];
    float offs;
    int ival;
    int client_id = *(request++) - '0';
    if ((client_id < 0) || (client_id >= MARLIN_MAX_CLIENTS))
        return 1;
    DBG_REQ("SRV: REQ %c%s", '0' + client_id, request);
    if (strncmp("!g ", request, 3) == 0) {
        processed = marlin_server_enqueue_gcode(request + 3);
    } else if (strncmp("!ig ", request, sizeof("!ig ") / sizeof(char) - 1) == 0) {
        unsigned long int iptr = strtoul(request + sizeof("!ig ") / sizeof(char) - 1, NULL, 0);
        processed = marlin_server_inject_gcode((const char *)iptr);
    } else if (strcmp("!start", request) == 0) {
        marlin_server_start_processing();
        processed = 1;
    } else if (strcmp("!stop", request) == 0) {
        marlin_server_stop_processing();
        processed = 1;
    } else if (strncmp("!var ", request, 5) == 0) {
        _server_set_var(request + 5);
        processed = 1;
    } else if (sscanf(request, "!update %08lx %08lx", msk32 + 0, msk32 + 1)) {
        _server_update_and_notify(client_id, msk32[0] + (((uint64_t)msk32[1]) << 32));
        processed = 1;
    } else if (sscanf(request, "!babystep_Z %f", &offs) == 1) {
        marlin_server_do_babystep_Z(offs);
        processed = 1;
    } else if (strcmp("!cfg_save", request) == 0) {
        marlin_server_settings_save();
        processed = 1;
    } else if (strcmp("!cfg_load", request) == 0) {
        marlin_server_settings_load();
        processed = 1;
    } else if (strcmp("!cfg_reset", request) == 0) {
        marlin_server_settings_reset();
        processed = 1;
    } else if (strcmp("!updt", request) == 0) {
        marlin_server_manage_heater();
        processed = 1;
    } else if (strcmp("!qstop", request) == 0) {
        marlin_server_quick_stop();
        processed = 1;
    } else if (strncmp("!pstart ", request, 8) == 0) {
        marlin_server_print_start(request + 8);
        processed = 1;
    } else if (strcmp("!pabort", request) == 0) {
        marlin_server_print_abort();
        processed = 1;
    } else if (strcmp("!ppause", request) == 0) {
        marlin_server_print_pause();
        processed = 1;
    } else if (strcmp("!presume", request) == 0) {
        marlin_server_print_resume();
        processed = 1;
    } else if (strcmp("!park", request) == 0) {
        marlin_server_park_head();
        processed = 1;
    } else if (sscanf(request, "!fsm_r %d", &ival) == 1) { //finit state machine response
        ClientResponseHandler::SetResponse(ival);
        processed = 1;
    } else if (sscanf(request, "!event_msk %08lx %08lx", msk32 + 0, msk32 + 1)) {
        marlin_server.notify_events[client_id] = msk32[0] + (((uint64_t)msk32[1]) << 32);
        processed = 1;
    } else if (sscanf(request, "!change_msk %08lx %08lx", msk32 + 0, msk32 + 1)) {
        marlin_server.notify_changes[client_id] = msk32[0] + (((uint64_t)msk32[1]) << 32);
        marlin_server.client_changes[client_id] = msk32[0] + (((uint64_t)msk32[1]) << 32);
        processed = 1;
    } else {
        bsod("Unknown request %s", request);
    }
    if (processed)
        if (!_send_notify_event_to_client(client_id, marlin_client_queue[client_id], MARLIN_EVT_Acknowledge, 0, 0))
            marlin_server.notify_events[client_id] |= MARLIN_EVT_MSK(MARLIN_EVT_Acknowledge); // set bit if notification not sent
    return processed;
}

// set variable from string request
static int _server_set_var(char *name_val_str) {
    int var_id;
    bool changed = false;
    char *val_str = strchr(name_val_str, ' ');
    *(val_str++) = 0;
    if ((var_id = marlin_vars_get_id_by_name(name_val_str)) >= 0) {
        if (marlin_vars_str_to_value(&(marlin_server.vars), var_id, val_str) == 1) {
            switch (var_id) {
            case MARLIN_VAR_TTEM_NOZ:
                changed = (thermalManager.temp_hotend[0].target != marlin_server.vars.target_nozzle);
                thermalManager.setTargetHotend(marlin_server.vars.target_nozzle, 0);
                break;
            case MARLIN_VAR_TTEM_BED:
                changed = (thermalManager.temp_bed.target != marlin_server.vars.target_bed);
                thermalManager.setTargetBed(marlin_server.vars.target_bed);
                break;
            case MARLIN_VAR_Z_OFFSET:
#if HAS_BED_PROBE
                changed = (probe_offset.z != marlin_server.vars.z_offset);
                probe_offset.z = marlin_server.vars.z_offset;
#endif //HAS_BED_PROBE
                break;
            case MARLIN_VAR_FANSPEED:
#if FAN_COUNT > 0
                changed = (thermalManager.fan_speed[0] != marlin_server.vars.fan_speed);
                thermalManager.set_fan_speed(0, marlin_server.vars.fan_speed);
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
            }
            if (changed) {
                int client_id;
                uint64_t var_msk = MARLIN_VAR_MSK(var_id);
                for (client_id = 0; client_id < MARLIN_MAX_CLIENTS; client_id++)
                    marlin_server.client_changes[client_id] |= (var_msk & marlin_server.notify_changes[client_id]);
            }
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
    DBG_XUI("XUI: onStartup");
    _send_notify_event(MARLIN_EVT_Startup, 0, 0);
}

void onIdle() {
    marlin_server_idle();
    if (marlin_server_idle_cb)
        marlin_server_idle_cb();
}

//todo remove me after new thermal manager
int _is_thermal_error(PGM_P const msg) {
    if (!strcmp(msg, GET_TEXT(MSG_HEATING_FAILED_LCD)))
        return 1;
    if (!strcmp(msg, GET_TEXT(MSG_HEATING_FAILED_LCD_BED)))
        return 1;
    if (!strcmp(msg, GET_TEXT(MSG_HEATING_FAILED_LCD_CHAMBER)))
        return 1;
    if (!strcmp(msg, GET_TEXT(MSG_ERR_REDUNDANT_TEMP)))
        return 1;
    if (!strcmp(msg, GET_TEXT(MSG_THERMAL_RUNAWAY)))
        return 1;
    if (!strcmp(msg, GET_TEXT(MSG_THERMAL_RUNAWAY_BED)))
        return 1;
    if (!strcmp(msg, GET_TEXT(MSG_THERMAL_RUNAWAY_CHAMBER)))
        return 1;
    if (!strcmp(msg, GET_TEXT(MSG_ERR_MAXTEMP)))
        return 1;
    if (!strcmp(msg, GET_TEXT(MSG_ERR_MINTEMP)))
        return 1;
    if (!strcmp(msg, GET_TEXT(MSG_ERR_MAXTEMP_BED)))
        return 1;
    if (!strcmp(msg, GET_TEXT(MSG_ERR_MINTEMP_BED)))
        return 1;
    return 0;
}

void onPrinterKilled(PGM_P const msg, PGM_P const component) {
    //_dbg("onPrinterKilled %s", msg);
    if (!(SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk))
        taskENTER_CRITICAL();     //never exit CRITICAL, wanted to use __disable_irq, but it does not work. i do not know why
    wdt_iwdg_refresh();           //watchdog reset
    if (_is_thermal_error(msg)) { //todo remove me after new thermal manager
        const marlin_vars_t &vars = marlin_server.vars;
        temp_error(msg, component, vars.temp_nozzle, vars.target_nozzle, vars.temp_bed, vars.target_bed);
    } else {
        general_error(msg, component);
    }
}

void onMediaInserted() {
    DBG_XUI("XUI: onMediaInserted");
    _send_notify_event(MARLIN_EVT_MediaInserted, 0, 0);
}

void onMediaError() {
    DBG_XUI("XUI: onMediaError");
    _send_notify_event(MARLIN_EVT_MediaError, 0, 0);
}

void onMediaRemoved() {
    DBG_XUI("XUI: onMediaRemoved");
    _send_notify_event(MARLIN_EVT_MediaRemoved, 0, 0);
}

void onPlayTone(const uint16_t frequency, const uint16_t duration) {
    DBG_XUI("XUI: onPlayTone");
    _send_notify_event(MARLIN_EVT_PlayTone, frequency, duration);
}

void onPrintTimerStarted() {
    DBG_XUI("XUI: onPrintTimerStarted");
    _send_notify_event(MARLIN_EVT_PrintTimerStarted, 0, 0);
}

void onPrintTimerPaused() {
    DBG_XUI("XUI: onPrintTimerPaused");
    _send_notify_event(MARLIN_EVT_PrintTimerPaused, 0, 0);
}

void onPrintTimerStopped() {
    DBG_XUI("XUI: onPrintTimerStopped");
    _send_notify_event(MARLIN_EVT_PrintTimerStopped, 0, 0);
}

void onFilamentRunout(const extruder_t extruder) {
    DBG_XUI("XUI: onFilamentRunout");
    _send_notify_event(MARLIN_EVT_FilamentRunout, 0, 0);
}

void onUserConfirmRequired(const char *const msg) {
    DBG_XUI("XUI: onUserConfirmRequired: %s", msg);
    _send_notify_event(MARLIN_EVT_UserConfirmRequired, 0, 0);
}

void onStatusChanged(const char *const msg) {
    static bool pending_err_msg = false;

    DBG_XUI("XUI: onStatusChanged: %s", msg);
    _send_notify_event(MARLIN_EVT_StatusChanged, 0, 0);
    if (strcmp(msg, "Prusa-mini Ready.") == 0) {
    } //TODO
    else if (strcmp(msg, "TMC CONNECTION ERROR") == 0)
        _send_notify_event(MARLIN_EVT_Error, MARLIN_ERR_TMCDriverError, 0);
    else {
        if (!is_abort_state(marlin_server.print_state))
            pending_err_msg = false;
        if (!pending_err_msg) {
            if (strcmp(msg, MSG_ERR_PROBING_FAILED) == 0) {
                _send_notify_event(MARLIN_EVT_Error, MARLIN_ERR_ProbingFailed, 0);
                marlin_server_print_abort();
                pending_err_msg = true;
            }

            if (msg && msg[0] != 0) { //empty message filter

                _add_status_msg(msg);
                _send_notify_event(MARLIN_EVT_Message, 0, 0);
            }
        }
    }
}

void onFactoryReset() {
    DBG_XUI("XUI: onFactoryReset");
    _send_notify_event(MARLIN_EVT_FactoryReset, 0, 0);
}

void onLoadSettings(char const *) {
    DBG_XUI("XUI: onLoadSettings");
    _send_notify_event(MARLIN_EVT_LoadSettings, 0, 0);
}

void onStoreSettings(char *) {
    DBG_XUI("XUI: onStoreSettings");
    _send_notify_event(MARLIN_EVT_StoreSettings, 0, 0);
}

void onConfigurationStoreWritten(bool success) {
    DBG_XUI("XUI: onConfigurationStoreWritten");
}

void onConfigurationStoreRead(bool success) {
    DBG_XUI("XUI: onConfigurationStoreRead");
}

void onMeshUpdate(const uint8_t xpos, const uint8_t ypos, const float zval) {
    DBG_XUI("XUI: onMeshUpdate x: %u, y: %u, z: %.2f", xpos, ypos, (double)zval);
    uint8_t index = xpos + marlin_server.mesh.xc * ypos;
    uint32_t usr32 = variant8_flt(zval).ui32;
    uint16_t usr16 = xpos | ((uint16_t)ypos << 8);
    marlin_server.mesh.z[index] = zval;
    _send_notify_event(MARLIN_EVT_MeshUpdate, usr32, usr16);
}

}

//remember last event
static uint32_t fsm_change_last_usr32 = -1;

//must match fsm_create_t signature
void fsm_create(ClientFSM type, uint8_t data) {
    //erase info about last event
    fsm_change_last_usr32 = -1;

    uint32_t usr32 = uint32_t(type) + (uint32_t(data) << 8);
    DBG_FSM("fsm_create %d", usr32);

    const MARLIN_EVT_t evt_id = MARLIN_EVT_FSM_Create;
    _send_notify_event(evt_id, usr32, 0);
}

//must match fsm_destroy_t signature
void fsm_destroy(ClientFSM type) {
    DBG_FSM("fsm_destroy %d", (int)type);

    const MARLIN_EVT_t evt_id = MARLIN_EVT_FSM_Destroy;
    _send_notify_event(evt_id, uint32_t(type), 0);
}

//must match fsm_change_t signature
void _fsm_change(ClientFSM type, uint8_t phase, uint8_t progress_tot, uint8_t progress) {
    uint32_t usr32 = uint32_t(type) + (uint32_t(phase) << 8) + (uint32_t(progress_tot) << 16) + (uint32_t(progress) << 24);
    if (usr32 == uint32_t(-1))
        bsod("FATAL Invalid Event");
    if (usr32 == fsm_change_last_usr32)
        return;
    fsm_change_last_usr32 = usr32;
    DBG_FSM("fsm_change %d", usr32);
    const MARLIN_EVT_t evt_id = MARLIN_EVT_FSM_Change;
    _send_notify_event(evt_id, usr32, 0);
}

/*****************************************************************************/
//FSM_notifier
FSM_notifier::data FSM_notifier::s_data;

FSM_notifier::FSM_notifier(ClientFSM type, uint8_t phase, cvariant8 min, cvariant8 max,
    uint8_t progress_min, uint8_t progress_max, uint8_t var_id)
    : temp_data(s_data) {
    s_data.type = type;
    s_data.phase = phase;
    s_data.scale = static_cast<float>(progress_max - progress_min) / static_cast<float>(max - min);
    s_data.offset = -static_cast<float>(min) * s_data.scale + static_cast<float>(progress_min);
    s_data.progress_min = progress_min;
    s_data.progress_max = progress_max;
    s_data.var_id = var_id;
    s_data.last_progress_sent = -1;
}

//static method
//notifies clients about progress rise
//scales "binded" variable via following formula to calculate progress
//x = (actual - s_data.min) * s_data.scale + s_data.progress_min;
//x = actual * s_data.scale - s_data.min * s_data.scale + s_data.progress_min;
//s_data.offset == -s_data.min * s_data.scale + s_data.progress_min
//simplified formula
//x = actual * s_data.scale + s_data.offset;
void FSM_notifier::SendNotification() {
    if (s_data.type == ClientFSM::_none)
        return;

    cvariant8 temp;
    temp.attach(marlin_vars_get_var(&(marlin_server.vars), s_data.var_id));

    float actual = static_cast<float>(temp);
    actual = actual * s_data.scale + s_data.offset;

    int progress = static_cast<int>(actual); //int - must be signed
    if (progress < s_data.progress_min)
        progress = s_data.progress_min;
    if (progress > s_data.progress_max)
        progress = s_data.progress_max;

    // after first sent, progress can only rise
    if ((s_data.last_progress_sent == uint8_t(-1)) || (progress > s_data.last_progress_sent)) {
        s_data.last_progress_sent = progress;
        _fsm_change(s_data.type, s_data.phase, progress, 0);
    }
}

FSM_notifier::~FSM_notifier() {
    s_data = temp_data;
}

/*****************************************************************************/
//ClientResponseHandler
//define static member
//-1 (maxval) is used as no response from client
uint32_t ClientResponseHandler::server_side_encoded_response = -1;
