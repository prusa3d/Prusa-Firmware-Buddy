#include "DialogConnectReg.hpp"
#include "RAII.hpp"
#include "../img_resources.hpp"
#include "../ScreenHandler.hpp"
#include "../lang/i18n.h"
#include <find_error.hpp>
#include <connect/connect.hpp>
#include <guiconfig/guiconfig.h>
#include <str_utils.hpp>

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
    : IDialog(WizardDefaults::RectSelftestFrame)
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

    text_state.SetText(_("Registering the printer to Prusa Connect..."));
    text_attempt.Hide();

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
    Screens::Access()->gui_loop_until_dialog_closed();
}

void DialogConnectRegister::windowEvent(window_t *sender, GUI_event_t event, void *param) {
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

                // Note: the URL hardcoded for production instance. This is
                // because the hostname for the printer is different from the
                // hostname for the user (because of certificates...)
                //
                // In case the user is not using the production instance, they
                // already need the ini file to override the hostname and
                // therefore the wizard is of little use to them.

                ArrayStringBuilder<max_url_len> url_sb;
                url_sb.append_string("https://connect.prusa3d.com/add/");
                url_sb.append_string(code);
                qr.SetText(url_sb.str());
                showQR();

                text_detail.SetText(_("Code: %s").formatted(code_params, code));
#if !HAS_MINI_DISPLAY()
                Rect16 adjusted_rect = text_detail.GetRect();
                adjusted_rect += Rect16::Top_t(WizardDefaults::row_h);
                adjusted_rect -= Rect16::Height_t(WizardDefaults::row_h);
                text_detail.SetRect(adjusted_rect);
#endif

#if HAS_MINI_DISPLAY()
                text_state.SetText(_("Scan the QR code using the Prusa app or camera, or visit prusa.io/add"));
#else
                text_state.SetText(_("1. Scan the QR code using the Prusa app or camera, or visit prusa.io/add\n\n2. Log in"));
#endif
                break;
                ;
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

            default: {
                const auto retries_count = get<2>(last_seen_status);
                const auto retry_ix = retries_count.transform([](auto v) { return connect_client::Registrator::starting_retries - v; }).value_or(0);

                // After a few attempts, show the user that we're retrying
                if (retry_ix > 1) {
                    text_attempt.SetText(_("Attempt %d/%d").formatted(attempt_params, retry_ix, connect_client::Registrator::starting_retries));
                    text_attempt.Invalidate();
                    text_attempt.Show();
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
        }

        break;
    }
    default:
        IDialog::windowEvent(sender, event, param);
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
#if !HAS_MINI_DISPLAY()
    return Rect16 {
        GuiDefaults::ScreenWidth - WizardDefaults::MarginRight - qrcodeWidth,
        WizardDefaults::row_1 + 5, // place qr under title and underlining
        qrcodeWidth,
        qrcodeHeight
    };
#else
    return Rect16 { 160 - qrcodeWidth / 2, 200 - qrcodeHeight / 2, qrcodeWidth, qrcodeHeight };
#endif
}

constexpr Rect16 DialogConnectRegister::Positioner::phoneIconRect() {
#if !HAS_MINI_DISPLAY()
    return Rect16 {
        qrcodeRect().Left() - phoneWidth,
        (qrcodeRect().Top() + qrcodeRect().Bottom()) / 2 - phoneHeight / 2,
        phoneWidth,
        phoneHeight
    };
#else
    return Rect16 { 20, 165, phoneWidth, phoneHeight };
#endif
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
#if !HAS_MINI_DISPLAY()
    return textRect();
#else
    return Rect16 {}; // Empty, this is not used
#endif
}

/** @param final if wizard is done and qr code is to be shown
    @returns Rect16 for static state text (wait/done)*/
constexpr Rect16 DialogConnectRegister::Positioner::textRectState([[maybe_unused]] bool final) {
#if !HAS_MINI_DISPLAY()
    if (final) {
        return textRect(WizardDefaults::row_h * 2, WizardDefaults::txt_h * 8, phoneIconRect().Left() - WizardDefaults::col_0);
    } else {
        return textRect(WizardDefaults::row_h * 2, WizardDefaults::row_h * 2);
    }
#else
    return Rect16 { WizardDefaults::col_0, WizardDefaults::row_0, textWidth, WizardDefaults::txt_h * 4 };
#endif
}

/** @returns Rect16 for number of attempts*/
constexpr Rect16 DialogConnectRegister::Positioner::textRectAttempt([[maybe_unused]] bool final) {
#if !HAS_MINI_DISPLAY()
    if (final) {
        return textRect(textRectState(final).Bottom() - WizardDefaults::row_0, WizardDefaults::row_h, textRectState(final).Width());
    } else {
        return textRect(textRectState().Bottom());
    }
#else
    return Rect16 {};
#endif
}

/** @param final if wizard is done and qr code is to be shown
    @returns Rect16 for error details or code/help link on final screen*/
constexpr Rect16 DialogConnectRegister::Positioner::textRectDetail([[maybe_unused]] bool final) {
#if !HAS_MINI_DISPLAY()
    if (final) {
        return textRect(WizardDefaults::RectRadioButton(0).Top() - WizardDefaults::row_0 - WizardDefaults::row_h * 2, WizardDefaults::row_h * 2);
    } else {
        return textRect(WizardDefaults::Y_space - textHeight - WizardDefaults::row_1);
    }

#else
    return Rect16 { WizardDefaults::col_0, WizardDefaults::row_0 + WizardDefaults::txt_h * 4, textWidth, WizardDefaults::txt_h * 2 };
#endif
}

/** @returns Rect16 for underlining title text */
constexpr Rect16 DialogConnectRegister::Positioner::lineRect() {
#if !HAS_MINI_DISPLAY()
    return Rect16 { WizardDefaults::col_0, WizardDefaults::row_1, textWidth, WizardDefaults::progress_h };
#else
    return Rect16 {};
#endif
}

/** @returns Rect16 position and size of the link widget */
constexpr Rect16 DialogConnectRegister::Positioner::codeRect() {
#if !HAS_MINI_DISPLAY()
    return Rect16 { WizardDefaults::col_0, WizardDefaults::Y_space - textHeight, phoneIconRect().Left() - WizardDefaults::col_0, textHeight };
#else
    return Rect16 { WizardDefaults::col_0, WizardDefaults::row_0 + textLines * WizardDefaults::row_h, textWidth, codeHeight };
#endif
}
