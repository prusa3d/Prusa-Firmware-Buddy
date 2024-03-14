#include "DialogConnectReg.hpp"
#include "RAII.hpp"
#include "../img_resources.hpp"
#include "../ScreenHandler.hpp"
#include "../lang/i18n.h"
#include "bsod_gui.hpp"
#include <connect/connect.hpp>

using connect_client::ConnectionStatus;
using connect_client::OnlineStatus;
using std::get;

namespace {

const PhaseResponses dlg_responses = { Response::Continue, Response::_none, Response::_none, Response::_none };
// TODO: How does this thing get translated/marked for translation?
const PhaseTexts dlg_texts = { { N_("Leave") } };

const constexpr size_t max_url_len = 128;

} // namespace

DialogConnectRegister::DialogConnectRegister()
    : AddSuperWindow<IDialog>(WizardDefaults::RectSelftestFrame)
    , header(this, _(headerLabel))
    , icon_phone(this, Positioner::phoneIconRect(), &img::hand_qr_59x72)
    , qr(this, Positioner::qrcodeRect(), "")
    , title(this, Positioner::textRectTitle(), is_multiline::no)
    , line(this, Positioner::lineRect())
    , text_state(this, Positioner::textRectState(), is_multiline::yes)
    , text_attempt(this, Positioner::textRectAttempt(), is_multiline::yes)
    , text_detail(this, Positioner::textRectDetail(), is_multiline::yes)
    , button(this, WizardDefaults::RectRadioButton(0), dlg_responses, &dlg_texts) {

    text_state.SetAlignment(Align_t::Center());
    text_attempt.SetAlignment(Align_t::Center());
    text_detail.SetAlignment(Align_t::Center());
    line.SetBackColor(COLOR_ORANGE);

    title.SetText(_("Prusa Connect - printer setup"));

    last_seen_status = std::make_tuple(connect_client::ConnectionStatus::Unknown, connect_client::OnlineError::NoError, std::nullopt);

    text_state.SetText(_("Acquiring registration code, please wait..."));

    char help_buff[20] = { 0 };
    _(attemptTxt).copyToRAM(help_buff, sizeof(help_buff)); // Translation
    snprintf(attempt_buffer, sizeof(attempt_buffer), "%s %d/%d", help_buff, 1, connect_client::Registrator::starting_retries);
    text_attempt.SetText(string_view_utf8::MakeRAM((const uint8_t *)attempt_buffer));
    text_attempt.Invalidate();

    // Show these only after we get the code.
    qr_rect = false;
    qr.Hide();
    icon_phone.Hide();

    connect_client::request_registration();
    CaptureNormalWindow(button);
}

DialogConnectRegister::~DialogConnectRegister() {
    if (!left_registration) {
        connect_client::leave_registration();
    }
}

void DialogConnectRegister::Show() {
    DialogConnectRegister dialog;
    dialog.MakeBlocking();
}

