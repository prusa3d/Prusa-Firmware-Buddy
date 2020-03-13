#pragma once
#include "dialog_commands.hpp"

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
            return Command::_NONE;
        uint32_t index = server_side_encoded_dialog_command & uint32_t(MAX_COMMANDS - 1); //get command index
        server_side_encoded_dialog_command = -1;                                          //erase command
        return GetCommand(phase, index);
    }
};
