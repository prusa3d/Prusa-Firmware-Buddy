#include "dbg.h"
#include "screen_printing_serial.hpp"
#include "config.h"
#include "window_header.hpp"
#include "status_footer.h"
#include "marlin_client.h"
#include "filament.h"
#include "marlin_server.h"
#include "guitypes.h"      //font_meas_text
#include "stm32f4xx_hal.h" //HAL_GetTick
#include "../lang/i18n.h"
#include "ScreenHandler.hpp"

const uint16_t serial_printing_icons[static_cast<size_t>(buttons_t::count)] = {
    IDR_PNG_menu_icon_settings,
    IDR_PNG_menu_icon_pause,
    IDR_PNG_menu_icon_disconnect
};

const char *serial_printing_labels[static_cast<size_t>(buttons_t::count)] = {
    N_("Tune"),
    N_("Pause"),
    N_("Disconnect")
};

static void set_icon_and_label(buttons_t id_to_set, window_icon_t *p_button, window_text_t *lbl) {
    // This check may also be skipped and set the icon every time
    // - set_icon_and_label is called only from screen_printing_serial_init
    // I don't see a reason why we should compare to some previous state
    size_t index_id = static_cast<size_t>(id_to_set);
    if (p_button->GetIdRes() != serial_printing_icons[index_id])
        p_button->SetIdRes(serial_printing_icons[index_id]);
    // disregard comparing strings - just set the label every time
    lbl->SetText(_(serial_printing_labels[index_id]));
}

//octo icon
static point_ui16_t pt_ico() { return icon_meas(resource_ptr(IDR_PNG_serial_printing)); }

screen_printing_serial_data_t::screen_printing_serial_data_t()
    : window_frame_t()
    , header(this)
    , footer(this)
    , octo_icon(this, rect_ui16((240 - pt_ico().x) / 2, gui_defaults.scr_body_sz.y, pt_ico().x, pt_ico().y), IDR_PNG_serial_printing)
    , w_buttons {
        { this, rect_ui16(8 + (15 + 64) * 0, 185, 64, 64), 0, []() { /*screen_open(get_scr_menu_tune()->id);*/ } },
        { this, rect_ui16(8 + (15 + 64) * 1, 185, 64, 64), 0, []() { marlin_gcode("M118 A1 action:pause"); } },
        { this, rect_ui16(8 + (15 + 64) * 2, 185, 64, 64), 0, []() { connection = connection_state_t::disconnect; } }
    }
    , w_labels { { this, rect_ui16(80 * 0, 196 + 48 + 8, 80, 22) }, { this, rect_ui16(80 * 1, 196 + 48 + 8, 80, 22) }, { this, rect_ui16(80 * 2, 196 + 48 + 8, 80, 22) } }
    , last_tick(0) {

    header.SetIcon(IDR_PNG_status_icon_printing);
    static const char sp[] = "SERIAL PRT.";
    header.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)sp));

    octo_icon.SetIdRes(IDR_PNG_serial_printing);
    octo_icon.Disable();
    octo_icon.UnswapBW();

    for (unsigned int col = 0; col < static_cast<size_t>(buttons_t::count); col++) {
        w_labels[col].font = resource_font(IDR_FNT_SMALL);
        w_labels[col].SetPadding(padding_ui8(0, 0, 0, 0));
        w_labels[col].SetAlignment(ALIGN_CENTER);
    }

    // -- CONTROLS
    window_icon_t *sp_button;
    // -- tune button
    static_assert(static_cast<size_t>(buttons_t::TUNE) < static_cast<size_t>(buttons_t::count), "buttons_t::TUNE not in range of buttons array");
    sp_button = &w_buttons[static_cast<size_t>(buttons_t::TUNE)];
    set_icon_and_label(buttons_t::TUNE, sp_button, &w_labels[static_cast<size_t>(buttons_t::TUNE)]);
    // -- pause
    static_assert(static_cast<size_t>(buttons_t::PAUSE) < static_cast<size_t>(buttons_t::count), "PAUSE not in range of buttons array");
    sp_button = &w_buttons[static_cast<size_t>(buttons_t::PAUSE)];
    set_icon_and_label(buttons_t::PAUSE, sp_button, &w_labels[static_cast<size_t>(buttons_t::PAUSE)]);
    // -- disconnect
    static_assert(static_cast<size_t>(buttons_t::DISCONNECT) < static_cast<size_t>(buttons_t::count), "DISCONNECT not in range of buttons array");
    sp_button = &w_buttons[static_cast<size_t>(buttons_t::DISCONNECT)];
    set_icon_and_label(buttons_t::DISCONNECT, sp_button, &w_labels[static_cast<size_t>(buttons_t::DISCONNECT)]);
}

screen_printing_serial_data_t::~screen_printing_serial_data_t() {
    marlin_gcode("G27 P2"); /// park nozzle and raise Z axis
    marlin_gcode("M86 S1"); /// enable safety timer
}

void screen_printing_serial_data_t::DisableButton(buttons_t b) {
    window_icon_t *p_button = &w_buttons[static_cast<size_t>(b)];
    if (!p_button->IsBWSwapped()) {
        p_button->SwapBW();
        p_button->Invalidate();
    }
}

void screen_printing_serial_data_t::windowEvent(window_t *sender, uint8_t event, void *param) {
    header.EventClr();

    /// end sequence waiting for empty marlin gcode queue
    /// parking -> cooldown hotend & bed -> turn off print fan

    if (connection == connection_state_t::disconnect) {
        if (MsgBoxWarning(_("Really Disconnect?"), Responses_YesNo, 1) == Response::Yes) {

            DisableButton(buttons_t::TUNE);
            DisableButton(buttons_t::PAUSE);
            DisableButton(buttons_t::DISCONNECT);

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

    window_frame_t::windowEvent(sender, event, param);
}

screen_printing_serial_data_t::connection_state_t screen_printing_serial_data_t::connection = screen_printing_serial_data_t::connection_state_t::disconnected;
