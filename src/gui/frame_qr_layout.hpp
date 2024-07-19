#pragma once

#include <guiconfig/wizard_config.hpp>

/** @brief Calculates the position of individual elements of the frame
 *
 * Resulting layout depends on GuiDefaults(ScreenWidth & ScreenHeight)
 * and WizardDefaults. Can be different on differently sized screens.
 *
 * Layout should be compliant with BFW-2561, but not pixel-perfect.
 *
 * Beware of the cyclic dependencies!
 */
class FrameQRLayout {
    static constexpr size_t qrcodeWidth { 140 };
    static constexpr size_t qrcodeHeight { 140 };
    static constexpr size_t phoneWidth { 64 };
    static constexpr size_t phoneHeight { 82 };
    static constexpr size_t textHeight { WizardDefaults::txt_h * 8 };

public:
    static constexpr Rect16 qrcode_rect() {
        if (GuiDefaults::ScreenWidth > 240) {
            return Rect16 {
                GuiDefaults::ScreenWidth - WizardDefaults::MarginRight - qrcodeWidth,
                WizardDefaults::row_0,
                qrcodeWidth,
                qrcodeHeight
            };
        } else {
            return Rect16 { 160 - qrcodeWidth / 2, WizardDefaults::RectRadioButton(0).Top() - qrcodeHeight - 15, qrcodeWidth, qrcodeHeight };
        }
    }

    /** @returns Rect16 position and size of the phone icon widget */
    static constexpr Rect16 phone_icon_rect() {
        if (GuiDefaults::ScreenWidth > 240) {
            return Rect16 {
                qrcode_rect().Left() - phoneWidth,
                (qrcode_rect().Top() + qrcode_rect().Bottom()) / 2 - phoneHeight / 2,
                phoneWidth,
                phoneHeight
            };
        } else {
            return Rect16 { 20, 165, phoneWidth, phoneHeight };
        }
    }

    /** @returns Rect16 position and size of the text widget */
    static constexpr Rect16 text_rect() {
        if (GuiDefaults::ScreenWidth > 240) {
            return Rect16 { WizardDefaults::col_0, WizardDefaults::row_0, phone_icon_rect().Left() - WizardDefaults::col_0, textHeight };
        } else {
            return Rect16 { WizardDefaults::col_0, WizardDefaults::row_0 + 10, WizardDefaults::X_space, textHeight };
        }
    }

    /** @returns Rect16 position and size of the link widget */
    static constexpr Rect16 link_rect() {
        if (GuiDefaults::ScreenWidth > 240) {
            return Rect16 { WizardDefaults::col_0, WizardDefaults::row_0 + textHeight, WizardDefaults::X_space, WizardDefaults::txt_h };
        } else {
            return Rect16 { WizardDefaults::col_0, qrcode_rect().Bottom() - 5 /* QR is actually smaller than its rect */, WizardDefaults::X_space, WizardDefaults::txt_h };
        }
    }
};
