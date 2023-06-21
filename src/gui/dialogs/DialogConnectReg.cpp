#include "DialogConnectReg.hpp"
#include "../png_resources.hpp"
#include "../ScreenHandler.hpp"

#include <connect/connect.hpp>

using connect_client::ConnectionStatus;
using connect_client::OnlineStatus;
using std::get;

namespace {

const PhaseResponses dlg_responses = { Response::Continue, Response::_none, Response::_none, Response::_none };
// TODO: How does this thing get translated/marked for translation?
const PhaseTexts dlg_texts = { { N_("Leave") } };

const constexpr size_t max_url_len = 128;

}

bool DialogConnectRegister::DialogShown = false;

DialogConnectRegister::DialogConnectRegister()
    : AddSuperWindow<IDialog>(WizardDefaults::RectSelftestFrame)
    , header(this, _(headerLabel))
    , icon_phone(this, Positioner::phoneIconRect(), &png::hand_qr_59x72)
    , qr(this, Positioner::qrcodeRect(), "")
    , text(this, Positioner::textRect(), is_multiline::yes)
    , code(this, Positioner::codeRect(), is_multiline::no)
    , button(this, WizardDefaults::RectRadioButton(0), dlg_responses, &dlg_texts) {

    DialogShown = true;
    text.SetText(_("Wait please, getting the registration code."));

    // Show these only after we get the code.
    hideDetails();

    connect_client::request_registration();
    CaptureNormalWindow(button);
}

DialogConnectRegister::~DialogConnectRegister() {
    DialogShown = false;

    if (!left_registration) {
        connect_client::leave_registration();
    }
}

void DialogConnectRegister::Show() {
    DialogConnectRegister dialog;
    dialog.MakeBlocking();
}

void DialogConnectRegister::windowEvent(EventLock, window_t *sender, GUI_event_t event, void *param) {
    switch (event) {
    case GUI_event_t::CHILD_CLICK:
        // We have a single button, so this simplification should work fine.
        Screens::Access()->Close();
        break;
    case GUI_event_t::LOOP: {
        const OnlineStatus status = connect_client::last_status();
        if (get<0>(status) != get<0>(last_seen_status)) {
            switch (get<0>(status)) {
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
                snprintf(url_buffer, sizeof url_buffer, "https://connect.prusa3d.com/add/%s", code);
                qr.SetText(url_buffer);
                qr.Show();
                icon_phone.Show();
                this->code.Show();
                // The MakeRAM doesn't copy it, it just passes the pointer
                // through and assumes the data live for long enough.
                //
                // This is OK here, as the registration_code is stable and not
                // changing until we leave the registration, which we do in our
                // destructor.
                this->code.SetText(string_view_utf8::MakeRAM(reinterpret_cast<const uint8_t *>(code)));
                text.SetText(_("1. Scan the QR code or visit prusa.io/add.\n2. Log in.\n3. Add printer with code:"));
                break;
            }
            case ConnectionStatus::RegistrationDone: {
                hideDetails();
                text.SetText(_("Done!"));
                connect_client::leave_registration();
                left_registration = true;
                break;
            }
            case ConnectionStatus::RegistrationError: {
                hideDetails();
                text.SetText(_("Registration failed. Likely a network error. Try again later."));
                break;
            }
            default:
                // Some other state:
                // * Unknown.
                // * Getting the code.
                // * Leftover from normal connect session.
                //
                // For these, we just keep the default.
                break;
            }
        }
        last_seen_status = status;
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
    code.Hide();
}

constexpr Rect16 DialogConnectRegister::Positioner::qrcodeRect() {
    if (GuiDefaults::ScreenWidth > 240) {
        return Rect16 {
            GuiDefaults::ScreenWidth - WizardDefaults::MarginRight - qrcodeWidth,
            WizardDefaults::row_0,
            qrcodeWidth,
            qrcodeHeight
        };
    } else {
        return Rect16 { 160 - qrcodeWidth / 2, 200 - qrcodeHeight / 2, qrcodeWidth, qrcodeHeight };
    }
}

constexpr Rect16 DialogConnectRegister::Positioner::phoneIconRect() {
    if (GuiDefaults::ScreenWidth > 240) {
        return Rect16 {
            qrcodeRect().Left() - phoneWidth,
            (qrcodeRect().Top() + qrcodeRect().Bottom()) / 2 - phoneHeight / 2,
            phoneWidth,
            phoneHeight
        };
    } else {
        return Rect16 { 20, 165, phoneWidth, phoneHeight };
    }
}

/** @returns Rect16 position and size of the text widget */
constexpr Rect16 DialogConnectRegister::Positioner::textRect() {
    if (GuiDefaults::ScreenWidth > 240) {
        return Rect16 { WizardDefaults::col_0, WizardDefaults::row_0, phoneIconRect().Left() - WizardDefaults::col_0, textHeight };
    } else {
        return Rect16 { WizardDefaults::col_0, WizardDefaults::row_0, WizardDefaults::X_space, textHeight };
    }
}

/** @returns Rect16 position and size of the link widget */
constexpr Rect16 DialogConnectRegister::Positioner::codeRect() {
    if (GuiDefaults::ScreenWidth > 240) {
        return Rect16 { WizardDefaults::col_0, WizardDefaults::Y_space - textHeight, phoneIconRect().Left() - WizardDefaults::col_0, textHeight };
    } else {
        return Rect16 { WizardDefaults::col_0, WizardDefaults::row_0 + textLines * WizardDefaults::row_h, WizardDefaults::X_space, codeHeight };
    }
}
