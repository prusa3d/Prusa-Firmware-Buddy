// marlin_server.cpp

#include "marlin_server.h"
#include <stdarg.h>
#include "dbg.h"
#include "app.h"
#include "bsod.h"
#include "cmsis_os.h"
#include "string.h" //strncmp

#include "../Marlin/src/lcd/extensible_ui/ui_api.h"
#include "../Marlin/src/gcode/queue.h"
#include "../Marlin/src/gcode/parser.h"
#include "../Marlin/src/module/planner.h"
#include "../Marlin/src/module/stepper.h"
#include "../Marlin/src/module/temperature.h"
#include "../Marlin/src/module/probe.h"
#include "../Marlin/src/module/configuration_store.h"
#include "../Marlin/src/module/printcounter.h"
#include "../Marlin/src/feature/host_actions.h"
#include "../Marlin/src/feature/babystep.h"
#include "../Marlin/src/feature/pause.h"
#include "../Marlin/src/sd/cardreader.h"
#include "../Marlin/src/libs/nozzle.h"
#include "../Marlin/src/core/language.h" //GET_TEXT(MSG)

#include "hwio_a3ides.h"
#include "eeprom.h"

#ifdef LCDSIM
    #include "lcdsim.h"
#endif // LCDSIM

#define DBG _dbg1 //enabled level 1
//#define DBG(...)

#define DBG_XUI DBG //trace ExtUI events
//#define DBG_XUI(...)    //disable trace

//#define DBG_REQ  DBG    //trace requests
#define DBG_REQ(...) //disable trace

#define DBG_HOST DBG //trace host_actions
//#define DBG_HOST(...)   //disable trace

// host_actions definitions
#define HOST_PROMPT_LEN_MAX 32 // max 32 chars prompt text
#define HOST_BUTTON_LEN_MAX 16 // max 16 chars button text
#define HOST_BUTTON_CNT_MAX 4 // max 4 buttons

#pragma pack(push)
#pragma pack(1)

typedef struct _marlin_server_t {
    uint16_t flags; // server flags (MARLIN_SFLG)
    uint64_t notify_events; // event notification mask
    uint64_t notify_changes; // variable change notification mask
    marlin_vars_t vars; // cached variables
    char request[MARLIN_MAX_REQUEST];
    int request_len;
    uint64_t client_events[MARLIN_MAX_CLIENTS]; // client event mask
    uint64_t client_changes[MARLIN_MAX_CLIENTS]; // client variable change mask
    variant8_t client_events_notsent[MARLIN_MAX_CLIENTS][4]; // buffer for events that are not sent
    uint32_t last_update; // last update tick count
    uint8_t idle_cnt; // idle call counter
    uint8_t pqueue_head; // copy of planner.block_buffer_head
    uint8_t pqueue_tail; // copy of planner.block_buffer_tail
    uint8_t pqueue; // calculated number of records in planner queue
    uint8_t gqueue; // copy of queue.length - number of commands in gcode queue
    uint32_t command; // actually running command
    marlin_mesh_t mesh; // meshbed leveling
    uint64_t mesh_point_notsent[MARLIN_MAX_CLIENTS]; // mesh point mask (points that are not sent)
} marlin_server_t;

#pragma pack(pop)

PromptReason host_prompt_reason = PROMPT_NOT_DEFINED;

