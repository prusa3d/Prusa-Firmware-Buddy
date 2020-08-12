#include "ScreenHandler.hpp"
#include "bsod.h"

Screens *Screens::instance = nullptr;

Screens::Screens(const ScreenFactory::Creator screen_creator)
    : stack({ { nullptr } })
    , stack_iterator(stack.begin())
    , current(nullptr)
    , creator(screen_creator)
    , close(false)
    , close_all(false) {
}

void Screens::Init(const ScreenFactory::Creator screen_creator) {
    static Screens s(screen_creator);
    instance = &s;
}

Screens::iter Screens::find_enabled_node(iter begin, iter end) {
    return std::find_if(begin, end, [](const ScreenFactory::Creator &node) { return node != nullptr; });
}

Screens::r_iter Screens::rfind_enabled_node(r_iter begin, r_iter end) {
    return std::find_if(r_iter(end), r_iter(begin), [](const ScreenFactory::Creator &node) { return node != nullptr; });
}

void Screens::Init(const ScreenFactory::Creator *begin, const ScreenFactory::Creator *end) {
    if (size_t(end - begin) > MAX_SCREENS)
        return;
    if (begin == end)
        return;

    //find last enabled creator
    iter node = find_enabled_node(begin, end);
    if (node == end)
        return;

    //have creator
    Init(*node);

    //Must push rest of enabled creators on stack
    Access()->PushBeforeCurrent(node + 1, end); //node + 1 excludes node
}

void Screens::RInit(const ScreenFactory::Creator *begin, const ScreenFactory::Creator *end) {
    if (size_t(end - begin) > MAX_SCREENS)
        return;
    if (begin == end)
        return;

    //initialize reverse iterators
    r_iter r_begin(begin);
    r_iter r_end(end);

    //find last enabled creator
    r_iter r_node = rfind_enabled_node(r_begin, r_end);
    if (r_node == r_begin)
        return;

    //have creator
    Init(*r_node);

    //Push rest of enabled creators on stack
    Access()->RPushBeforeCurrent(begin, r_node.base());
}

// Push enabled creators on stack - in reverted order
// not a bug non reverting method must use reverse iterators
void Screens::PushBeforeCurrent(const ScreenFactory::Creator *begin, const ScreenFactory::Creator *end) {
    if (size_t(end - begin) > MAX_SCREENS)
        return;
    if (begin == end)
        return;

    //initialize reverse iterators
    r_iter r_begin(begin);
    r_iter r_node(end + 1); //point behind end, first call of "r_node + 1" will revert this

    do {
        r_node = rfind_enabled_node(r_begin, r_node + 1);
        if (r_node != r_begin) {
            (*stack_iterator) = *r_node;
            ++stack_iterator;
        }
    } while (r_node != r_begin);
}

// Push enabled creators on stack - in non reverted order
// not a bug reverting method must use normal iterators
void Screens::RPushBeforeCurrent(const ScreenFactory::Creator *begin, const ScreenFactory::Creator *end) {
    if (size_t(end - begin) > MAX_SCREENS)
        return;
    if (begin == end)
        return;

    iter node = begin - 1; //point before begin, first call of "node + 1" will revert this

    do {
        node = find_enabled_node(node + 1, end);
        if (node != end) {
            (*stack_iterator) = *node;
            ++stack_iterator;
        }
    } while (node != end);
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

void Screens::Open(const ScreenFactory::Creator screen_creator) {
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

void Screens::PushBeforeCurrent(const ScreenFactory::Creator screen_creator) {
    if (stack_iterator != stack.end()) {
        (*(stack_iterator + 1)) = *stack_iterator; // copy current creator
        (*stack_iterator) = screen_creator;        // save new screen creator before current
        ++stack_iterator;                          // point to current creator
    } else {
        bsod("Screen stack overflow");
    }
}

void Screens::Loop() {
    if (close_all) {
        if (current) {                              // is there something to close?
            if (creator) {                          // have creator, have to emulate opening
                stack_iterator = stack.begin();     // point to screen[0], (screen[0] is home)
                close = false;                      // clr close flag, creator will be pushed into screen[1] position
            } else {                                // do not have creator, have to emulate closing
                stack_iterator = stack.begin() + 1; // point behind screen[0], (screen[0] is home)
                close = true;                       // set flag to close screen[1] == open screen[0] (home)
            }
        }
        close_all = false;
    }

    //open new screen
    if (creator || close) {
        if (current) {
            current.reset(); //without a reset screens do not behave correctly, I do not know why
            if (close) {
                if (stack_iterator != stack.begin()) {
                    --stack_iterator;          // point to previous screen - will become "behind last creator"
                    creator = *stack_iterator; // use previous screen as current creator
                    close = false;
                } else {
                    bsod("Screen stack underflow");
                }
            } else {
                if (stack_iterator != stack.end() && (stack_iterator + 1) != stack.end()) {
                    ++stack_iterator;            // point behind last creator
                    (*stack_iterator) = creator; // save future creator on top of the stack
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
