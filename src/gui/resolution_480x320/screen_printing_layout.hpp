/**
 * @file screen_printing_layout.hpp
 */
#pragma once
#include "img_resources.hpp"
#include "window_icon.hpp"

inline constexpr uint16_t btn_padding = 90;
inline constexpr uint16_t btn_spacing = 30;
inline constexpr WindowMultiIconButton::Pngs icon_resources[] = {
    { img::settings_80x80, img::settings_80x80_focused, img::settings_80x80_disabled },
    { img::pause_80x80, img::pause_80x80_focused, img::pause_80x80_disabled },
    { img::stop_80x80, img::stop_80x80_focused, img::stop_80x80_disabled },
    { img::resume_80x80, img::resume_80x80_focused, img::resume_80x80_disabled },
    { img::home_80x80, img::home_80x80_focused, img::home_80x80_disabled },
    { img::reprint_80x80, img::reprint_80x80_focused, img::reprint_80x80_disabled },
    { img::disconnect_80x80, img::disconnect_80x80_focused, img::disconnect_80x80_disabled },
    { img::set_ready_80x80, img::set_ready_80x80_focused, img::set_ready_80x80_disabled },
};