void DialogConnectRegister::windowEvent(EventLock, window_t *sender, GUI_event_t event, void *param) {
    if (event_in_progress) {
        return;
    }

    AutoRestore avoid_recursion(event_in_progress, true);

    switch (event) {
    case GUI_event_t::CHILD_CLICK: {
        // We have a single button, so this simplification should work fine.

        bool close = true;

        switch (get<0>(last_seen_status)) {
        case ConnectionStatus::RegistrationRequesting:
        case ConnectionStatus::RegistrationCode:
            close = MsgBoxWarning(_("Prusa Connect setup is not finished. Do you want to exit and abort the process?"), Responses_YesNo)
                == Response::Yes;
            break;
        default:
            break;
        }

        if (close) {
            Screens::Access()->Close();
        }
    } break;
    case GUI_event_t::LOOP: {
        const OnlineStatus status = connect_client::last_status();
        if (status != last_seen_status) {
            last_seen_status = status;

            switch (get<0>(last_seen_status)) {
            case ConnectionStatus::RegistrationCode: {
                const char *code = connect_client::registration_code();
                char url_buffer[max_url_len + 1];
                // Note: the URL hardcoded for production instance. This is
                // because the hostname for the printer is different from the
                // hostname for the user (because of certificates...)
                //
                // In case the user is not using the production instance, they
                // already need the ini file to override the hostname and
                // therefore the wizard is of little use to them.
                showQR();
                snprintf(url_buffer, sizeof url_buffer, "https://connect.prusa3d.com/add/%s", code);
                qr.SetText(url_buffer);
                // The MakeRAM doesn't copy it, it just passes the pointer
                // through and assumes the data live for long enough.
                //
                // This is OK here, as the registration_code is stable and not
                // changing until we leave the registration, which we do in our
                // destructor.
                text_detail.SetText(string_view_utf8::MakeRAM(reinterpret_cast<const uint8_t *>(code)));

#ifdef USE_ST7789
                text_state.SetText(_("Scan QR or visit prusa.io/add, log in and add printer code:"));
#else /*USE_ST7789*/
                text_state.SetText(_("1. Scan the QR code or visit prusa.io/add.\n2. Log in.\n3. Add printer with code:\n"));
#endif /*USE_ST7789*/
                break;
            }
            case ConnectionStatus::RegistrationDone: {
                hideDetails();
                text_state.SetText(_("Registration successful, continue at connect.prusa3d.com"));
                connect_client::leave_registration();
                left_registration = true;
                break;
            }
            case ConnectionStatus::RegistrationError: {
                const char *err_buffer;
                switch (get<1>(last_seen_status)) {
                case connect_client::OnlineError::Dns:
                    err_buffer = "DNS error";
                    break;
                case connect_client::OnlineError::Connection:
                    err_buffer = "Refused";
                    break;
                case connect_client::OnlineError::Tls:
                    err_buffer = "TLS error";
                    break;
                case connect_client::OnlineError::Auth:
                    err_buffer = "Unauthorized";
                    break;
                case connect_client::OnlineError::Server:
                    err_buffer = "Server error";
                    break;
                case connect_client::OnlineError::Internal:
                    err_buffer = "Internal error";
                    break;
                case connect_client::OnlineError::Network:
                    err_buffer = "Network error";
                    break;
                case connect_client::OnlineError::Confused:
                    err_buffer = "Protocol error";
                    break;
                default:
                    err_buffer = "";
                }

                char url_buffer[15] = { 0 };
                char error_help_buffer[70];
                char error_detail_buffer[30];

                auto error = find_error(ErrCode::ERR_CONNECT_CONNECT_REGISTRATION_FAILED);
                _(error.err_text).copyToRAM(error_help_buffer, sizeof(error_help_buffer)); // Translation
                snprintf(error_buffer, sizeof(error_buffer), "%s %s", error_help_buffer, err_buffer);
                text_state.SetText(string_view_utf8::MakeRAM((const uint8_t *)error_buffer));
                text_state.Invalidate();

                snprintf(url_buffer, sizeof(url_buffer), "prusa.io/%d", static_cast<int>(error.err_code));
                qr.SetText(url_buffer);
                showQR();

                // Detail uses the same buffer from the start so SetText here is obsolete as it does nothing and the text is changed within memory
                _(moreDetailTxt).copyToRAM(error_detail_buffer, sizeof(error_detail_buffer)); // Translation
                snprintf(detail_buffer, sizeof(detail_buffer), "%s:\n%s", error_detail_buffer, url_buffer);
                text_detail.SetText(string_view_utf8::MakeRAM((const uint8_t *)detail_buffer));
                text_detail.Invalidate();
                break;
            }
            default:
                const auto &retries_count { get<2>(last_seen_status) };
                if (retries_count.has_value()) {
                    char help_buff[20] = { 0 };
                    _(attemptTxt).copyToRAM(help_buff, sizeof(help_buff)); // Translation
                    if (get<1>(last_seen_status) != connect_client::OnlineError::NoError) {
                        snprintf(attempt_buffer, sizeof(attempt_buffer), "%s %d/%d", help_buff, (connect_client::Registrator::starting_retries - retries_count.value()), connect_client::Registrator::starting_retries);
                        text_attempt.Invalidate();
                    }
                }
                // Some other state:
                // * Unknown.
                // * Getting the code.
                // * Leftover from normal connect session.
                //
                // For these, we just keep the default.
                break;
            }
        }

        break;
    }
    default:
        SuperWindowEvent(sender, event, param);
        break;
    }
}

void DialogConnectRegister::hideDetails() {
    qr.Hide();
    icon_phone.Hide();
    text_detail.Hide();
}

void DialogConnectRegister::showQR() {
    if (!qr_rect) {
        text_state.Hide();
        text_attempt.Hide();
        text_detail.Hide();
        text_state.SetRect(Positioner::textRectState(true));
        text_detail.SetRect(Positioner::textRectDetail(true));
        text_state.SetAlignment(Align_t::Left());
        text_detail.SetAlignment(Align_t::Left());
        text_state.Show();
        text_detail.Show();
        qr.Show();
        icon_phone.Show();
        qr_rect = true;
    }
}

constexpr Rect16 DialogConnectRegister::Positioner::qrcodeRect() {
#ifndef USE_ST7789
    return Rect16 {
        GuiDefaults::ScreenWidth - WizardDefaults::MarginRight - qrcodeWidth,
        WizardDefaults::row_1 + 5, // place qr under title and underlining
        qrcodeWidth,
        qrcodeHeight
    };
#else /*USE_ST7789*/
    return Rect16 { 160 - qrcodeWidth / 2, 200 - qrcodeHeight / 2, qrcodeWidth, qrcodeHeight };
#endif /*USE_ST7789*/
}

