/**
 * @file selftest_frame_esp_qr.hpp
 * @brief part of screen containing update of esp credentials
 * this frame contains qr code
 */

#pragma once

#include "selftest_frame.hpp"
#include "window_icon.hpp"
#include "window_qr.hpp"
#include "window_wizard_progress.hpp"

class SelftestFrameESP_qr : public AddSuperWindow<SelftestFrameWithRadio> {
    window_text_t text;
    window_icon_t icon_phone;
    window_qr_t qr;

protected:
    virtual void change() override;

public:
    SelftestFrameESP_qr(window_t *parent, PhasesSelftest ph, fsm::PhaseData data);
};
