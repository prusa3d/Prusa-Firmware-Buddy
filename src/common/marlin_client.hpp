// marlin_client.hpp
#pragma once

#include "marlin_client.h"
#include "dialog_commands.hpp"

//-----------------------------------------------------------------------------
// client side functions (can be called from client thread only)

// click button, return success
template <class T>
bool marlin_dialog_command(T phase, Command command) {
    uint32_t encoded = DialogCommands::Encode(phase, command);
    if (encoded == uint32_t(-1))
        return false;

    marlin_radio_button_click_encoded(encoded);
    return true;
}
