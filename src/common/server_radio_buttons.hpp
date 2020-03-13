#pragma once
#include "radio_buttons.hpp"

//inheritred class for server side to be able to work with server_side_encoded_radio_button
class ServerRadioButtons : public RadioButtons {
    ServerRadioButtons() = delete;
    static uint32_t server_side_encoded_radio_button;

public:
    static void SetRadioButtons(uint32_t encoded_bt) {
        server_side_encoded_radio_button = encoded_bt;
    }
    //return radiobutton state and erase it
    //return -1 if button does not match
    template <class T>
    static Button GetRadioButton(T radio_bt) {
        uint32_t _radio_bt = server_side_encoded_radio_button >> BTNS_BITS;
        if ((static_cast<uint32_t>(radio_bt)) != _radio_bt)
            return Button::_NONE;
        uint32_t index = server_side_encoded_radio_button & uint32_t(MAX_BTNS - 1); //get button idex
        server_side_encoded_radio_button = -1;                                      //erase button click
        return GetButton(radio_bt, index);
    }
};
