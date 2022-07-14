#include "ScreenHandler.hpp"
#include "bsod.h"

Screens *Screens::instance = nullptr;

Screens::Screens(screen_node screen_creator)
    : stack_iterator(stack.begin())
    , current(nullptr)
    , creator_node(screen_creator)
    , close(false)
    , close_all(false)
    , close_serial(false)
    , timeout_tick(0) {
}

void Screens::Init(screen_node screen_creator) {
    static Screens s(screen_creator);
    instance = &s;
}

Screens::iter Screens::find_enabled_node(iter begin, iter end) {
    return std::find_if(begin, end, [](const screen_node &node) { return node.creator != nullptr; });
}

Screens::r_iter Screens::rfind_enabled_node(r_iter begin, r_iter end) {
    return std::find_if(r_iter(end), r_iter(begin), [](const screen_node &node) { return node.creator != nullptr; });
}

void Screens::Init(const screen_node *begin, const screen_node *end) {
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

void Screens::RInit(const screen_node *begin, const screen_node *end) {
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

void Screens::EnableMenuTimeout() {
    ResetTimeout();
    menu_timeout_enabled = true;
}
void Screens::DisableMenuTimeout() {
    menu_timeout_enabled = false;
}
bool Screens::GetMenuTimeout() { return menu_timeout_enabled; }

// Push enabled creators on stack - in reverted order
// not a bug non reverting method must use reverse iterators
void Screens::PushBeforeCurrent(const screen_node *begin, const screen_node *end) {
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
void Screens::RPushBeforeCurrent(const screen_node *begin, const screen_node *end) {
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

void Screens::ScreenEvent(window_t *sender, GUI_event_t event, void *param) {
    if (current == nullptr)
        return;
    //todo shouldn't I use "sender ? sender : current.get()"?
    current->ScreenEvent(current.get(), event, param);
}

void Screens::WindowEvent(GUI_event_t event, void *param) {
    if (current == nullptr)
        return;
    current->WindowEvent(current.get(), event, param);
}

void Screens::Draw() {
    if (current == nullptr)
        return;
    current->Draw();
}

screen_t *Screens::Get() const {
    if (!current) {
        return nullptr;
    }
    return current.get();
}

void Screens::Open(screen_node screen_creator) {
    creator_node = screen_creator;
}

void Screens::Open(const ScreenFactory::Creator screen_creator) {
    Open(screen_node(screen_creator));
}

void Screens::Close() {
    close = true;
}

void Screens::CloseAll() {
    close_all = true;
}

void Screens::CloseSerial() {
    /// serial close logic:
    /// when serial printing screen (M876) is open, Screens::SerialClose() is
    /// called and it will iterate all screens to close those that should be closed
    while (Get() && Get()->ClosedOnSerialPrint() && stack_iterator != stack.begin()) {
        close = true;
        InnerLoop();
    }
}

//used to close blocking dialogs
bool Screens::ConsumeClose() {
    bool ret = close | close_all; // close_all must also close dialogs
    close = false;                //close_all cannot be consumed
    return ret;
}

void Screens::PushBeforeCurrent(screen_node screen_creator) {
    if (stack_iterator != stack.end()) {
        (*(stack_iterator + 1)) = *stack_iterator; // copy current creator
        (*stack_iterator) = screen_creator;        // save new screen creator before current
        ++stack_iterator;                          // point to current creator
    } else {
        bsod("Screen stack overflow");
    }
}

void Screens::ResetTimeout() {
    timeout_tick = gui::GetTick();
}

void Screens::Loop() {
    /// menu timeout logic:
    /// when timeout is expired on current screen,
    /// we iterate through whole stack and close every screen that should be closed
    if (menu_timeout_enabled && Get() && Get()->ClosedOnTimeout() && (!Get()->HasDialogOrPopup())) {
        if (gui::GetTick() - timeout_tick > MENU_TIMEOUT_MS) {
            while (Get() && Get()->ClosedOnTimeout() && stack_iterator != stack.begin()) {
                close = true;
                InnerLoop();
            }
            // no need to call ResetTimeout() - screen destructor does that
            return;
        }
    } else {
        ResetTimeout(); // in case timeout was enabled while menu was opened
    }
    /// continue inner loop
    InnerLoop();
}

void Screens::InnerLoop() {
    screen_init_variant screen_state;
    if (close_all) {
        if (current) {                              // is there something to close?
            if (creator_node.creator) {             // have creator, have to emulate opening
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
    if (creator_node.creator || close) {
        if (current) {
            screen_state = current->GetCurrentState();
            current.reset(); //without a reset screens do not behave correctly, because they occupy the same memory space as the new screen to be created
            if (close) {
                if (stack_iterator != stack.begin()) {
                    --stack_iterator;               // point to previous screen - will become "behind last creator"
                    creator_node = *stack_iterator; // use previous screen as current creator
                    close = false;
                } else {
                    bsod("Screen stack underflow");
                }
            } else {
                if (stack_iterator != stack.end() && (stack_iterator + 1) != stack.end()) {
                    stack_iterator->init_data = screen_state; // store current init data
                    ++stack_iterator;                         // point behind last creator
                    (*stack_iterator) = creator_node;         // save future creator on top of the stack
                } else {
                    bsod("Screen stack overflow");
                }
            }
        }

        /// need to reset focused and capture ptr before calling current = creator();
        /// screen ctor can change those pointers
        /// screen was destroyed by unique_ptr.release()
        window_t::ResetFocusedWindow();
        current = creator_node.creator();
        /// need to be reset also focused ptr
        if (!current->IsFocused() && !current->IsChildFocused()) {
            window_t *child = current->GetFirstEnabledSubWin();
            if (child) {
                child->SetFocus();
            } else {
                current->SetFocus();
            }
        }
        current->InitState(creator_node.init_data);
        creator_node = nullptr;
    }
}
