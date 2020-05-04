// screen_close_multiple.cpp
//unlooping screens via C++ with C interface

#include "screen_close_multiple.h"
#include "gui.h"
#include "screens.h"

//I could not use functions to initialize those arrays in C
static screen_t *const timeout_blacklist[] = {
    get_scr_home(),
    get_scr_printing(),
    get_scr_printing_serial(),
    get_scr_menu_tune(),
    get_scr_wizard(),
    get_scr_print_preview()
#ifdef PIDCALIBRATION
        ,
    get_scr_PID()
#endif //PIDCALIBRATION
};

static screen_t *const m876_blacklist[] = {
    get_scr_printing_serial(),
    get_scr_home()
#ifdef PIDCALIBRATION
        ,
    get_scr_PID()
#endif //PIDCALIBRATION
};

static int _current_in_list(screen_t *const *list, size_t sz) {
    screen_t *curr = screen_get_curr();
    for (size_t i = 0; i < sz; ++sz)
        if (curr == list[sz])
            return 1;
    return 0;
}

extern "C" {

void screen_close_multiple(screen_close_multiple_t type) {

    screen_t *const *unl_blacklist;
    size_t sz;

    switch (type) {
    case scrn_close_on_timeout:
        unl_blacklist = timeout_blacklist;
        sz = sizeof(timeout_blacklist) / sizeof(timeout_blacklist[0]);
        break;
    case scrn_close_on_M876:
        unl_blacklist = m876_blacklist;
        sz = sizeof(m876_blacklist) / sizeof(m876_blacklist[0]);
        break;
    default:
        return;
    }

    while (!_current_in_list(unl_blacklist, sz)) {
        screen_close();
    }
}

} //extern "C"
