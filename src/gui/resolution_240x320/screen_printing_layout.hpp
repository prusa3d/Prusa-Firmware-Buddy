/**
 * @file screen_printing_layout.hpp
 */
#pragma once
#include "img_resources.hpp"

static constexpr uint16_t btn_padding = 8;
static constexpr uint16_t btn_spacing = 16;
static const WindowMultiIconButton::Pngs icon_resources[] = {
    { img::settings_64x64, img::settings_64x64_focused, img::settings_64x64_disabled },
    { img::pause_64x64, img::pause_64x64_focused, img::pause_64x64_disabled },
    { img::stop_64x64, img::stop_64x64_focused, img::stop_64x64_disabled },
    { img::resume_64x64, img::resume_64x64_focused, img::resume_64x64_disabled },
    { img::home_64x64, img::home_64x64_focused, img::home_64x64_disabled },
    { img::reprint_64x64, img::reprint_64x64_focused, img::reprint_64x64_disabled },
    { img::disconnect_64x64, img::disconnect_64x64_focused, img::disconnect_64x64_disabled }
};
