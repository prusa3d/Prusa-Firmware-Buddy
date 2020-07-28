#include "dbg.h"
#include "screen_printing_serial.hpp"
#include "config.h"
#include "marlin_client.h"
#include "filament.h"
#include "marlin_server.h"
#include "guitypes.h"      //font_meas_text
#include "stm32f4xx_hal.h" //HAL_GetTick
#include "../lang/i18n.h"
#include "ScreenHandler.hpp"

//octo icon
static point_ui16_t pt_ico() { return icon_meas(resource_ptr(IDR_PNG_serial_printing)); }

screen_printing_serial_data_t::screen_printing_serial_data_t()
    : IScreenPrinting(
        string_view_utf8::MakeCPUFLASH((const uint8_t *)caption), []() { /*screen_open(get_scr_menu_tune()->id);*/ }, []() { marlin_gcode("M118 A1 action:pause"); }, []() {})
    , octo_icon(this, rect_ui16((240 - pt_ico().x) / 2, gui_defaults.scr_body_sz.y, pt_ico().x, pt_ico().y), IDR_PNG_serial_printing)
    , last_tick(0) {

    octo_icon.SetIdRes(IDR_PNG_serial_printing);
    octo_icon.Disable();
    octo_icon.UnswapBW();

    setIconAndLabel(btn_stop, res_disconnect);
}

void screen_printing_serial_data_t::DisableButton(btn &b) {
    if (!b.ico.IsBWSwapped()) {
        b.ico.SwapBW();
        b.ico.Invalidate();
    }
}

void screen_printing_serial_data_t::windowEvent(window_t *sender, uint8_t event, void *param) {
    header.EventClr();

    /// end sequence waiting for empty marlin gcode queue
    /// parking -> cooldown hotend & bed -> turn off print fan

    if (connection == connection_state_t::disconnect) {
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
        marlin_gcode("G27 P2");     /// park nozzle and raise Z axis
        marlin_gcode("M104 S0 D0"); /// set temperatures to zero
        marlin_gcode("M140 S0");    /// set temperatures to zero
        marlin_gcode("M107");       /// print fan off
        Screens::Access()->Close();
        return;
    }

    IScreenPrinting::windowEvent(sender, event, param);
}

screen_printing_serial_data_t::connection_state_t screen_printing_serial_data_t::connection = screen_printing_serial_data_t::connection_state_t::disconnected;
