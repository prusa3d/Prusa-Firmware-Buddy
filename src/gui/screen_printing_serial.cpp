#include "screen_printing_serial.hpp"
#include "marlin_client.hpp"
#include "i18n.h"
#include "ScreenHandler.hpp"
#include "window_icon.hpp"
#include "screen_menu_tune.hpp"
#include "img_resources.hpp"
#include <serial_printing.hpp>

screen_printing_serial_data_t::screen_printing_serial_data_t()
    : ScreenPrintingModel(_(caption))
    , octo_icon(this, Rect16((display::GetW() - img::serial_printing_172x138.w) / 2, GuiDefaults::RectScreenBody.Top(), img::serial_printing_172x138.w, img::serial_printing_172x138.h), &img::serial_printing_172x138)
    , last_tick(0)
    , connection(connection_state_t::connected)
    , last_state(marlin_server::State::Aborted) {
    ClrMenuTimeoutClose();
    SetOnSerialClose();

    octo_icon.Disable();
    octo_icon.Unshadow();

    SetButtonIconAndLabel(BtnSocket::Right, BtnRes::Disconnect, LabelRes::Stop);
}

void screen_printing_serial_data_t::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    marlin_server::State state = marlin_vars()->print_state;

    if (state != last_state) {
        switch (state) {
        case marlin_server::State::Paused:
            // print is paused -> show resume button
            SetButtonIconAndLabel(BtnSocket::Middle, BtnRes::Resume, LabelRes::Resume);
            EnableButton(BtnSocket::Middle);
            break;
        case marlin_server::State::Printing:
            // print is running -> show pause button
            SetButtonIconAndLabel(BtnSocket::Middle, BtnRes::Pause, LabelRes::Pause);
            EnableButton(BtnSocket::Middle);
            break;
        default:
            // Any other state means printer pausing or resuming, so just wait for this to finish
            DisableButton(BtnSocket::Middle);
            break;
        }

        last_state = state;
    }

    ScreenPrintingModel::windowEvent(sender, event, param);
}

void screen_printing_serial_data_t::tuneAction() {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuTune>);
}

void screen_printing_serial_data_t::pauseAction() {
    // pause or resume button, depenging on what state we are in
    marlin_server::State state = marlin_vars()->print_state;
    switch (state) {
    case marlin_server::State::Paused:
        marlin_client::print_resume();
        break;
    case marlin_server::State::Printing:
        marlin_client::print_pause();
        break;
    default:
        break;
    }
}

void screen_printing_serial_data_t::stopAction() {
    if (MsgBoxWarning(_("Are you sure to stop this printing?"), Responses_YesNo, 1)
        != Response::Yes) {
        return;
    }

    // abort print, disable button and wait for screen to close from marlin server
    marlin_client::print_abort();
    DisableButton(BtnSocket::Right);
}
