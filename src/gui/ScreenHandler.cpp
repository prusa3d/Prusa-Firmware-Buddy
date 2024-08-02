#include "ScreenHandler.hpp"
#include "bsod.h"

#include <gui.hpp>

static const uint32_t MENU_TIMEOUT_MS = 30000;

Screens *Screens::instance = nullptr;

Screens::Screens(screen_node screen_creator)
    : stack_iterator(stack.begin())
    , current(nullptr)
    , creator_node(screen_creator)
    , close(false)
    , close_all(false)
    , close_printing(false)
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
    if (size_t(end - begin) > MAX_SCREENS) {
        return;
    }
    if (begin == end) {
        return;
    }

    // find last enabled creator
    iter node = find_enabled_node(begin, end);
    if (node == end) {
        return;
    }

    // have creator
    Init(*node);

    // Must push rest of enabled creators on stack
    Access()->PushBeforeCurrent(node + 1, end); // node + 1 excludes node
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
    if (size_t(end - begin) > MAX_SCREENS) {
        return;
    }
    if (begin == end) {
        return;
    }

    // initialize reverse iterators
    r_iter r_begin(begin);
    r_iter r_node(end + 1); // point behind end, first call of "r_node + 1" will revert this

    do {
        r_node = rfind_enabled_node(r_begin, r_node + 1);
        if (r_node != r_begin) {
            (*stack_iterator) = *r_node;
            ++stack_iterator;
        }
    } while (r_node != r_begin);
}

Screens *Screens::Access() {
    if (!instance) {
        bsod("Accessing uninitialized screen");
    }
    return instance;
}

void Screens::ScreenEvent([[maybe_unused]] window_t *sender, GUI_event_t event, void *const param) {
    if (current == nullptr) {
        return;
    }
    // todo shouldn't I use "sender ? sender : current.get()"?
    current->ScreenEvent(current.get(), event, param);
}

void Screens::WindowEvent(GUI_event_t event, void *const param) {
    if (current == nullptr) {
        return;
    }
    current->WindowEvent(current.get(), event, param);
}

