/**
 * @file selftest_frame_esp_progress.hpp
 * @brief part of screen containing esp selftest (update)
 * this frame contains progress bar
 */

#pragma once

#include "selftest_frame.hpp"
#include "window_icon.hpp"
#include "window_wizard_progress.hpp"

#define ESP_FilesString "[0 / 0]"

class SelftestFrameESP_progress : public AddSuperWindow<SelftestFrameWithRadio> {
    char progr_text[sizeof(ESP_FilesString)] = ESP_FilesString;
    window_text_t text_top;
    window_wizard_progress_t progress;
    window_text_t text_progress;
    window_icon_t icon;
    window_text_t text_bot;

protected:
    virtual void change() override;

public:
    SelftestFrameESP_progress(window_t *parent, PhasesSelftest ph, fsm::PhaseData data);
};
