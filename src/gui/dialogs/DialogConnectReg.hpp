#pragma once

#include <gui.hpp>
#include <window_header.hpp>
#include <window_qr.hpp>
#include "radio_button.hpp"
#include <guiconfig/wizard_config.hpp>

#include <connect/status.hpp>
#include <connect/registrator.hpp>

class DialogConnectRegister : public IDialog {
private:
    std::array<char, 32> attempt_buffer;
    char detail_buffer[70];
    char error_buffer[90];

    StringViewUtf8Parameters<connect_client::CODE_SIZE + 1> code_params;

    // TODO: Doesn't fit
    constexpr static const char *const headerLabel = N_("PRUSA CONNECT");
    constexpr static const char *const moreDetailTxt = N_("More detail at");

    // TODO: Stolen from selftest_frame_esp_qr.hpp ‒ unify to a common place.
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
        static constexpr size_t textLines { 2 };
        static constexpr size_t textHeight { WizardDefaults::txt_h * textLines };
        static constexpr size_t codeWidth { WizardDefaults::X_space };
        static constexpr size_t codeHeight { WizardDefaults::txt_h };

    public:
        /** @returns Rect16 position and size of QR Code widget */
        static constexpr Rect16 qrcodeRect();

        /** @returns Rect16 position and size of the phone icon widget */
        static constexpr Rect16 phoneIconRect();

        /** @returns Rect16 position and size of the text widget */
        static constexpr Rect16 textRect(int16_t add_top = 0, uint16_t height = WizardDefaults::row_h, uint16_t width = WizardDefaults::X_space);

        static constexpr Rect16 textRectTitle();
        static constexpr Rect16 textRectState(bool final = false);
        static constexpr Rect16 textRectAttempt(bool final = false);
        static constexpr Rect16 textRectDetail(bool final = false);
        static constexpr Rect16 lineRect();

        static constexpr Rect16 codeRect();
    };

    connect_client::OnlineStatus last_seen_status = std::make_tuple(connect_client::ConnectionStatus::Unknown, connect_client::OnlineError::NoError, std::nullopt);
    bool left_registration = false;
    bool qr_rect = false;
    bool event_in_progress = false;

    window_header_t header;
    window_icon_t icon_phone;
    window_qr_t qr;
    window_text_t title;
    window_t line;
    window_text_t text_state;
    window_text_t text_attempt;
    window_text_t text_detail;
    RadioButton button;

    DialogConnectRegister();

    void hideDetails();
    void showQR();

protected:
    virtual void windowEvent(window_t *sender, GUI_event_t event, void *param) override;

public:
    static void Show();
    virtual ~DialogConnectRegister();
};
