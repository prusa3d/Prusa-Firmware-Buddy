/**
 * @file marlin_server.hpp
 * @brief interface between Marlin and Prusa code
 */

#pragma once

#include "../../lib/Marlin/Marlin/src/inc/MarlinConfig.h"
#include "marlin_events.h"
#include "marlin_vars.hpp"
#include "marlin_errors.h"
#include "client_fsm_types.h"
#include "marlin_server.hpp"
#include "client_response.hpp"
#include "fsm_types.hpp"
#include "fsm_progress_type.hpp"

#include <cstddef>
#include <optional>
#include <atomic>

// server flags
// FIXME define the same type for these and marlin_server.flags
static constexpr uint16_t MARLIN_SFLG_STARTED = 0x0001; // server started (set in marlin_server_init)
static constexpr uint16_t MARLIN_SFLG_PROCESS = 0x0002; // loop processing in main thread is enabled
static constexpr uint16_t MARLIN_SFLG_BUSY = 0x0004;    // loop is busy
static constexpr uint16_t MARLIN_SFLG_PENDREQ = 0x0008; // pending request
static constexpr uint16_t MARLIN_SFLG_EXCMODE = 0x0010; // exclusive mode enabled (currently used for selftest/wizard)

// server variable update interval [ms]
static constexpr uint8_t MARLIN_UPDATE_PERIOD = 100;

using marlin_server_idle_t = void (*)();

// callback for idle operation inside marlin (called from ExtUI handler onIdle)
extern marlin_server_idle_t marlin_server_idle_cb;

//-----------------------------------------------------------------------------
// server side functions (can be called from server thread only)

// initialize server side - must be called at beginning in server thread
void marlin_server_init();

// server loop - must be called periodically in server thread
int marlin_server_loop();

// returns enabled status of loop processing
int marlin_server_processing();

// direct start loop processing
void marlin_server_start_processing();

// direct stop loop processing + disable heaters and safe state
void marlin_server_stop_processing();

// direct call of babystep.add_steps(Z_AXIS, ...)
void marlin_server_do_babystep_Z(float offs);

void marlin_server_move_axis(float pos, float feedrate, size_t axis);

// direct call of 'enqueue_and_echo_command'
// @retval true command enqueued
// @retval false otherwise
bool marlin_server_enqueue_gcode(const char *gcode);

// direct call of 'inject_P'
// @retval true command enqueued
// @retval false otherwise
bool marlin_server_inject_gcode(const char *gcode);

// direct call of settings.save()
void marlin_server_settings_save();

// direct call of settings.load()
void marlin_server_settings_load();

// direct call of settings.reset()
void marlin_server_settings_reset();

// direct call of thermalManager.manage_heater()
void marlin_server_manage_heater();

// direct call of planner.quick_stop()
void marlin_server_quick_stop();

// direct print file with SFM format
void marlin_server_print_start(const char *filename, bool skip_preview);

//
uint32_t marlin_server_get_command();

//
void marlin_server_set_command(uint32_t command);

//
void marlin_server_test_start(uint64_t mask);

//
void marlin_server_test_abort();

//
void marlin_server_print_abort();

//
void marlin_server_print_resume();

//
void marlin_server_print_reheat_start();

//
bool marlin_server_print_reheat_ready();

// return true if the printer is not moving (idle, paused, aborted or finished)
bool marlin_server_printer_idle();

typedef struct
{
    xyze_pos_t pos;    // resume position for unpark_head
    float nozzle_temp; // resume nozzle temperature
    uint8_t fan_speed; // resume fan speed
} resume_state_t;

//
void marlin_server_print_pause();

// return true if the printer is in the paused and not moving state
bool marlin_server_printer_paused();

// return the resume state during a paused print
resume_state_t *marlin_server_get_resume_data();

// set the resume state for unpausing a print
void marlin_server_set_resume_data(const resume_state_t *data);

/// Plans retract and returns E stepper position in mm
void marlin_server_retract();

/// Lifts printing head
void marlin_server_lift_head();

/// Parks head at print pause or crash
/// If Z lift or retraction wasn't performed
/// you can rerun them.
void marlin_server_park_head();

//
void marlin_server_unpark_head_XY();
void marlin_server_unpark_head_ZE();

//
int marlin_all_axes_homed();

//
int marlin_all_axes_known();

// returns state of exclusive mode (1/0)
int marlin_server_get_exclusive_mode();

// set state of exclusive mode (1/0)
void marlin_server_set_exclusive_mode(int exclusive);

// display different value than target, used in preheat
void marlin_server_set_temp_to_display(float value);

//
float marlin_server_get_temp_to_display();

//
float marlin_server_get_temp_nozzle();

//
void marlin_server_resuming_begin();

uint32_t marlin_server_get_user_click_count();

uint32_t marlin_server_get_user_move_count();

void marlin_server_nozzle_timeout_on();
void marlin_server_nozzle_timeout_off();