void Screens::Draw() {
    if (current == nullptr) {
        return;
    }
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

/**
 * @brief close current screen
 * it sets flag to close current screen
 * it also clears creator_node, because order matters!
 * In case you want to replace current screen, you must call Close() first and Open() after
 */
void Screens::Close() {
    close = true;
    creator_node.MakeEmpty();
}

/**
 * @brief close all screens (but top one - home)
 * it sets flag to close all screens
 * it also clears creator_node, because order matters!
 * In case you want to open new screen, you must call CloseAll() first and Open() after
 */
void Screens::CloseAll() {
    close_all = true;
    creator_node.MakeEmpty();
}

/**
 * @brief close all screens with WindowFlags::print_close (but top one - home)
 * it sets flag to close all screens closable on print
 * it also clears creator_node, because order matters!
 * In case you want to open new screen, you must call ClosePrinting() first and Open() after
 */
void Screens::ClosePrinting() {
    close_printing = true;
    creator_node.MakeEmpty();
}

void Screens::PushBeforeCurrent(screen_node screen_creator) {
    if (stack_iterator != stack.end()) {
        (*(stack_iterator + 1)) = *stack_iterator; // copy current creator
        (*stack_iterator) = screen_creator; // save new screen creator before current
        ++stack_iterator; // point to current creator
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

/**
 * @brief inner loop, actually does all the work
 * it closes and opens screens
 * open is indicated by creator_node
 * top screen cannot be closed
 *
 * combinations are allowed
 * close + close_all   == close_all
 * close_all + open    == keep top screen + open new one
 * close + open        == replace current screen with new one, will not work with top one
 * close_printing + open == close all screens closable on print and open new one (until it finds one it cannot close)
 *
 * duplicity is not checked, so it is possible to open multiple screens of the same type
 * it would complicate code and it is probably not necessary
 * there is an exception combination of close/close_all + open, it does check opened screen type and does not recreate it
 * if close_printing finds screen of the same type that should be opened, it will close it too
 */
void Screens::InnerLoop() {
    screen_init_variant screen_state;
    if (close_all) {
        if (stack_iterator != stack.begin()) { // is there something to close? (we never close screen[0])
            if (!creator_node.IsEmpty()) { // have creator, have to emulate opening
                if ((stack_iterator)->creator == creator_node.creator) { // screen to be opened is already opened (on top of stack)
                    *(stack.begin() + 1) = *stack_iterator; // move (copy) current screen_node, init data does not matter - they are set after screen is closed
                    stack_iterator = stack.begin() + 1; // point to current screen_node
                    if (creator_node.init_data.IsValid()) { // have some new meaningful data
                        current->InitState(creator_node.init_data); // reinitialize
                    }
                    creator_node.MakeEmpty(); // erase creator pointer, so we dont continue to open part of tis function
                } else { // screen to be opened is not currently opened (but might be between the closed ones)
                    stack_iterator = stack.begin(); // point to screen[0] (screen[0]), keep creator_node, we will continue to open part of tis function
                } //
                close = false; // clr close flag, creator will be pushed into screen[1] position
            } else { // do not have creator, have to emulate closing
                stack_iterator = stack.begin() + 1; // point behind screen[0], (screen[0] is home)
                close = true; // set flag to close screen[1] == open screen[0] (home)
            }
        }
        close_all = false; // reset close all flag
        close_printing = false; // all screens were closed, close_printing has no meaning
    }

    if (close_printing) {
        close_printing = false; // reset close printing flag now, so following InnerLoop only closes a screen
        // print close logic:
        // Screens::ClosePrinting is called whenever a print starts, either normal print or serial print(M876)
        // The purpose is to close appropriate screens when print starts from screens other than home screens
        // or filebrowser, which happens in case of serial print, or print started via network.
        if (stack_iterator != stack.begin()) { // is there something to close?
            auto backup = creator_node; // backup creator (in case we need to both close and open at the same time)
            creator_node.creator = nullptr; // erase creator node
            while (stack_iterator != stack.begin() && ((Get() && Get()->ClosedOnPrint()) || (stack_iterator)->creator == backup.creator)) {
                close = true;
                InnerLoop(); // call recursively - but with only single level of recursion .. this will just close single screen (we already know it should be closed)
            }

            creator_node = backup; // now all screens are closed, so we just restore creator to open screen if there was a request to do it
        }
    }

    // special case open + close
    if (close && !creator_node.IsEmpty()) {
        if (stack_iterator != stack.begin()) { // is there something to close? (we never close screen[0])
            if ((stack_iterator)->creator == creator_node.creator) { // screen to be opened is already opened (on top of stack)
                *(stack_iterator - 1) = *stack_iterator; // move (copy) current screen_node 1 position up on stack, init data does not matter - they are set after screen is closed
                --stack_iterator; // point to current screen_node
                if (creator_node.init_data.IsValid()) { // have some new meaningful data
                    current->InitState(creator_node.init_data); // reinitialize
                }
                creator_node.MakeEmpty(); // erase creator pointer, so we dont continue to open part of tis function
            } else { // screen to be opened is not currently opened (but might be between the closed ones)
                --stack_iterator; // point to screen_node 1 position up on stack
            }
        }
        close = false; // clr close flag
    }

    // open new screen .. close means open old one from stack
    if (!creator_node.IsEmpty() || close) {
        if (current) {
            screen_state = current->GetCurrentState();
            if (close) {
                if (stack_iterator != stack.begin()) {
                    --stack_iterator; // point to previous screen - will become "behind last creator"
                    creator_node = *stack_iterator; // use previous screen as current creator
                    close = false;
                } else {
                    bsod("Screen stack underflow");
                }
            } else {
                if (stack_iterator != stack.end() && (stack_iterator + 1) != stack.end()) {
                    stack_iterator->init_data = screen_state; // store current init data
                    ++stack_iterator; // point behind last creator
                    (*stack_iterator) = creator_node; // save future creator on top of the stack
                } else {
                    bsod("Screen stack overflow");
                }
            }
        }

        /// without a reset screens does not behave correctly, because they occupy the same memory space as the new screen to be created
        if (current) {
            current.reset();
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
        creator_node.MakeEmpty();
    }
}

void Screens::gui_loop_until_dialog_closed(stdext::inplace_function<void()> callback) {
    for (;;) {
        const bool dialog_closed = close || close_all;
        close = false; // Note: We reset close flag because it is reused for closing both dialogs and screens
        if (dialog_closed) {
            break;
        }

        gui::TickLoop();
        gui_loop();
        if (callback) {
            callback();
        }
    }
}
