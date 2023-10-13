#pragma once

#include "esp_frame.hpp"
#include "window_icon.hpp"
#include "window_text.hpp"
#include "window_wizard_progress.hpp"

#define ESP_FilesString "[0 / 0]"

class ESPFrameProgress : public AddSuperWindow<ESPFrame> {
    char progr_text[sizeof(ESP_FilesString)] = ESP_FilesString;
    window_text_t text_top;
    window_wizard_progress_t progress;
    window_text_t text_progress;
    window_icon_t icon;
    window_text_t text_bot;

protected:
    virtual void change() override;

public:
    ESPFrameProgress(window_t *parent, PhasesESP ph, fsm::PhaseData data);
};