//todo ensure signature match
//notify all clients to create finite statemachine
void fsm_create(ClientFSM type, uint8_t data = 0);
//notify all clients to destroy finite statemachine, must match fsm_destroy_t signature
void fsm_destroy(ClientFSM type);
//notify all clients to change state of finite statemachine, must match fsm_change_t signature
//can be called inside while, notification is send only when is different from previous one
void _fsm_change(ClientFSM type, fsm::BaseData data);

template <class T>
void fsm_change(ClientFSM type, T phase, fsm::PhaseData data = fsm::PhaseData({ 0, 0, 0, 0 })) {
    _fsm_change(type, fsm::BaseData(GetPhaseIndex(phase), data));
}

// user can stop waiting for heating/cooling by pressing a button
bool can_stop_wait_for_heatup();
void can_stop_wait_for_heatup(bool val);

//inherited class for server side to be able to work with server_side_encoded_response
class ClientResponseHandler : public ClientResponses {
    ClientResponseHandler() = delete;
    ClientResponseHandler(ClientResponseHandler &) = delete;
    static std::atomic<uint32_t> server_side_encoded_response;

public:
    //call inside marlin server on received response from client
    static void SetResponse(uint32_t encoded_bt) {
        server_side_encoded_response = encoded_bt;
    }
    //return response and erase it
    //return UINT32_MAX if button does not match
    //can be used from sub thread, as long as only one thread at the time reads it
    template <class T>
    static Response GetResponseFromPhase(T phase) {
        const uint32_t value = server_side_encoded_response.exchange(UINT32_MAX); //read and erase response

        uint32_t _phase = value >> RESPONSE_BITS;
        if ((static_cast<uint32_t>(phase)) != _phase)
            return Response::_none;
        uint32_t index = value & uint32_t(MAX_RESPONSES - 1); //get response index
        return GetResponse(phase, index);
    }
};

//FSM_notifier
class FSM_notifier {
    struct data { //used floats - no need to retype
        ClientFSM type;
        uint8_t phase;
        float scale = 1;  //scale from value to progress
        float offset = 0; //offset from lowest value
        uint8_t progress_min = 0;
        uint8_t progress_max = 100;
        marlin_var_id_t var_id;
        std::optional<uint8_t> last_progress_sent;
        data()
            : type(ClientFSM::_none)
            , phase(0)
            , var_id(static_cast<marlin_var_id_t>(0)) {}
    };
    //static members
    //there can be only one active instance of FSM_notifier, which use this data
    static data s_data;
    static FSM_notifier *activeInstance;

    //temporary members
    //constructor stores previous state of FSM_notifier (its static data), destructor restores it
    data temp_data;

protected:
    //protected ctor so this instance cannot be created
    FSM_notifier(ClientFSM type, uint8_t phase, variant8_t min, variant8_t max, uint8_t progress_min, uint8_t progress_max, marlin_var_id_t var_id);
    FSM_notifier(const FSM_notifier &) = delete;
    virtual void preSendNotification() {}
    virtual void postSendNotification() {}

public:
    ~FSM_notifier();
    static void SendNotification();
};

//template used by using statement
template <marlin_var_id_t VAR_ID, class T>
class Notifier : public FSM_notifier {
public:
    Notifier(ClientFSM type, uint8_t phase, T min, T max, uint8_t progress_min, uint8_t progress_max) {};
    //        : FSM_notifier(type, phase, min, max, progress_min, progress_max, VAR_ID) {}
};

template <marlin_var_id_t VAR_ID>
class Notifier<VAR_ID, float> : public FSM_notifier {
public:
    Notifier(ClientFSM type, uint8_t phase, float min, float max, uint8_t progress_min, uint8_t progress_max)
        : FSM_notifier(type, phase, variant8_flt(min), variant8_flt(max), progress_min, progress_max, VAR_ID) {};
};

//use an alias to automatically notify progress
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
using Notifier_SD_PDONE = Notifier<MARLIN_VAR_SD_PDONE, uint8_t>;
using Notifier_DURATION = Notifier<MARLIN_VAR_DURATION, uint32_t>;

//create finite state machine and automatically destroy it at the end of scope
class FSM_Holder {
    ClientFSM dialog;

public:
    FSM_Holder(ClientFSM type, uint8_t data) //any data to send to dialog, could have different meaning for different dialogs
        : dialog(type) {
        fsm_create(type, data);
    }

    template <class T>
    void Change(T phase) const {
        fsm_change(dialog, phase);
    }

    template <class T>
    void Change(T phase, uint8_t progress) const {
        ProgressSerializer serializer(progress);
        fsm_change(dialog, phase, serializer.Serialize());
    }

    template <class T, class U>
    void Change(T phase, const U &serializer) const {
        fsm_change(dialog, phase, serializer.Serialize());
    }

    ~FSM_Holder() {
        fsm_destroy(dialog);
    }
};

uint8_t get_var_sd_percent_done();
void set_var_sd_percent_done(uint8_t value);
void set_warning(WarningType type);

#if ENABLED(CRASH_RECOVERY)
// Sets length of X and Y axes for crash recovery
void set_length(xy_float_t xy);
#endif

//directly access marlin server variables
const marlin_vars_t &marlin_server_read_vars();
