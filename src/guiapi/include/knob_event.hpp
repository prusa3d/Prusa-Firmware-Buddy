/**
 * @file knob_event.hpp
 * @brief function api for window knob events
 */
#pragma once
#include "window_types.hpp" // BtnState_t

namespace gui::knob {
using action_cb = bool (*)();
using screen_action_cb = void (*)();
void RegisterHeldRightAction(action_cb cb); // action performed by captured window
void RegisterHeldLeftAction(action_cb cb); // action performed by captured window
void RegisterLongPressScreenAction(screen_action_cb cb); // action performed by current screen

action_cb GetHeldRightAction(); // action performed by captured window
action_cb GetHeldLeftAction(); // action performed by captured window
screen_action_cb GetLongPressScreenAction(); // action performed by current screen

bool HeldRightAction();
bool HeldLeftAction();
void LongPressScreenAction();

bool EventEncoder(int diff);
bool EventClick(BtnState_t state);
} // namespace gui::knob
