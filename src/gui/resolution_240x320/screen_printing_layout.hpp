/**
 * @file screen_printing_layout.hpp
 */
#pragma once
#include "png_resources.hpp"

static constexpr uint16_t btn_padding = 8;
static constexpr uint16_t btn_spacing = 16;
static const WindowMultiIconButton::Pngs icon_resources[] = {
    { png::settings_64x64, png::settings_64x64_focused, png::settings_64x64_disabled },
    { png::pause_64x64, png::pause_64x64_focused, png::pause_64x64_disabled },
    { png::stop_64x64, png::stop_64x64_focused, png::stop_64x64_disabled },
    { png::resume_64x64, png::resume_64x64_focused, png::resume_64x64_disabled },
    { png::home_64x64, png::home_64x64_focused, png::home_64x64_disabled },
    { png::reprint_64x64, png::reprint_64x64_focused, png::reprint_64x64_disabled },
    { png::disconnect_64x64, png::disconnect_64x64_focused, png::disconnect_64x64_disabled }
};
