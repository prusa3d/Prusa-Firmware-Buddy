/**
 * @file screen_printing_layout.hpp
 */
#pragma once
#include "img_resources.hpp"
#include "window_icon.hpp"

inline constexpr uint16_t btn_padding = 8;
inline constexpr uint16_t btn_spacing = 16;
inline constexpr WindowMultiIconButton::Pngs icon_resources[] = {
    { img::settings_64x64, img::settings_64x64_focused, img::settings_64x64_disabled },
    { img::pause_64x64, img::pause_64x64_focused, img::pause_64x64_disabled },
    { img::stop_64x64, img::stop_64x64_focused, img::stop_64x64_disabled },
    { img::resume_64x64, img::resume_64x64_focused, img::resume_64x64_disabled },
    { img::home_64x64, img::home_64x64_focused, img::home_64x64_disabled },
    { img::reprint_64x64, img::reprint_64x64_focused, img::reprint_64x64_disabled },
    { img::disconnect_64x64, img::disconnect_64x64_focused, img::disconnect_64x64_disabled },
    { img::set_ready_64x64, img::set_ready_64x64_focused, img::set_ready_64x64_disabled },
};
