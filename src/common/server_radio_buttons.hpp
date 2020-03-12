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
    template <class T>
    static uint8_t IsButtons(T bt) {
        uint32_t _bt = server_side_encoded_radio_button >> BTNS_BITS;
        if ((static_cast<uint32_t>(bt)) != _bt)
            return -1;
        return (static_cast<uint32_t>(bt)) & (MAX_BTNS - 1);
    }
};
