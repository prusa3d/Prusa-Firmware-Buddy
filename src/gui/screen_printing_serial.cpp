#include "screen_printing_serial.hpp"
#include "config.h"
#include "marlin_client.hpp"
#include "filament.hpp"
#include "i18n.h"
#include "ScreenHandler.hpp"
#include "odometer.hpp"
#include "config_features.h"
#include "window_icon.hpp"
#include "screen_menu_tune.hpp"
#include "png_resources.hpp"

#if ENABLED(CRASH_RECOVERY)
    #include "../Marlin/src/feature/prusa/crash_recovery.h"
#endif

screen_printing_serial_data_t::screen_printing_serial_data_t()
    : AddSuperWindow<ScreenPrintingModel>(_(caption))
    , octo_icon(this, Rect16((120 - png::serial_printing_172x69.w) / 2, GuiDefaults::RectScreenBody.Top() + 8, png::serial_printing_172x69.w, png::serial_printing_172x69.h), &png::serial_printing_172x69)
    , w_progress(this, Rect16(10, GuiDefaults::RectScreenBody.Top() + png::serial_printing_172x69.h + 14, GuiDefaults::RectScreen.Width() - 2 * 10, 16))
    , w_progress_txt(this, Rect16(10, GuiDefaults::RectScreenBody.Top() + png::serial_printing_172x69.h + 34, GuiDefaults::RectScreen.Width() - 2 * 10, 30))
    , w_message(this, Rect16(10, GuiDefaults::RectScreenBody.Top() + png::serial_printing_172x69.h + 60, GuiDefaults::RectScreen.Width() - 2 * 10, 20))
    , last_tick(0)
    , connection(connection_state_t::connected) {
    ClrMenuTimeoutClose();
    ClrOnSerialClose(); // don't close on Serial print

    octo_icon.Disable();
    octo_icon.Unshadow();

    SetButtonIconAndLabel(BtnSocket::Right, BtnRes::Disconnect, LabelRes::Disconnect);

    w_message.font = resource_font(IDR_FNT_SMALL);
    w_message.SetAlignment(Align_t::CenterBottom());
    w_message.SetPadding({ 0, 2, 0, 2 });
}

void screen_printing_serial_data_t::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    /// end sequence waiting for empty marlin gcode queue
    /// parking -> cooldown hotend & bed -> turn off print fan
    if (connection == connection_state_t::disconnect) {
        connection = connection_state_t::disconnect_ask;
        if (MsgBoxWarning(_("Really Disconnect?"), Responses_YesNo, 1) == Response::Yes) {

            DisableButton(BtnSocket::Left);
            DisableButton(BtnSocket::Middle);
            DisableButton(BtnSocket::Right);

            marlin_gcode("M118 A1 action:disconnect");

            connection = connection_state_t::disconnecting;
        } else {
            connection = connection_state_t::connected;
        }
    }

    if (connection == connection_state_t::disconnecting && marlin_vars()->gqueue < 1) {
        connection = connection_state_t::disconnected;
        marlin_gcode("G27 P2");     /// park nozzle and raise Z axis
        marlin_gcode("M104 S0 D0"); /// set temperatures to zero
        marlin_gcode("M140 S0");    /// set temperatures to zero
        marlin_gcode("M107");       /// print fan off.
        Odometer_s::instance().force_to_eeprom();
#if ENABLED(CRASH_RECOVERY)
        crash_s.write_stat_to_eeprom();
#endif
        return;
    }
    if (connection == connection_state_t::disconnected) {
        Screens::Access()->Close();
    }

    SuperWindowEvent(sender, event, param);
}

void screen_printing_serial_data_t::tuneAction() {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuTune>);
}

void screen_printing_serial_data_t::pauseAction() {
    marlin_gcode("M118 A1 action:pause");
}

void screen_printing_serial_data_t::stopAction() {
    connection = connection_state_t::disconnect;
}
