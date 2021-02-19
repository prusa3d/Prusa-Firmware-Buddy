// marlin_server.hpp
#pragma once

#include "marlin_server.h"
#include "client_response.hpp"

/*****************************************************************************/
//C++ only features

//todo ensure signature match
//notify all clients to create finit statemachine
void fsm_create(ClientFSM type, uint8_t data = 0);
//notify all clients to destroy finit statemachine, must match fsm_destroy_t signature
void fsm_destroy(ClientFSM type);
//notify all clients to change state of finit statemachine, must match fsm_change_t signature
//can be called inside while, notification is send only when is different from previous one
void _fsm_change(ClientFSM type, uint8_t phase, uint8_t progress_tot, uint8_t progress);

template <class T>
void fsm_change(ClientFSM type, T phase, uint8_t progress_tot, uint8_t progress) {
    _fsm_change(type, GetPhaseIndex(phase), progress_tot, progress);
}

//inherited class for server side to be able to work with server_side_encoded_response
class ClientResponseHandler : public ClientResponses {
    ClientResponseHandler() = delete;
    ClientResponseHandler(ClientResponseHandler &) = delete;
    static uint32_t server_side_encoded_response;

public:
    //call inside marlin server on received response from client
    static void SetResponse(uint32_t encoded_bt) {
        server_side_encoded_response = encoded_bt;
    }
    //return response and erase it
    //return -1 if button does not match
    template <class T>
    static Response GetResponseFromPhase(T phase) {
        uint32_t _phase = server_side_encoded_response >> RESPONSE_BITS;
        if ((static_cast<uint32_t>(phase)) != _phase)
            return Response::_none;
        uint32_t index = server_side_encoded_response & uint32_t(MAX_RESPONSES - 1); //get response index
        server_side_encoded_response = -1;                                           //erase response
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
        uint8_t var_id;
        uint8_t last_progress_sent;
        data()
            : type(ClientFSM::_none)
            , phase(0)
            , var_id(0)
            , last_progress_sent(-1) {}
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
    FSM_notifier(ClientFSM type, uint8_t phase, variant8_t min, variant8_t max, uint8_t progress_min, uint8_t progress_max, uint8_t var_id);
    FSM_notifier(const FSM_notifier &) = delete;
    virtual void preSendNotification() {}
    virtual void postSendNotification() {}

public:
    ~FSM_notifier();
    static void SendNotification();
};

//template used by using statement
template <int VAR_ID, class T>
class Notifier : public FSM_notifier {
public:
    Notifier(ClientFSM type, uint8_t phase, T min, T max, uint8_t progress_min, uint8_t progress_max) {};
    //        : FSM_notifier(type, phase, min, max, progress_min, progress_max, VAR_ID) {}
};

template <int VAR_ID>
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
using Notifier_SD_PRINT = Notifier<MARLIN_VAR_SD_PRINT, uint8_t>;
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
    void Change(T phase, uint8_t progress_tot, uint8_t progress) const {
        fsm_change(dialog, phase, progress_tot, progress);
    }

    ~FSM_Holder() {
        fsm_destroy(dialog);
    }
};

uint8_t get_var_sd_percent_done();
void set_var_sd_percent_done(uint8_t value);
void set_warning(WarningType type);
