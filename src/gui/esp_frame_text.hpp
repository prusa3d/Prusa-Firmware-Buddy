#pragma once

#include "esp_frame.hpp"

class ESPFrameText : public AddSuperWindow<ESPFrame> {
    window_text_t text;

protected:
    virtual void change() override;

public:
    ESPFrameText(window_t *parent, PhasesESP ph, fsm::PhaseData data);
};
