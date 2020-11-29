/**
 * @file screen.cpp
 * @author Radek Vana
 * @date 2020-11-29
 */

#include "screen.hpp"

screen_t::screen_t(window_t *parent, Rect16 rect, win_type_t type, is_closed_on_timeout_t timeout, is_closed_on_serial_t serial)
    : AddSuperWindow<window_frame_t>(parent, rect, type, timeout, serial)
    , captured_normal_window(nullptr)
    , first_dialog(nullptr)
    , last_dialog(nullptr)
    , first_strong_dialog(nullptr)
    , last_strong_dialog(nullptr)
    , first_popup(nullptr)
    , last_popup(nullptr) {}
