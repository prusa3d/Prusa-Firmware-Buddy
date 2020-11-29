/**
 * @file screen.cpp
 * @author Radek Vana
 * @date 2020-11-29
 */

#include "screen.hpp"

Screen::Screen()
    : AddSuperWindow<window_frame_t>(nullptr, GuiDefaults::RectScreen, win_type_t::normal, is_closed_on_timeout_t::yes, is_closed_on_serial_t::yes)
    , captured_normal_window(nullptr)
    , first_dialog(nullptr)
    , last_dialog(nullptr)
    , first_strong_dialog(nullptr)
    , last_strong_dialog(nullptr)
    , first_popup(nullptr)
    , last_popup(nullptr) {}
