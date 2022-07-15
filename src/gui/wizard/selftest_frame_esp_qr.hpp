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

    /** @brief Calculates the position of individual elements of the frame
     *
     * Resulting layout depends on GuiDefaults(ScreenWidth & ScreenHeight)
     * and WizardDefaults. Can be different on differently sized screens.
     *
     * Layout should be compliant with BFW-2561, but not pixel-perfect.
     *
     * Beware of the cyclic dependencies!
     */
    class Positioner {
        static constexpr size_t qrcodeWidth { 140 };
        static constexpr size_t qrcodeHeight { 140 };
        static constexpr size_t phoneWidth { 64 };
        static constexpr size_t phoneHeight { 82 };
        static constexpr size_t textWidth { WizardDefaults::X_space };
        static constexpr size_t textHeight { WizardDefaults::txt_h * 4 };

    public:
        /** @returns Rect16 position and size of QR Code widget */
        static constexpr Rect16 qrcodeRect();

        /** @returns Rect16 position and size of the phone icon widget */
        static constexpr Rect16 phoneIconRect();

        /** @returns Rect16 position and size of the text widget */
        static constexpr Rect16 textRect();
    };

    window_text_t text;
    window_icon_t icon_phone;
    QR qr;

protected:
    virtual void change() override;

public:
    SelftestFrameESP_qr(window_t *parent, PhasesSelftest ph, fsm::PhaseData data);
};
