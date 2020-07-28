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
    current->ScreenEvent(current.get(), event, param);
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

void Screens::CloseAll() {
    close_all = true;
}

//used to close blocking dialogs
bool Screens::ConsumeClose() {
    bool ret = close | close_all; // close_all must also close dialogs
    close = false;                //close_all cannot be consumed
    return ret;
}

void Screens::Loop() {
    if (close_all) {
        if (current) {                              // is there something to close?
            if (creator) {                          // have creator, have to emulate opening
                stack_iterator = stack.begin() + 1; // point behind screen[0], (screen[0] is home)
                close = false;                      // clr close flag, creator will be pusthed into screen[1] position
            } else {                                // do not have creator, have to emulate closing
                stack_iterator = stack.begin() + 2; // point behind screen[1], (screen[0] is home)
                close = true;                       // set flag to close screen[1] == open screen[0] (home)
            }
        }
        close_all = false;
    }

    //open new screen
    if (creator || close) {
        if (current) {
            current.reset(); //without reset screens does not behave correctly, I do not know why
            if (close) {
                if (stack_iterator != stack.begin() && (stack_iterator - 1) != stack.begin()) {
                    --stack_iterator;                  // point behind current creator - will become "behind last creator"
                    creator = (*(stack_iterator - 1)); // use previous screen as current creator
                    close = false;
                } else {
                    bsod("Screen stack underflow");
                }
            } else {
                if (stack_iterator != stack.end()) {
                    (*stack_iterator) = creator; // save creator on stack
                    ++stack_iterator;            // point behind last creator
                } else {
                    bsod("Screen stack overflow");
                }
            }
        }
        current = creator();
        if (!current->IsChildCaptured())
            current->SetCapture();
        if (!current->IsChildFocused() && !current->IsChildFocused()) {
            window_t *child = current->GetFirstEnabledSubWin();
            if (child) {
                child->SetFocus();
            } else {
                current->SetFocus();
            }
        }
        creator = nullptr;
        gui_invalidate();
    }
}
