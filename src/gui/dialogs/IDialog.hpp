#pragma once

#include <stdint.h>
#include "window.hpp"

//window_t has pragma pack, have to use here too
//interface for dialog
class IDialog : protected window_t {
public:
    IDialog();
    //virtual void Change(uint8_t phase, uint8_t progress_tot, uint8_t progress) = 0;

    static void c_draw(window_t *win);
    static void c_event(window_t *win, uint8_t event, void *param);
};
