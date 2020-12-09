//window_event.hpp
#pragma once

#include <inttypes.h>

//window events
enum class GUI_event_t {
    BTN_DN = 1,  //button down
    BTN_UP,      //button up
    ENC_DN,      //encoder minus
    ENC_UP,      //encoder plus
    FOCUS0,      //focus lost
    FOCUS1,      //focus set
    CAPT_0,      //capture lost
    CAPT_1,      //capture set
    CLICK,       //clicked (tag > 0)
    HOLD,        // held button
    CHANGE,      //value/index changed (tag > 0)
    CHANGING,    //value/index changing (tag > 0)
    LOOP,        //gui loop (every 50ms)
    TIMER,       //gui timer
    TEXT_ROLL,   //tick for text rolling classes
    MESSAGE,     //onStatusChange() message notification
    MEDIA,       // marlin media change
    CHILD_CLICK, //click at the child screen
};

constexpr bool GUI_event_IsKnob(GUI_event_t event) {
    switch (event) {
    case GUI_event_t::BTN_DN:
    case GUI_event_t::BTN_UP:
    case GUI_event_t::ENC_DN:
    case GUI_event_t::ENC_UP:
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
    case GUI_event_t::CHANGE:
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
    case GUI_event_t::BTN_DN:
        return "button down";
    case GUI_event_t::BTN_UP:
        return "button up";
    case GUI_event_t::ENC_DN:
        return "encoder minus";
    case GUI_event_t::ENC_UP:
        return "encoder plus";
    case GUI_event_t::FOCUS0:
        return "focus lost";
    case GUI_event_t::FOCUS1:
        return "focus set";
    case GUI_event_t::CAPT_0:
        return "capture lost";
    case GUI_event_t::CAPT_1:
        return "capture set";
    case GUI_event_t::CLICK:
        return "clicked";
    case GUI_event_t::HOLD:
        return "held button";
    case GUI_event_t::CHANGE:
        return "value/index changed";
    case GUI_event_t::CHANGING:
        return "value/index changing";
    case GUI_event_t::LOOP:
        return "gui loop";
    case GUI_event_t::TIMER:
        return "gui timer";
    case GUI_event_t::TEXT_ROLL:
        return "text roll base tick";
    case GUI_event_t::MESSAGE:
        return "message notification";
    case GUI_event_t::MEDIA:
        return "Marlin media changed";
    case GUI_event_t::CHILD_CLICK:
        return "child click";
    }
    return "error bad index";
}

//forward declarations
class window_t;
template <class Base>
struct AddSuperWindow;

// class to lock an event
// hasprivate ctor - only friend (AddSuperWindow or base window_t) can create lock and call locked methods
// also provides trace
class EventLock {
    EventLock(const char *event_method_name, window_t *sender, GUI_event_t event); //ctor must be private
    template <class T>
    friend class AddSuperWindow;
    friend class window_t;
};
