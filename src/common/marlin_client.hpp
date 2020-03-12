// marlin_client.hpp
#pragma once

#include "marlin_client.h"
#include "radio_buttons.h"

//-----------------------------------------------------------------------------
// client side functions (can be called from client thread only)

// click button, return success
template <class T>
bool marlin_radio_button_click(T bt, uint8_t clicked_index) {
    uint32_t encoded = RadioButtons::Encode(bt, clicked_index);
    if (encoded == uint32_t(-1))
        return false;

    marlin_button_click_encoded(encoded);
    return true;
}
