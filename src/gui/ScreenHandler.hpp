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
#pragma once
#include "window_frame.hpp"
#include "ScreenFactory.hpp"
#include <array>

//stack with screen creator methods
using ScreenArray = std::array<ScreenFactory::Creator, 32>;

class Screens {

    ScreenArray stack;
    ScreenArray::iterator stack_iterator;

    ScreenFactory::UniquePtr current;
    ScreenFactory::Creator creator; // set by Open

    bool close;

    void stack_push(int16_t screen_id) {}

    int16_t stack_pop(void) {}

    //static screen_t *screen_get_curr(void){}

    Screens(ScreenFactory::Creator screen_creator);
    Screens(const Screens &) = delete;
    static Screens *instance;

public:
    void Loop(); //call inside guiloop

    void Open(ScreenFactory::Creator screen_creator); //remember creator and create later

    void Close();

    void Draw();

    void DispatchEvent(window_t *sender, uint8_t event, void *param);

    static void Init(ScreenFactory::Creator screen_creator);
    static Screens *Access();
};