extern "C" {

#ifndef _DEBUG
extern IWDG_HandleTypeDef hiwdg; //watchdog handle
#endif //_DEBUG

//-----------------------------------------------------------------------------
// variables

osThreadId marlin_server_task = 0; // task handle
osMessageQId marlin_server_queue = 0; // input queue (uint8_t)
osSemaphoreId marlin_server_sema = 0; // semaphore handle

marlin_server_t marlin_server; // server structure - initialize task to zero

marlin_server_idle_t *marlin_server_idle_cb = 0; // idle callback

//==========MSG_STACK===================
//	top of the stack is at [0]

msg_stack_t msg_stack = { '\0', 0 };

void _add_status_msg(const char *const popup_msg) {
    char message[MSG_MAX_LENGTH];
    size_t str_size = strlen(popup_msg);
    if (str_size >= MSG_MAX_LENGTH - 1) {
        strncpy(message, popup_msg, MSG_MAX_LENGTH - 1); // popup_msg is not always null-terminated...
        message[MSG_MAX_LENGTH - 1] = '\0';
    } else {
        strncpy(message, popup_msg, str_size);
        message[str_size] = '\0';
    }

    for (uint8_t i = msg_stack.count; i; i--) {
        if (i == MSG_STACK_SIZE)
            i--; // last place of the limited stack will be always overwritten
        strncpy(msg_stack.msg_data[i], msg_stack.msg_data[i - 1], MSG_MAX_LENGTH);
    }

    strncpy(msg_stack.msg_data[0], message, MSG_MAX_LENGTH);

    if (msg_stack.count < MSG_STACK_SIZE)
        msg_stack.count++;
}

// host_actions vars
char host_prompt[HOST_PROMPT_LEN_MAX] = "";
char host_prompt_button[HOST_BUTTON_CNT_MAX][HOST_PROMPT_LEN_MAX];
uint8_t host_prompt_buttons = 0;
host_prompt_button_t host_prompt_button_clicked = HOST_PROMPT_BTN_None;

//-----------------------------------------------------------------------------
// external variables from marlin_client

extern osThreadId marlin_client_task[MARLIN_MAX_CLIENTS]; // task handles
extern osMessageQId marlin_client_queue[MARLIN_MAX_CLIENTS]; // input queue handles (uint32_t)

//-----------------------------------------------------------------------------
// forward declarations of private functions

int _send_notify_to_client(osMessageQId queue, variant8_t msg);
int _send_notify_event_to_client(int client_id, osMessageQId queue, uint8_t evt_id, uint32_t usr32, uint16_t usr16);
uint64_t _send_notify_events_to_client(int client_id, osMessageQId queue, uint64_t evt_msk);
void _send_notify_event(uint8_t evt_id, uint32_t usr32, uint16_t usr16);
int _send_notify_change_to_client(osMessageQId queue, uint8_t var_id, variant8_t var);
uint64_t _send_notify_changes_to_client(int client_id, osMessageQId queue, uint64_t var_msk);
void _server_update_gqueue(void);
void _server_update_pqueue(void);
uint64_t _server_update_vars(uint64_t force_update_msk);
int _process_server_request(char *request);
int _server_set_var(char *name_val_str);

//-----------------------------------------------------------------------------
// server side functions

void marlin_server_init(void) {
    memset(&marlin_server, 0, sizeof(marlin_server_t));
    osMessageQDef(serverQueue, 64, uint8_t);
    marlin_server_queue = osMessageCreate(osMessageQ(serverQueue), NULL);
    osSemaphoreDef(serverSema);
    marlin_server_sema = osSemaphoreCreate(osSemaphore(serverSema), 1);
    marlin_server.flags = MARLIN_SFLG_PROCESS | MARLIN_SFLG_STARTED;
    marlin_server.notify_events = MARLIN_EVT_MSK_DEF;
    marlin_server.notify_changes = MARLIN_VAR_MSK_DEF;
    marlin_server_task = osThreadGetId();
    marlin_server.mesh.xc = 4;
    marlin_server.mesh.yc = 4;
}

int marlin_server_cycle(void) {
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
        changes = _server_update_vars(marlin_server.notify_changes);
    }
    // send notifications
    for (client_id = 0; client_id < MARLIN_MAX_CLIENTS; client_id++)
        if ((queue = marlin_client_queue[client_id]) != 0) {
            marlin_server.client_changes[client_id] |= changes;
            // send change notifications, clear bits for successful sent notification
            if ((msk = marlin_server.client_changes[client_id]) != 0)
                marlin_server.client_changes[client_id] &= ~_send_notify_changes_to_client(client_id, queue, msk);
            // send events, clear bits for successful sent notification
            if ((msk = marlin_server.client_events[client_id]) != 0)
                marlin_server.client_events[client_id] &= ~_send_notify_events_to_client(client_id, queue, msk);
        }
#ifndef _DEBUG
    if ((marlin_server.flags & MARLIN_SFLG_PROCESS) == 0)
        HAL_IWDG_Refresh(&hiwdg); // this prevents iwdg reset while processing disabled
#endif //_DEBUG
    return count;
}

