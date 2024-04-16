/**
 * @file window_wizard_progress.hpp
 * @author Radek Vana
 * @brief Progress bar for wizard
 * @date 2020-12-07
 */

#pragma once
#include "window_progress.hpp"
#include <guiconfig/wizard_config.hpp>

class window_wizard_progress_t : public window_numberless_progress_t {
public:
    window_wizard_progress_t(window_t *parent, Rect16::Top_t top)
        : window_numberless_progress_t(parent, Rect16(WizardDefaults::progress_LR_margin, top, WizardDefaults::progress_width, WizardDefaults::progress_h), COLOR_ORANGE, COLOR_GRAY) {}
};
