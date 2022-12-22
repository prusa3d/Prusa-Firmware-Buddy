#include "screen_printing_serial.hpp"
#include "config.h"
#include "marlin_client.h"
#include "filament.hpp"
#include "i18n.h"
#include "ScreenHandler.hpp"
#include "screen_menus.hpp"
#include "odometer.hpp"
#include "window_icon.hpp"
#include "screen_menu_tune.hpp"

screen_printing_serial_data_t::screen_printing_serial_data_t()
    : AddSuperWindow<ScreenPrintingModel>(_(caption))
    , octo_icon(this, Rect16((240 - png::serial_printing_172x138.w) / 2, GuiDefaults::RectScreenBody.Top(), png::serial_printing_172x138.w, png::serial_printing_172x138.h), &png::serial_printing_172x138)
    , last_tick(0)
    , connection(connection_state_t::connected) {
    ClrMenuTimeoutClose();
    ClrOnSerialClose(); // don't close on Serial print

    octo_icon.Disable();
    octo_icon.Unshadow();

    initAndSetIconAndLabel(btn_tune, res_tune);
    initAndSetIconAndLabel(btn_pause, res_pause);
    initAndSetIconAndLabel(btn_stop, res_disconnect);
}

void screen_printing_serial_data_t::DisableButton(btn &b) {
    if (!b.ico.IsShadowed()) {
        b.ico.Shadow();
        b.ico.Invalidate();
    }
}

void screen_printing_serial_data_t::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    /// end sequence waiting for empty marlin gcode queue
    /// parking -> cooldown hotend & bed -> turn off print fan
    if (connection == connection_state_t::disconnect) {
        connection = connection_state_t::disconnect_ask;
        if (MsgBoxWarning(_("Really Disconnect?"), Responses_YesNo, 1) == Response::Yes) {

            DisableButton(btn_tune);
            DisableButton(btn_pause);
            DisableButton(btn_stop);

            marlin_gcode("M118 A1 action:disconnect");

            connection = connection_state_t::disconnecting;
        } else {
            connection = connection_state_t::connected;
        }
    }

    if (connection == connection_state_t::disconnecting && marlin_get_gqueue() < 1) {
        connection = connection_state_t::disconnected;
        marlin_gcode("G27 P2");     /// park nozzle and raise Z axis
        marlin_gcode("M104 S0 D0"); /// set temperatures to zero
        marlin_gcode("M140 S0");    /// set temperatures to zero
        marlin_gcode("M107");       /// print fan off.
        Odometer_s::instance().force_to_eeprom();
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