#define MARLIN_IDLE_CNT_BUSY 2

int marlin_server_loop(void) {
    if (marlin_server.idle_cnt >= MARLIN_IDLE_CNT_BUSY)
        if (marlin_server.flags & MARLIN_SFLG_BUSY) {
            //_dbg("SVR: READY");
            marlin_server.flags &= ~MARLIN_SFLG_BUSY;
            _send_notify_event(MARLIN_EVT_Ready, 0, 0);
            if (marlin_server.command != MARLIN_CMD_NONE) {
                _send_notify_event(MARLIN_EVT_CommandEnd, marlin_server.command, 0);
                marlin_server.command = MARLIN_CMD_NONE;
            }
        }
    marlin_server.idle_cnt = 0;
    return marlin_server_cycle();
}

int marlin_server_idle(void) {
    if (marlin_server.idle_cnt < MARLIN_IDLE_CNT_BUSY)
        marlin_server.idle_cnt++;
    else if ((marlin_server.flags & MARLIN_SFLG_BUSY) == 0) {
        //_dbg("SVR: BUSY");
        marlin_server.flags |= MARLIN_SFLG_BUSY;
        _send_notify_event(MARLIN_EVT_Busy, 0, 0);
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
            case 600:
            case 701:
            case 702:
                marlin_server.command = MARLIN_CMD_M + parser.codenum;
                break;
            }
        if (marlin_server.command != MARLIN_CMD_NONE)
            _send_notify_event(MARLIN_EVT_CommandBegin, marlin_server.command, 0);
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

void marlin_server_update(uint64_t update) {
    int client_id;
    osMessageQId queue;
    _server_update_vars(update);
    for (client_id = 0; client_id < MARLIN_MAX_CLIENTS; client_id++)
        if ((queue = marlin_client_queue[client_id]) != 0)
            marlin_server.client_changes[client_id] &= ~_send_notify_changes_to_client(client_id, queue, update);
    //	_dbg0("UPDATE");
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
    (void)settings.save();
}

void marlin_server_settings_load(void) {
    (void)settings.load();
}

void marlin_server_manage_heater(void) {
    thermalManager.manage_heater();
}

void marlin_server_quick_stop(void) {
    planner.quick_stop();
}

void marlin_server_print_abort(void) {
    wait_for_heatup = wait_for_user = false;
    card.flag.abort_sd_printing = true;
    print_job_timer.stop();
    //	planner.quick_stop();
    //	marlin_server_park_head();
    //	planner.synchronize();
    //	queue.inject_P("M125");
}

void marlin_server_print_pause(void) {
    card.pauseSDPrint();
    print_job_timer.pause();
    queue.inject_P("M125");
}

void marlin_server_print_resume(void) {
    wait_for_user = false;
    host_prompt_button_clicked = HOST_PROMPT_BTN_Continue;
    queue.inject_P("M24");
}

void marlin_server_park_head(void) {
    //homed check
    if (all_axes_homed() && all_axes_known()) {
        xyz_pos_t park_point = NOZZLE_PARK_POINT;
        nozzle.park(2, park_point);
    }
}

int marlin_all_axes_homed(void) {
    return all_axes_homed() ? 1 : 0;
}

int marlin_all_axes_known(void) {
    return all_axes_known() ? 1 : 0;
}

//-----------------------------------------------------------------------------
// private functions

// send notify message (variant8_t) to client queue (called from server thread)
int _send_notify_to_client(osMessageQId queue, variant8_t msg) {
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
int _send_notify_event_to_client(int client_id, osMessageQId queue, uint8_t evt_id, uint32_t usr32, uint16_t usr16) {
    variant8_t msg;
    msg = variant8_user(usr32);
    msg.usr16 = usr16;
    msg.usr8 = evt_id;
    return _send_notify_to_client(queue, msg);
}

// send event notification to client - multiple events (called from server thread)
// returns mask of succesfull sent events
uint64_t _send_notify_events_to_client(int client_id, osMessageQId queue, uint64_t evt_msk) {
    uint64_t sent = 0;
    uint64_t msk = 1;
    for (uint8_t evt_id = 0; evt_id < 64; evt_id++) {
        if (msk & evt_msk)
            switch (evt_id) {
            // Idle and PrinterKilled events not used
            //case MARLIN_EVT_Idle:
            //case MARLIN_EVT_PrinterKilled:
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
            case MARLIN_EVT_Busy:
            case MARLIN_EVT_Ready:
                if (_send_notify_event_to_client(client_id, queue, evt_id, 0, 0))
                    sent |= msk; // event sent, set bit
                break;
            //case MARLIN_EVT_Error:
            // StatusChanged event - one string argument
            case MARLIN_EVT_StatusChanged:
                if (_send_notify_event_to_client(client_id, queue, evt_id, 0, 0))
                    sent |= msk; // event sent, set bit
                break;
            // CommandBegin/End - one ui32 argument (CMD)
            //case MARLIN_EVT_CommandBegin:
            //case MARLIN_EVT_CommandEnd:
            //	if (_send_notify_event_to_client(client_id, queue, evt_id, 0, 0))
            //		sent |= msk; // event sent, set bit
            //	break;
            //case MARLIN_EVT_PlayTone:
            //case MARLIN_EVT_UserConfirmRequired:
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
            }
        msk <<= 1;
    }
    return sent;
}

// send event notification to all clients (called from server thread)
void _send_notify_event(uint8_t evt_id, uint32_t usr32, uint16_t usr16) {
    if ((marlin_server.notify_events & ((uint64_t)1 << evt_id)) == 0)
        return;
    for (int client_id = 0; client_id < MARLIN_MAX_CLIENTS; client_id++)
        if (_send_notify_event_to_client(client_id, marlin_client_queue[client_id], evt_id, usr32, usr16) == 0) {
            marlin_server.client_events[client_id] |= ((uint64_t)1 << evt_id); // event not sent, set bit
            if (evt_id == MARLIN_EVT_MeshUpdate) {
                uint8_t x = usr16 & 0xff; // x index
                uint8_t y = usr16 >> 8; // y index
                uint8_t index = x + marlin_server.mesh.xc * y; // index
                uint64_t mask = ((uint64_t)1 << index); // mask
                marlin_server.mesh_point_notsent[client_id] |= mask;
            }
        }
}

// send variable change notification to client (called from server thread)
int _send_notify_change_to_client(osMessageQId queue, uint8_t var_id, variant8_t var) {
    var.usr8 = var_id | MARLIN_USR8_VAR_FLG;
    return _send_notify_to_client(queue, var);
}

// send variable change notification to client - multiple vars (called from server thread)
uint64_t _send_notify_changes_to_client(int client_id, osMessageQId queue, uint64_t var_msk) {
    variant8_t var;
    uint64_t sent = 0;
    uint64_t msk = 1;
    for (uint8_t var_id = 0; var_id < 64; var_id++) {
        if (msk & var_msk) {
            var = marlin_vars_get_var(&(marlin_server.vars), var_id);
            if (var.type != VARIANT8_EMPTY)
                if (_send_notify_change_to_client(queue, var_id, var))
                    sent |= msk;
        }
        msk <<= 1;
    }
    return sent;
}

void _server_update_gqueue(void) {
    if (marlin_server.gqueue != queue.length) {
        marlin_server.gqueue = queue.length;
        //		_dbg("gqueue: %2d", marlin_server.gqueue);
    }
}

void _server_update_pqueue(void) {
    if ((marlin_server.pqueue_head != planner.block_buffer_head) || (marlin_server.pqueue_tail != planner.block_buffer_tail)) {
        marlin_server.pqueue_head = planner.block_buffer_head;
        marlin_server.pqueue_tail = planner.block_buffer_tail;
        marlin_server.pqueue = (marlin_server.pqueue_head >= marlin_server.pqueue_tail) ? (marlin_server.pqueue_head - marlin_server.pqueue_tail) : (BLOCK_BUFFER_SIZE + marlin_server.pqueue_head - marlin_server.pqueue_tail);
        //		_dbg("pqueue: %2d", marlin_server.pqueue);
    }
}

// update server variables defined by 'update', returns changed variables mask (called from server thread)
uint64_t _server_update_vars(uint64_t update) {
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
        v.ui8 = thermalManager.fan_speed[0];
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
        v.ui8 = card.flag.sdprinting ? 1 : 0;
        if (marlin_server.vars.sd_printing != v.ui8) {
            marlin_server.vars.sd_printing = v.ui8;
            changes |= MARLIN_VAR_MSK(MARLIN_VAR_SD_PRINT);
        }
    }

    if (update & MARLIN_VAR_MSK(MARLIN_VAR_SD_PDONE)) {
        v.ui8 = card.percentDone();
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
    return changes;
}

// process request on server side
int _process_server_request(char *request) {
    int processed = 0;
    //uint64_t msk;
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
        marlin_server_update(msk32[0] + (((uint64_t)msk32[1]) << 32));
        processed = 1;
    } else if (sscanf(request, "!babystep_Z %f", &offs) == 1) {
        marlin_server_do_babystep_Z(offs);
        processed = 1;
    } else if (strcmp("!save", request) == 0) {
        marlin_server_settings_save();
        processed = 1;
    } else if (strcmp("!load", request) == 0) {
        marlin_server_settings_load();
        processed = 1;
    } else if (strcmp("!updt", request) == 0) {
        marlin_server_manage_heater();
        processed = 1;
    } else if (strcmp("!qstop", request) == 0) {
        marlin_server_quick_stop();
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
    } else if (sscanf(request, "!hclick %d", &ival) == 1) {
        host_prompt_button_clicked = (host_prompt_button_t)ival;
        processed = 1;
    }
    if (processed)
        _send_notify_event_to_client(client_id, marlin_client_queue[client_id], MARLIN_EVT_Acknowledge, 0, 0);
    return processed;
}

// set variable from string request
int _server_set_var(char *name_val_str) {
    int var_id;
    char *val_str = strchr(name_val_str, ' ');
    *(val_str++) = 0;
    if ((var_id = marlin_vars_get_id_by_name(name_val_str)) >= 0) {
        if (marlin_vars_str_to_value(&(marlin_server.vars), var_id, val_str) == 1) {
            switch (var_id) {
            case MARLIN_VAR_TTEM_NOZ:
                thermalManager.setTargetHotend(marlin_server.vars.target_nozzle, 0);
                break;
            case MARLIN_VAR_TTEM_BED:
                thermalManager.setTargetBed(marlin_server.vars.target_bed);
                break;
            case MARLIN_VAR_Z_OFFSET:
                probe_offset.z = marlin_server.vars.z_offset;
                break;
            case MARLIN_VAR_FANSPEED:
                thermalManager.set_fan_speed(0, marlin_server.vars.fan_speed);
                break;
            case MARLIN_VAR_PRNSPEED:
                feedrate_percentage = (int16_t)marlin_server.vars.print_speed;
                break;
            case MARLIN_VAR_FLOWFACT:
                planner.flow_percentage[0] = (int16_t)marlin_server.vars.flow_factor;
                planner.refresh_e_factor(0);
                break;
            case MARLIN_VAR_WAITHEAT:
                wait_for_heatup = marlin_server.vars.wait_heat ? true : false;
                break;
            case MARLIN_VAR_WAITUSER:
                wait_for_user = marlin_server.vars.wait_user ? true : false;
                break;
            }
        }
    }
    //	_dbg("_server_set_var %d %s %s", var_id, name_val_str, val_str);
    return 1;
}

} // extern "C"

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
    DBG_XUI("XUI: onStatusChanged: %s", msg);
    _send_notify_event(MARLIN_EVT_StatusChanged, 0, 0);
    if (strcmp(msg, "Prusa-mini Ready.") == 0) {
    } //TODO
    else if (strcmp(msg, "TMC CONNECTION ERROR") == 0)
        _send_notify_event(MARLIN_EVT_Error, MARLIN_ERR_TMCDriverError, 0);
    else if (strcmp(msg, MSG_ERR_PROBING_FAILED) == 0)
        _send_notify_event(MARLIN_EVT_Error, MARLIN_ERR_ProbingFailed, 0);
    else {
        if (msg && msg[0] != 0) { //empty message filter

            _add_status_msg(msg);
            _send_notify_event(MARLIN_EVT_Message, 0, 0);
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

//-----------------------------------------------------------------------------
// host_actions callbacks

void host_action(const char *const pstr, const bool eol) {
}

//Marlin kill handler
// message reported to serial: MSG_ERR_KILLED "Printer halted. kill() called!"
// known kill calls:
//  kill()
//   Marlin/src/gcode/control/M108_M112_M410.cpp, line 44
//   Marlin/src/gcode/queue.cpp, line 666
//   Marlin/src/module/temperature.cpp, line 921
//   Marlin/src/module/temperature.cpp, line 1141
//   Marlin/src/Marlin.cpp, line 446
//   Marlin/src/Marlin.cpp, line 514
//  kill("Driver error")
//   Marlin/src/feature/tmc_util.cpp, line 215
//  kill(PSTR(MSG_ERR_PROBING_FAILED)) "Probing failed"
//   Marlin/src/Marlin.cpp, line 401
//  kill(PSTR(MSG_ERR_HOMING_FAILED)) "Homing failed"
//   Marlin/src/module/endstops.cpp, line 290
//  _temp_error(heater, PSTR(MSG_T_HEATING_FAILED), TEMP_ERR_PSTR(MSG_HEATING_FAILED_LCD, heater)) "Heating failed"
//   Marlin/src/module/temperature.cpp, line 514
//  _temp_error(e, PSTR(MSG_T_HEATING_FAILED), TEMP_ERR_PSTR(MSG_HEATING_FAILED_LCD, e)) "Heating failed"
//   Marlin/src/module/temperature.cpp, line 959
//  _temp_error(-1, PSTR(MSG_T_HEATING_FAILED), TEMP_ERR_PSTR(MSG_HEATING_FAILED_LCD, -1)) "Heating failed"
//   Marlin/src/module/temperature.cpp, line 999
//  _temp_error(-2, PSTR(MSG_T_HEATING_FAILED), TEMP_ERR_PSTR(MSG_HEATING_FAILED_LCD, -2)) "Heating failed"
//   Marlin/src/module/temperature.cpp, line 1069
//  _temp_error(heater, PSTR(MSG_T_THERMAL_RUNAWAY), TEMP_ERR_PSTR(MSG_THERMAL_RUNAWAY, heater)) "Thermal Runaway"
//   Marlin/src/module/temperature.cpp, line 517
//  _temp_error(heater_id, PSTR(MSG_T_THERMAL_RUNAWAY), TEMP_ERR_PSTR(MSG_THERMAL_RUNAWAY, heater_id)) "Thermal Runaway"
//   Marlin/src/module/temperature.cpp, line 1914
//  _temp_error(heater, PSTR(MSG_T_MAXTEMP), TEMP_ERR_PSTR(MSG_ERR_MAXTEMP, heater)) "MAXTEMP triggered"
//   Marlin/src/module/temperature.cpp, line 732
//  _temp_error(heater, PSTR(MSG_T_MINTEMP), TEMP_ERR_PSTR(MSG_ERR_MINTEMP, heater)) "MINTEMP triggered"
//   Marlin/src/module/temperature.cpp, line 736
//  _temp_error(0, PSTR(MSG_REDUNDANCY), PSTR(MSG_ERR_REDUNDANT_TEMP)) "Heater switched off. Temperature difference between temp sensors is too high !"
//   Marlin/src/module/temperature.cpp, line 968
void host_action_kill() {
#ifdef LCDSIM
    char text[85]; //max 4 lines of 20 chars + 4x '\n' + '\x00'
    lcdsim_grab_text(text); //grab text from display buffer
    bsod(text); //BSOD (endless loop with disabled interrupts)
#endif // LCDSIM
}

void host_action_safety_timer_expired() {
    DBG_HOST("host_action_safety_timer_expired");
    _send_notify_event(MARLIN_EVT_SafetyTimerExpired, 0, 0);
}

void host_action_pause(const bool eol /*=true*/) {
    DBG_HOST("host_action_pause");
}

void host_action_resume() {
    DBG_HOST("host_action_resume");
}

void host_action_cancel() {
    DBG_HOST("host_action_cancel");
}

void host_action_paused(const bool eol /*=true*/) {
    DBG_HOST("host_action_paused");
}

void host_action_resumed() {
    DBG_HOST("host_action_resumed");
}

void host_response_handler(const uint8_t response) {
    DBG_HOST("host_response_handler %d", (int)response);
}

void host_action_prompt_begin(const char *const pstr, const bool eol) {
    DBG_HOST("host_action_prompt_begin '%s' %d", pstr, (int)eol);
    strcpy(host_prompt, pstr);
    host_prompt_buttons = 0;
}

void host_action_prompt_button(const char *const pstr) {
    DBG_HOST("host_action_prompt_button '%s'", pstr);
    if (host_prompt_buttons < HOST_BUTTON_CNT_MAX) {
        strcpy(host_prompt_button[host_prompt_buttons], pstr);
        host_prompt_buttons++;
    }
}

void host_action_prompt_end() {
    DBG_HOST("host_action_prompt_end");
    *host_prompt = 0;
    host_prompt_buttons = 0;
}

void host_action_prompt_show() {
    DBG_HOST("host_action_prompt_show");
    marlin_host_prompt_t prompt = {
        marlin_host_prompt_by_text(host_prompt),
        host_prompt_buttons,
        {
            (host_prompt_buttons > 0) ? marlin_host_prompt_button_by_text(host_prompt_button[0]) : HOST_PROMPT_BTN_None,
            (host_prompt_buttons > 1) ? marlin_host_prompt_button_by_text(host_prompt_button[1]) : HOST_PROMPT_BTN_None,
            (host_prompt_buttons > 2) ? marlin_host_prompt_button_by_text(host_prompt_button[2]) : HOST_PROMPT_BTN_None,
            (host_prompt_buttons > 3) ? marlin_host_prompt_button_by_text(host_prompt_button[3]) : HOST_PROMPT_BTN_None,
        }
    };
    int paused = 0;
    uint32_t ui32 = marlin_host_prompt_encode(&prompt);
    _send_notify_event(MARLIN_EVT_HostPrompt, ui32, 0);
    switch (prompt.type) {
    case HOST_PROMPT_Paused:
        paused = 0;
        if ((marlin_server.command == MARLIN_CMD_M600) || (marlin_server.command == MARLIN_CMD_M701))
            paused = 1; // pause only for M600 or M701
        while (paused) {
            host_prompt_button_clicked = HOST_PROMPT_BTN_None;
            while (host_prompt_button_clicked == HOST_PROMPT_BTN_None) {
                idle(); // call Marlin idle()
            }
            switch (host_prompt_button_clicked) {
            case HOST_PROMPT_BTN_Continue:
                paused = 0;
                break;
            case HOST_PROMPT_BTN_PurgeMore:
                do_pause_e_move(ADVANCED_PAUSE_PURGE_LENGTH, ADVANCED_PAUSE_PURGE_FEEDRATE);
                paused = 0; //TODO: temporarily skip the loop because of unload UI
                break;
            default:
                paused = 0;
                break;
            }
        }
    case HOST_PROMPT_LoadFilament:
        wait_for_user = false; //hack - skip waiting
        break;
    case HOST_PROMPT_FilamentRunout:
        break;
    default:
        break;
    }
    *host_prompt = 0;
    host_prompt_buttons = 0;
}

void host_prompt_do(const PromptReason type, const char *const pstr, const char *const pbtn) {
    DBG_HOST("host_prompt_do %d '%s' '%s'", (int)type, pstr, pbtn);

    switch (type) {
    case PROMPT_INFO:
        if (strcmp(pstr, "Reheating") == 0) {
            _send_notify_event(MARLIN_EVT_Reheat, 1, 0);
        }
        break;
    case PROMPT_USER_CONTINUE:
        //gui must call marlin_print_resume();
        if (strcmp(pstr, "Reheat Done") == 0) {
            _send_notify_event(MARLIN_EVT_Reheat, 0, 0);
        }
        break;
    default:
        break;
    }
}
