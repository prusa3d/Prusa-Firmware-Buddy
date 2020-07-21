#include "ScreenHandler.hpp"
#include "bsod.h"

Screens *Screens::instance = nullptr;

Screens::Screens(ScreenFactory::Creator screen_creator)
    : stack({ { nullptr } })
    , stack_iterator(stack.begin())
    , current(nullptr)
    , creator(screen_creator)
    , close(false) {
}

void Screens::Init(ScreenFactory::Creator screen_creator) {
    static Screens s(screen_creator);
    instance = &s;
}

Screens *Screens::Access() {
    if (!instance)
        bsod("Accessing uninitialized screen");
    return instance;
}

void Screens::ScreenEvent(window_t *sender, uint8_t event, void *param) {
    current->WindowEvent(current.get(), event, param);
}

void Screens::Draw() {
    current->Draw();
}

window_frame_t *Screens::Get() {
    return current.get();
}

void Screens::Open(ScreenFactory::Creator screen_creator) {
    creator = screen_creator;
}

void Screens::Close() {
    close = true;
}

bool Screens::ConsumeClose() {
    bool ret = close;
    close = false;
    return ret;
}

void Screens::Loop() {
    //close screen
    if (close) {
        if (stack_iterator != stack.begin()) {
            --stack_iterator;
            creator = (*(stack_iterator - 1));
            close = false;
        } else {
            bsod("Screen stack underflow");
        }
    }

    //open screen
    if (creator) {
        if (current) {
            current.reset(); //without reset screens does not behave correctly, I do not know why
            if (stack_iterator != stack.end()) {
                (*stack_iterator) = creator;
                ++stack_iterator;
            } else {
                bsod("Screen stack overflow");
            }
        }
        current = creator();
        creator = nullptr;
        gui_invalidate();
    }
}
