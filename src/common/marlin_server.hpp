// marlin_server.hpp
#pragma once

#include "marlin_server.h"
#include "dialog_commands.hpp"

/*****************************************************************************/
//C++ only features

//inheritred class for server side to be able to work with server_side_encoded_dialog_command
class ServerDialogCommands : public DialogCommands {
    ServerDialogCommands() = delete;
    static uint32_t server_side_encoded_dialog_command;

public:
    //call inside marlin server on received command from client
    static void SetCommand(uint32_t encoded_bt) {
        server_side_encoded_dialog_command = encoded_bt;
    }
    //return command state and erase it
    //return -1 if button does not match
    template <class T>
    static Command GetCommandFromPhase(T phase) {
        uint32_t _phase = server_side_encoded_dialog_command >> COMMAND_BITS;
        if ((static_cast<uint32_t>(phase)) != _phase)
            return Command::_none;
        uint32_t index = server_side_encoded_dialog_command & uint32_t(MAX_COMMANDS - 1); //get command index
        server_side_encoded_dialog_command = -1;                                          //erase command
        return GetCommand(phase, index);
    }
};

//Dialog_notifier
class Dialog_notifier {
    struct data { //used floats - no need to retype
        dialog_t type;
        uint8_t phase;
        float scale;  //scale from value to progress
        float offset; //offset from lowest value
        uint8_t progress_min;
        uint8_t progress_max;
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
    Dialog_notifier(dialog_t type, uint8_t phase, cvariant8 min, cvariant8 max, uint8_t progress_min, uint8_t progress_max, uint8_t var_id);
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
        : Dialog_notifier(type, phase, cvariant8(min), cvariant8(max), progress_min, progress_max, VAR_ID) {}
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

//create dialog and automatically destroy it at the end of scope
class DialogRAII {
    dialog_t dialog;

public:
    DialogRAII(dialog_t type, uint8_t data)
        : dialog(type) {
        open_dialog_handler(type, data);
    }
    ~DialogRAII() {
        close_dialog_handler(dialog);
    }
};