constexpr Rect16 DialogConnectRegister::Positioner::phoneIconRect() {
#ifndef USE_ST7789
    return Rect16 {
        qrcodeRect().Left() - phoneWidth,
        (qrcodeRect().Top() + qrcodeRect().Bottom()) / 2 - phoneHeight / 2,
        phoneWidth,
        phoneHeight
    };
#else /*USE_ST7789*/
    return Rect16 { 20, 165, phoneWidth, phoneHeight };
#endif /*USE_ST7789*/
}

/** @param add_top places rect x pixels under @link{WizardDefaults::row_0}
    @param height specify height of rectangle
    @param width specify width of rectangle
    @returns Rect16 position and size of the text widget
    Rectangle will be left panned to @link{WizardDefaults::col_0}
    */
constexpr Rect16 DialogConnectRegister::Positioner::textRect(int16_t add_top, uint16_t height, uint16_t width) {
    return Rect16 { WizardDefaults::col_0, static_cast<int16_t>(WizardDefaults::row_0 + add_top), width, height };
}

/** @returns Rect16 for title (prusaConnect)*/
constexpr Rect16 DialogConnectRegister::Positioner::textRectTitle() {
#ifndef USE_ST7789
    return textRect();
#else /*USE_ST7789*/
    return Rect16 {}; // Empty, this is not used
#endif /*USE_ST7789*/
}

/** @param final if wizard is done and qr code is to be shown
    @returns Rect16 for static state text (wait/done)*/
constexpr Rect16 DialogConnectRegister::Positioner::textRectState([[maybe_unused]] bool final) {
#ifndef USE_ST7789
    if (final) {
        return textRect(WizardDefaults::row_h * 2, WizardDefaults::txt_h * 4, phoneIconRect().Left() - WizardDefaults::col_0);
    } else {
        return textRect(WizardDefaults::row_h * 2, WizardDefaults::row_h * 2);
    }
#else /*USE_ST7789*/
    return Rect16 { WizardDefaults::col_0, WizardDefaults::row_0, textWidth, WizardDefaults::txt_h * 4 };
#endif /*USE_ST7789*/
}

/** @returns Rect16 for number of attempts*/
constexpr Rect16 DialogConnectRegister::Positioner::textRectAttempt([[maybe_unused]] bool final) {
#ifndef USE_ST7789
    if (final) {
        return textRect(textRectState(final).Bottom() - WizardDefaults::row_0, WizardDefaults::row_h, textRectState(final).Width());
    } else {
        return textRect(textRectState().Bottom());
    }
#else /*USE_ST7789*/
    return Rect16 {};
#endif /*USE_ST7789*/
}

/** @param final if wizard is done and qr code is to be shown
    @returns Rect16 for error details or code/help link on final screen*/
constexpr Rect16 DialogConnectRegister::Positioner::textRectDetail([[maybe_unused]] bool final) {
#ifndef USE_ST7789
    if (final) {
        return textRect(WizardDefaults::RectRadioButton(0).Top() - WizardDefaults::row_0 - WizardDefaults::row_h * 2, WizardDefaults::row_h * 2);
    } else {
        return textRect(WizardDefaults::Y_space - textHeight - WizardDefaults::row_1);
    }

#else /*USE_ST7789*/
    return Rect16 { WizardDefaults::col_0, WizardDefaults::row_0 + WizardDefaults::txt_h * 4, textWidth, WizardDefaults::txt_h * 2 };
#endif /*USE_ST7789*/
}

/** @returns Rect16 for underlining title text */
constexpr Rect16 DialogConnectRegister::Positioner::lineRect() {
#ifndef USE_ST7789
    return Rect16 { WizardDefaults::col_0, WizardDefaults::row_1, textWidth, WizardDefaults::progress_h };
#else /*USE_ST7789*/
    return Rect16 {};
#endif /*USE_ST7789*/
}

/** @returns Rect16 position and size of the link widget */
constexpr Rect16 DialogConnectRegister::Positioner::codeRect() {
#ifndef USE_ST7789
    return Rect16 { WizardDefaults::col_0, WizardDefaults::Y_space - textHeight, phoneIconRect().Left() - WizardDefaults::col_0, textHeight };
#else /*USE_ST7789*/
    return Rect16 { WizardDefaults::col_0, WizardDefaults::row_0 + textLines * WizardDefaults::row_h, textWidth, codeHeight };
#endif /*USE_ST7789*/
}
