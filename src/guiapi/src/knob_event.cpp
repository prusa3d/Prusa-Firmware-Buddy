/**
 * @file knob_event.cpp
 */

#include "knob_event.hpp"
#include "marlin_client.hpp" // marlin_client::notify_server_about_encoder_move
#include "ScreenHandler.hpp" // GetCapturedWindow
#include "sound.hpp"
#include <option/has_side_leds.h>

#if HAS_SIDE_LEDS()
    #include <leds/side_strip_control.hpp>
#endif

using namespace gui::knob;

static action_cb fnc_held_right = nullptr;
static action_cb fnc_held_left = nullptr;
static screen_action_cb fnc_long_press = nullptr;

void gui::knob::RegisterHeldRightAction(action_cb cb) {
    fnc_held_right = cb;
}

void gui::knob::RegisterHeldLeftAction(action_cb cb) {
    fnc_held_left = cb;
}

void gui::knob::RegisterLongPressScreenAction(screen_action_cb cb) {
    fnc_long_press = cb;
}

action_cb gui::knob::GetHeldRightAction() {
    return fnc_held_right;
}

action_cb gui::knob::GetHeldLeftAction() {
    return fnc_held_left;
}

screen_action_cb gui::knob::GetLongPressScreenAction() {
    return fnc_long_press;
}

void gui::knob::LongPressScreenAction() {
    if (fnc_long_press) {
        fnc_long_press();
    }
}

bool gui::knob::HeldRightAction() {
    if (!fnc_held_right) {
        return false;
    }
    return fnc_held_right();
}

bool gui::knob::HeldLeftAction() {
    if (!fnc_held_left) {
        return false;
    }
    return fnc_held_left();
}

bool gui::knob::EventEncoder(int diff) {
    if (diff == 0) {
        return false;
    }

    marlin_client::notify_server_about_encoder_move();
    window_t *capture_ptr = Screens::Access()->Get()->GetCapturedWindow();
    Screens::Access()->ScreenEvent(nullptr, GUI_event_t::ENC_CHANGE, (void *)(intptr_t)diff);

#if HAS_SIDE_LEDS()
    leds::side_strip_control.ActivityPing();
#endif

    if (!capture_ptr) {
        return false;
    }

    if (diff > 0) {
        capture_ptr->WindowEvent(capture_ptr, GUI_event_t::ENC_UP, (void *)(intptr_t)diff);
    } else {
        capture_ptr->WindowEvent(capture_ptr, GUI_event_t::ENC_DN, (void *)(intptr_t)-diff);
    }

    Screens::Access()->ResetTimeout();
    return true;
}

bool gui::knob::EventClick(BtnState_t state) {
    static bool dont_click_on_next_release = false;
    window_t *capture_ptr = Screens::Access()->Get()->GetCapturedWindow();

#if HAS_SIDE_LEDS()
    leds::side_strip_control.ActivityPing();
#endif

    switch (state) {
    case BtnState_t::Pressed:
        Screens::Access()->ScreenEvent(nullptr, GUI_event_t::BTN_DN, 0);
        break;
    case BtnState_t::Released:
        Sound_Play(eSOUND_TYPE::ButtonEcho);
        Screens::Access()->ScreenEvent(nullptr, GUI_event_t::BTN_UP, 0);
        if (!dont_click_on_next_release && capture_ptr) {
            capture_ptr->WindowEvent(capture_ptr, GUI_event_t::CLICK, 0);
        }
        dont_click_on_next_release = false;

        // Only send after the click is processed on the GUI side - marlin might close some open warnigns as a result of user activity,
        // we don't want the user to acidentally interact with something else
        // BFW-6451
        marlin_client::notify_server_about_knob_click();
        break;
    case BtnState_t::Held:
        Sound_Play(eSOUND_TYPE::ButtonEcho);
        break;
    case BtnState_t::HeldAndRight:
        dont_click_on_next_release = true;
        // want to send only to current screen and not send it to all subwindows
        if (gui::knob::HeldRightAction()) {
            Sound_Play(eSOUND_TYPE::ButtonEcho);
        } else {
            Sound_Play(eSOUND_TYPE::StandardAlert);
        }
        break;
    case BtnState_t::HeldAndLeft:
        dont_click_on_next_release = true;
        // want to send only to current screen and not send it to all subwindows
        if (gui::knob::HeldLeftAction()) {
            Sound_Play(eSOUND_TYPE::ButtonEcho);
        } else {
            Sound_Play(eSOUND_TYPE::StandardAlert);
        }
        break;
    case BtnState_t::HeldAndReleased:
        dont_click_on_next_release = true;
        // screen event will not resend this event to all subwindows
        Screens::Access()->ScreenEvent(nullptr, GUI_event_t::HELD_RELEASED, 0);
    }

    Screens::Access()->ResetTimeout();
    return true;
}
