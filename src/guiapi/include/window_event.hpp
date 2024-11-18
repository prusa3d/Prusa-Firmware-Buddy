// window_event.hpp
#pragma once

#include <bitset>
#include <inttypes.h>

#include <utils/utility_extensions.hpp>

#undef CHANGE /// collision with Arduino macro

// window events
enum class GUI_event_t : uint8_t {
    LOOP = 1, ///< gui loop (every 50ms)
    BTN_DN, ///< button down                ... all windows - not only captured
    BTN_UP, ///< button up                  ... all windows - not only captured
    ENC_CHANGE, ///< value/index encoder change ... all windows - not only captured
    ENC_DN, ///< encoder minus              ... captured window only
    ENC_UP, ///< encoder plus               ... captured window only
    CLICK, ///< clicked (tag > 0)          ... captured window only
    HOLD, ///< held button                ... captured window only
    HELD_LEFT, ///< held and moved left        ... captured window only
    HELD_RIGHT, ///< held and moved right       ... captured window only
    HELD_RELEASED, ///< held and released          ... captured window only
    CHILD_CLICK, ///< click at the child screen
    FOCUS0, ///< focus lost
    FOCUS1, ///< focus set
    CAPT_0, ///< capture lost
    CAPT_1, ///< capture set
    TEXT_ROLL, ///< tick for text rolling classes
    MESSAGE, ///< onStatusChange() message notification
    MEDIA, ///< marlin media change
    GUI_STARTUP, ///< finish splash screen => initialization finish
    CHILD_CHANGED, ///< notify parent about child window change, bahavior depends on implementation
    REINIT_FOOTER, ///< forces reinitialization of all footers in GUI
    TOUCH_CLICK, ///< touch click; captured window only
    TOUCH_SWIPE_UP, ///< touch gesture, single finger swipe; all windows; scroll semantic
    TOUCH_SWIPE_DOWN, ///< touch gesture, single finger swipe; all windows; scroll semantic
    TOUCH_SWIPE_LEFT, ///< touch gesture, single finger swipe; all windows; both swipe left and right have the "back" semantic
    TOUCH_SWIPE_RIGHT, ///< touch gesture, single finger swipe; all windows; both swipe left and right have the "back" semantic
    HEADER_COMMAND, ///< commands header to do something
    _count
};

static constexpr int GUI_event_t_count = ftrstd::to_underlying(GUI_event_t::_count);
using GUI_event_t_bitset = std::bitset<GUI_event_t_count>;

template <typename... Args>
constexpr GUI_event_t_bitset gui_event_t_bitset(Args... args) {
    uint64_t result = 0;
    ((result |= static_cast<uint64_t>(1) << ftrstd::to_underlying(args)), ...);
    return GUI_event_t_bitset(result);
}

static constexpr std::bitset<GUI_event_t_count> GUI_event_IsKnob_data = gui_event_t_bitset(
    GUI_event_t::BTN_DN,
    GUI_event_t::BTN_UP,
    GUI_event_t::ENC_CHANGE);

// lower lever knob events
constexpr bool GUI_event_IsKnob(GUI_event_t event) {
    return GUI_event_IsKnob_data[ftrstd::to_underlying(event)];
}

static constexpr std::bitset<GUI_event_t_count> GUI_event_IsCaptureEv_data = gui_event_t_bitset(
    GUI_event_t::ENC_DN,
    GUI_event_t::ENC_UP,
    GUI_event_t::CLICK,
    GUI_event_t::HOLD,
    GUI_event_t::TOUCH_CLICK);

constexpr bool GUI_event_IsCaptureEv(GUI_event_t event) {
    return GUI_event_IsCaptureEv_data[ftrstd::to_underlying(event)];
}

static constexpr std::bitset<GUI_event_t_count> GUI_event_IsWindowKnobReaction_data = gui_event_t_bitset(
    GUI_event_t::FOCUS0,
    GUI_event_t::FOCUS1,
    GUI_event_t::CAPT_0,
    GUI_event_t::CAPT_1,
    GUI_event_t::CLICK,
    GUI_event_t::HOLD);

constexpr bool GUI_event_IsWindowKnobReaction(GUI_event_t event) {
    return GUI_event_IsWindowKnobReaction_data[ftrstd::to_underlying(event)];
}

static constexpr std::bitset<GUI_event_t_count> GUI_event_is_input_event_data = gui_event_t_bitset(
    GUI_event_t::BTN_DN,
    GUI_event_t::BTN_UP,
    GUI_event_t::ENC_CHANGE,
    GUI_event_t::ENC_DN,
    GUI_event_t::ENC_UP,
    // GUI_event_t::CLICK, click can be raised because of BTN_UP or TOUCH_CLICK, so it's not a direct input event
    GUI_event_t::HOLD,
    GUI_event_t::HELD_LEFT,
    GUI_event_t::HELD_RIGHT,
    GUI_event_t::HELD_RELEASED,
    GUI_event_t::TOUCH_CLICK,
    GUI_event_t::TOUCH_SWIPE_UP,
    GUI_event_t::TOUCH_SWIPE_DOWN,
    GUI_event_t::TOUCH_SWIPE_LEFT,
    GUI_event_t::TOUCH_SWIPE_RIGHT);

constexpr bool GUI_event_is_input_event(GUI_event_t event) {
    return GUI_event_is_input_event_data[static_cast<int>(event)];
}

extern GUI_event_t last_gui_input_event;

static constexpr std::bitset<GUI_event_t_count> GUI_event_is_touch_event_data = gui_event_t_bitset(
    GUI_event_t::TOUCH_CLICK,
    GUI_event_t::TOUCH_SWIPE_UP,
    GUI_event_t::TOUCH_SWIPE_DOWN,
    GUI_event_t::TOUCH_SWIPE_LEFT,
    GUI_event_t::TOUCH_SWIPE_RIGHT);

constexpr bool GUI_event_is_touch_event(GUI_event_t event) {
    return GUI_event_is_touch_event_data[static_cast<int>(event)];
}

constexpr bool GUI_event_IsAnyButLoop(GUI_event_t event) {
    return event != GUI_event_t::LOOP;
}

static constexpr const char *GUI_event_prt_data[GUI_event_t_count + 1] = {
    "-", // 0
    "gui loop",
    "button down",
    "button up",
    "value/index changed",
    "encoder minus",
    "encoder plus",
    "clicked",
    "held button",
    "held and left",
    "held and right",
    "held and released",
    "child click",
    "focus lost",
    "focus set",
    "capture lost",
    "capture set",
    "text roll base tick",
    "message notification",
    "Marlin media changed",
    "gui startup",
    "child changed",
    "footers items invalid",
    "touch click",
    "touch swipe up",
    "touch swipe down",
    "touch swipe left",
    "touch swipe right",
    "header command",
    "-",
};

constexpr const char *GUI_event_prt(GUI_event_t event) {
    return GUI_event_prt_data[static_cast<int>(event)];
}
