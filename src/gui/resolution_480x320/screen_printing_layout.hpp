/**
 * @file screen_printing_layout.hpp
 */
#pragma once
#include "png_resources.hpp"

static constexpr uint16_t btn_padding = 90;
static constexpr uint16_t btn_spacing = 30;
static const WindowMultiIconButton::Pngs icon_resources[] = {
    { png::settings_80x80, png::settings_80x80_focused, png::settings_80x80_disabled },
    { png::pause_80x80, png::pause_80x80_focused, png::pause_80x80_disabled },
    { png::stop_80x80, png::stop_80x80_focused, png::stop_80x80_disabled },
    { png::resume_80x80, png::resume_80x80_focused, png::resume_80x80_disabled },
    { png::home_80x80, png::home_80x80_focused, png::home_80x80_disabled },
    { png::reprint_80x80, png::reprint_80x80_focused, png::reprint_80x80_disabled },
    { png::disconnect_80x80, png::disconnect_80x80_focused, png::disconnect_80x80_disabled }
};
