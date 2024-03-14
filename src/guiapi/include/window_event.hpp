// window_event.hpp
#pragma once

#include <inttypes.h>

#undef CHANGE /// collision with Arduino macro

// window events
enum class GUI_event_t {
    LOOP = 1, // gui loop (every 50ms)
    BTN_DN, // button down                ... all windows - not only captured
    BTN_UP, // button up                  ... all windows - not only captured
    ENC_CHANGE, // value/index encoder change ... all windows - not only captured
    ENC_DN, // encoder minus              ... captured window only
    ENC_UP, // encoder plus               ... captured window only
    CLICK, // clicked (tag > 0)          ... captured window only
    HOLD, // held button                ... captured window only
    HELD_LEFT, // held and moved left        ... captured window only
    HELD_RIGHT, // held and moved right       ... captured window only
    HELD_RELEASED, // held and released          ... captured window only
    CHILD_CLICK, // click at the child screen
    FOCUS0, // focus lost
    FOCUS1, // focus set
    CAPT_0, // capture lost
    CAPT_1, // capture set
    TIMER, // gui timer
    TEXT_ROLL, // tick for text rolling classes
    MESSAGE, // onStatusChange() message notification
    MEDIA, // marlin media change
    GUI_STARTUP, // finish splash screen => initialization finish
    CHILD_CHANGED, // notify parent about child window change, bahavior depends on implementation
    REINIT_FOOTER, // forces reinitialization of all footers in GUI
    TOUCH, // event from touch screen
    HEADER_COMMAND // commands header to do something
};

// lower lever knob events
constexpr bool GUI_event_IsKnob(GUI_event_t event) {
    switch (event) {
    case GUI_event_t::BTN_DN:
    case GUI_event_t::BTN_UP:
    case GUI_event_t::ENC_CHANGE:
        return true;
    default:
        return false;
    }
}

constexpr bool GUI_event_IsCaptureEv(GUI_event_t event) {
    switch (event) {
    case GUI_event_t::ENC_DN:
    case GUI_event_t::ENC_UP:
    case GUI_event_t::CLICK:
    case GUI_event_t::HOLD:
    case GUI_event_t::TOUCH:
        return true;
    default:
        return false;
    }
}

constexpr bool GUI_event_IsWindowKnobReaction(GUI_event_t event) {
    switch (event) {
    case GUI_event_t::FOCUS0:
    case GUI_event_t::FOCUS1:
    case GUI_event_t::CAPT_0:
    case GUI_event_t::CAPT_1:
    case GUI_event_t::CLICK:
    case GUI_event_t::HOLD:
        return true;
    default:
        return false;
    }
}

constexpr bool GUI_event_IsAnyButLoop(GUI_event_t event) {
    return event != GUI_event_t::LOOP;
}

constexpr const char *GUI_event_prt(GUI_event_t event) {
    // cannot use: case GUI_event_t::BTN_DN: { static const char txt[] = "button down"; return txt; }
    // error: 'txt' declared 'static' in 'constexpr' function
    switch (event) {
    case GUI_event_t::LOOP:
        return "gui loop";
    case GUI_event_t::BTN_DN:
        return "button down";
    case GUI_event_t::BTN_UP:
        return "button up";
    case GUI_event_t::ENC_CHANGE:
        return "value/index changed";
    case GUI_event_t::ENC_DN:
        return "encoder minus";
    case GUI_event_t::ENC_UP:
        return "encoder plus";
    case GUI_event_t::CLICK:
        return "clicked";
    case GUI_event_t::HOLD:
        return "held button";
    case GUI_event_t::HELD_LEFT:
        return "held and left";
    case GUI_event_t::HELD_RIGHT:
        return "held and right";
    case GUI_event_t::HELD_RELEASED:
        return "held and released";
    case GUI_event_t::CHILD_CLICK:
        return "child click";
    case GUI_event_t::FOCUS0:
        return "focus lost";
    case GUI_event_t::FOCUS1:
        return "focus set";
    case GUI_event_t::CAPT_0:
        return "capture lost";
    case GUI_event_t::CAPT_1:
        return "capture set";
    case GUI_event_t::TIMER:
        return "gui timer";
    case GUI_event_t::TEXT_ROLL:
        return "text roll base tick";
    case GUI_event_t::MESSAGE:
        return "message notification";
    case GUI_event_t::MEDIA:
        return "Marlin media changed";
    case GUI_event_t::GUI_STARTUP:
        return "gui startup";
    case GUI_event_t::CHILD_CHANGED:
        return "child changed";
    case GUI_event_t::REINIT_FOOTER:
        return "footers items invalid";
    case GUI_event_t::TOUCH:
        return "touch";
    case GUI_event_t::HEADER_COMMAND:
        return "header command";
    }

    return "error bad index";
}

// forward declarations
class window_t;
template <class Base>
struct AddSuperWindow;

// class to lock an event
// hasprivate ctor - only friend (AddSuperWindow or base window_t) can create lock and call locked methods
// also provides trace
class EventLock {
    EventLock(const char *event_method_name, window_t *sender, GUI_event_t event); // ctor must be private
    template <class T>
    friend struct AddSuperWindow;
    friend class window_t;
};
