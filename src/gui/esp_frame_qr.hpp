#pragma once

#include "esp_frame.hpp"
#include "window_icon.hpp"
#include "window_qr.hpp"
#include "window_wizard_progress.hpp"

#include "printers.h"

class ESPFrameQR : public AddSuperWindow<ESPFrame> {
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
        static constexpr size_t textHeight { WizardDefaults::txt_h * 4 };

    public:
        /** @returns Rect16 position and size of QR Code widget */
        static constexpr Rect16 qrcodeRect();

        /** @returns Rect16 position and size of the phone icon widget */
        static constexpr Rect16 phoneIconRect();

        /** @returns Rect16 position and size of the text widget */
        static constexpr Rect16 textRect();

        /** @returns Rect16 position and size of the link widget */
        static constexpr Rect16 linkRect();
    };

    window_text_t text;
    window_text_t link;
    window_icon_t icon_phone;
    window_qr_t qr;

protected:
    virtual void change() override;

private:
#if PRINTER_IS_PRUSA_MINI
    static auto constexpr QR_ADDR = "prusa.io/wifiminiqr";
    static auto constexpr ADDR_IN_TEXT = "prusa.io/wifimini";
#elif PRINTER_IS_PRUSA_MK4 || PRINTER_IS_PRUSA_MK3_5 || PRINTER_IS_PRUSA_iX
    static auto constexpr QR_ADDR = "prusa.io/wifimk4qr";
    static auto constexpr ADDR_IN_TEXT = "prusa.io/wifimk4";
#elif PRINTER_IS_PRUSA_XL
    static auto constexpr QR_ADDR = "prusa.io/wifixlqr";
    static auto constexpr ADDR_IN_TEXT = "prusa.io/wifixl";
#else
    #error "WIFI not supported"
#endif

public:
    ESPFrameQR(window_t *parent, PhasesESP ph, fsm::PhaseData data);
};
