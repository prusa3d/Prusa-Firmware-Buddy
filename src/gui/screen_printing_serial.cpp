#include "screen_printing_serial.hpp"
#include "config.h"
#include "marlin_client.h"
#include "filament.hpp"
#include "i18n.h"
#include "ScreenHandler.hpp"
#include "screen_menus.hpp"
#include "odometer.hpp"
#include "window_icon.hpp"

//octo icon, 86x69
static point_ui16_t pt_ico() { return icon_meas(resource_ptr(IDR_PNG_serial_printing)); }

screen_printing_serial_data_t::screen_printing_serial_data_t()
    : AddSuperWindow<ScreenPrintingModel>(_(caption))
    , octo_icon(this, Rect16(120 - pt_ico().x / 2, GuiDefaults::RectScreenBody.Top() + 8, pt_ico().x, pt_ico().y), IDR_PNG_serial_printing)
    , w_progress(this, Rect16(10, GuiDefaults::RectScreenBody.Top() + pt_ico().y + 14, GuiDefaults::RectScreen.Width() - 2 * 10, 16))
    , w_progress_txt(this, Rect16(10, GuiDefaults::RectScreenBody.Top() + pt_ico().y + 34, GuiDefaults::RectScreen.Width() - 2 * 10, 30))
    , w_message(this, Rect16(10, GuiDefaults::RectScreenBody.Top() + pt_ico().y + 60, GuiDefaults::RectScreen.Width() - 2 * 10, 20))
    , last_tick(0)
    , connection(connection_state_t::connected) {
    ClrMenuTimeoutClose();
    ClrOnSerialClose(); // don't close on Serial print

    octo_icon.setFileName("/internal/res/png/screen_printing_serial.png");
    octo_icon.Disable();
    octo_icon.Unshadow();

    w_message.font = resource_font(IDR_FNT_SMALL);
    w_message.SetAlignment(Align_t::CenterBottom());
    w_message.SetPadding({ 0, 2, 0, 2 });

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
    Screens::Access()->Open(GetScreenMenuTune);
}

void screen_printing_serial_data_t::pauseAction() {
    marlin_gcode("M118 A1 action:pause");
}

void screen_printing_serial_data_t::stopAction() {
    connection = connection_state_t::disconnect;
}
