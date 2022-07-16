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

#define QR_ADDR         "prusa.io/wifiminiqr"
#define QR_ADDR_IN_TEXT "prusa.io/wifimini"

class SelftestFrameESP_qr : public AddSuperWindow<SelftestFrameWithRadio> {
    class QR : public AddSuperWindow<window_qr_t> {
    public:
        QR(window_t *parent, Rect16 rect);
    };

    window_text_t text;
    window_icon_t icon_phone;
    QR qr;

protected:
    virtual void change() override;

public:
    SelftestFrameESP_qr(window_t *parent, PhasesSelftest ph, fsm::PhaseData data);
};
