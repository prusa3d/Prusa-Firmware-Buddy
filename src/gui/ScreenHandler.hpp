#if 0
    #pragma once

    #include <stdint.h>
    #include "client_fsm_types.h"
    #include "DialogFactory.hpp"

class DialogHandler {
    static_unique_ptr<IDialogStateful> ptr;
    DialogFactory::Ctors dialog_ctors;
    ClientFSM waiting_closed = ClientFSM::_none;

    DialogHandler(DialogFactory::Ctors ctors)
        : dialog_ctors(ctors) {}
    DialogHandler(DialogHandler &) = delete;

    void open(ClientFSM dialog, uint8_t data);
    void close(ClientFSM dialog);
    void change(ClientFSM dialog, uint8_t phase, uint8_t progress_tot, uint8_t progress);

    void wait_until_closed(ClientFSM dialog, uint8_t data);

public:
    //accessor for static methods
    static DialogHandler &Access();
    //static methods to be registerd as callbacks
    static void Open(ClientFSM dialog, uint8_t data);
    static void Close(ClientFSM dialog);
    static void Change(ClientFSM dialog, uint8_t phase, uint8_t progress_tot, uint8_t progress);
    static void WaitUntilClosed(ClientFSM dialog, uint8_t data); //opens dialog, waits until closed, auto loops
};
#endif

#include "window_frame.hpp"
#include "ScreenFactory.hpp"
#include <array>

using ScreenArray = std::array<ScreenFactory::fnc, 32>;
class Screens {

    static ScreenArray stack;
    static void stack_push(int16_t screen_id) {}

    static int16_t stack_pop(void) {}

    //static screen_t *screen_get_curr(void){}
    static static_unique_ptr<window_frame_t> current;

public:
    static void Init(int16_t screen_id) {} //similar to open
    static void Open(int16_t screen_id) {}

    static void Close(void) {}

    static void Draw(void) {}

    static void DispatchEvent(window_t *window, uint8_t event, void *param) {}
};
