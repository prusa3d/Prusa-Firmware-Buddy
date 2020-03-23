// marlin_server.h
#ifndef _MARLIN_SERVER_H
#define _MARLIN_SERVER_H

#include "marlin_events.h"
#include "marlin_vars.h"
#include "marlin_errors.h"
#include "marlin_host.h"
#include "dialogs.h"

// server flags
#define MARLIN_SFLG_STARTED 0x0001 // server started (set in marlin_server_init)
#define MARLIN_SFLG_PROCESS 0x0002 // loop processing in main thread is enabled
#define MARLIN_SFLG_BUSY    0x0004 // loop is busy
#define MARLIN_SFLG_PENDREQ 0x0008 // pending request

// server variable update interval [ms]
#define MARLIN_UPDATE_PERIOD 100
#define MSG_STACK_SIZE       8  //status message stack size
#define MSG_MAX_LENGTH       21 //status message max length

typedef void(marlin_server_idle_t)(void);

#pragma pack(push)
#pragma pack(1)

typedef struct msg_stack {

    char msg_data[MSG_STACK_SIZE][MSG_MAX_LENGTH];
    uint8_t count;

} msg_stack_t;

#pragma pack(pop)

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern msg_stack_t msg_stack;

// callback for idle operation inside marlin (called from ExtUI handler onIdle)
extern marlin_server_idle_t *marlin_server_idle_cb;

//-----------------------------------------------------------------------------
// server side functions (can be called from server thread only)

// initialize server side - must be called at beginning in server thread
extern void marlin_server_init(void);

// server loop - must be called periodically in server thread
extern int marlin_server_loop(void);

// returns enabled status of loop processing
extern int marlin_server_processing(void);

// direct start loop processing
extern void marlin_server_start_processing(void);

// direct stop loop processing + disable heaters and safe state
extern void marlin_server_stop_processing(void);

// update variables at server side, defined by 'update' mask and send notification to all clients
extern void marlin_server_update(uint64_t update);

// set printing gcode name (for WUI)
extern void marlin_server_set_gcode_name(const char *request);

// get printing gcode name (for WUI)
extern void marlin_server_get_gcode_name(const char *dest);

// direct call of babystep.add_steps(Z_AXIS, ...)
extern void marlin_server_do_babystep_Z(float offs);

// direct call of 'enqueue_and_echo_command', returns 1 if command enqueued, otherwise 0
extern int marlin_server_enqueue_gcode(const char *gcode);

// direct call of 'inject_P', returns 1 if command enqueued, otherwise 0
extern int marlin_server_inject_gcode(const char *gcode);

// direct call of settings.save()
extern void marlin_server_settings_save(void);

// direct call of settings.load()
extern void marlin_server_settings_load(void);

// direct call of thermalManager.manage_heater()
extern void marlin_server_manage_heater(void);

// direct call of planner.quick_stop()
extern void marlin_server_quick_stop(void);

//
extern void marlin_server_print_abort(void);

//
extern void marlin_server_print_pause(void);

//
extern void marlin_server_print_resume(void);

//
extern void marlin_server_park_head(void);

//
extern int marlin_all_axes_homed(void);

//
extern int marlin_all_axes_known(void);

//must match dialog_open_cb_t signature
extern void open_dialog_handler(dialog_t type, uint8_t data);

//must match dialog_close_cb_t signature
extern void close_dialog_handler(dialog_t type);

//must match dialog_change_cb_t signature
extern void change_dialog_handler(dialog_t type, uint8_t phase, uint8_t progress_tot, uint8_t progress);

#ifdef __cplusplus
}

/*****************************************************************************/
//C++ only features

//Dialog_notifier
class Dialog_notifier {
    struct data {
        dialog_t type;
        uint8_t phase;
        cvariant8 min;
        cvariant8 range;
        cvariant8 progress_min;
        cvariant8 progress_range;
        uint8_t var_id;
        uint8_t last_progress_sent;
        data()
            : type(DLG_no_dialog)
            , phase(0)
            , var_id(0)
            , last_progress_sent(-1) {}
    };
    //static members
    static data s_data;

    //temporary members
    //to store previous state
    data temp_data;

protected:
    //protected ctor so this instance cannot be created
    Dialog_notifier(dialog_t type, uint8_t phase, cvariant8 min, cvariant8 max, cvariant8 progress_min, cvariant8 progress_max, uint8_t var_id);
    Dialog_notifier(const Dialog_notifier &) = delete;

public:
    ~Dialog_notifier();
    static void SendNotification();
};

//template used by using statement
template <int VAR_ID, class T>
class Notifier : public Dialog_notifier {
public:
    Notifier(dialog_t type, uint8_t phase, T min, T max, uint8_t progress_min, uint8_t progress_max)
        : Dialog_notifier(type, phase, cvariant8(min), cvariant8(max), cvariant8((T)progress_min), cvariant8((T)progress_max), VAR_ID) {}
};

//use an alias to automatically notyfi progress
//just create an instance and progress will be notyfied while it exists
using Notifier_MOTION = Notifier<MARLIN_VAR_MOTION, uint8_t>;
using Notifier_GQUEUE = Notifier<MARLIN_VAR_GQUEUE, uint8_t>;
using Notifier_PQUEUE = Notifier<MARLIN_VAR_PQUEUE, uint8_t>;
using Notifier_IPOS_X = Notifier<MARLIN_VAR_IPOS_X, uint32_t>;
using Notifier_IPOS_Y = Notifier<MARLIN_VAR_IPOS_Y, uint32_t>;
using Notifier_IPOS_Z = Notifier<MARLIN_VAR_IPOS_Z, uint32_t>;
using Notifier_IPOS_E = Notifier<MARLIN_VAR_IPOS_E, uint32_t>;
using Notifier_POS_X = Notifier<MARLIN_VAR_POS_X, float>;
using Notifier_POS_Y = Notifier<MARLIN_VAR_POS_Y, float>;
using Notifier_POS_Z = Notifier<MARLIN_VAR_POS_Z, float>;
using Notifier_POS_E = Notifier<MARLIN_VAR_POS_E, float>;
using Notifier_TEMP_NOZ = Notifier<MARLIN_VAR_TEMP_NOZ, float>;
using Notifier_TEMP_BED = Notifier<MARLIN_VAR_TEMP_BED, float>;
using Notifier_TTEM_NOZ = Notifier<MARLIN_VAR_TTEM_NOZ, float>;
using Notifier_TTEM_BED = Notifier<MARLIN_VAR_TTEM_BED, float>;
using Notifier_Z_OFFSET = Notifier<MARLIN_VAR_Z_OFFSET, float>;
using Notifier_FANSPEED = Notifier<MARLIN_VAR_FANSPEED, uint8_t>;
using Notifier_PRNSPEED = Notifier<MARLIN_VAR_PRNSPEED, uint16_t>;
using Notifier_FLOWFACT = Notifier<MARLIN_VAR_FLOWFACT, uint16_t>;
using Notifier_WAITHEAT = Notifier<MARLIN_VAR_WAITHEAT, uint8_t>;
using Notifier_WAITUSER = Notifier<MARLIN_VAR_WAITUSER, uint8_t>;
using Notifier_SD_PRINT = Notifier<MARLIN_VAR_SD_PRINT, uint8_t>;
using Notifier_SD_PDONE = Notifier<MARLIN_VAR_SD_PDONE, uint8_t>;
using Notifier_DURATION = Notifier<MARLIN_VAR_DURATION, uint32_t>;

#endif //__cplusplus

#endif //_MARLIN_SERVER_H
