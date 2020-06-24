#include "dbg.h"
#include "gui.h"
#include "config.h"
#include "window_header.h"
#include "status_footer.h"
#include "marlin_client.h"
#include "filament.h"
#include "marlin_server.h"
#include "guitypes.h"      //font_meas_text
#include "stm32f4xx_hal.h" //HAL_GetTick
#include "screens.h"
#include "../lang/i18n.h"

enum class buttons_t {
    TUNE = 0,
    PAUSE,
    DISCONNECT
};

enum class item_id_t {
    tune,
    pause,
    disconnect,
    count // - MAIN COUNT INDEX for asert check
};

const uint16_t serial_printing_icons[static_cast<size_t>(item_id_t::count)] = {
    IDR_PNG_menu_icon_settings,
    IDR_PNG_menu_icon_pause,
    IDR_PNG_menu_icon_disconnect
};

const char *serial_printing_labels[static_cast<size_t>(item_id_t::count)] = {
    "Tune",
    "Pause",
    "Disconnect"
};

struct screen_printing_serial_data_t {
    window_frame_t root;

    window_header_t header;
    status_footer_t footer;

    window_icon_t octo_icon;

    window_icon_t w_buttons[static_cast<size_t>(item_id_t::count)];
    window_text_t w_labels[static_cast<size_t>(item_id_t::count)];

    int last_tick;
    bool disconnect_pressed;
};

void screen_printing_serial_init(screen_t *screen);
void screen_printing_serial_done(screen_t *screen);
void screen_printing_serial_draw(screen_t *screen);
int screen_printing_serial_event(screen_t *screen, window_t *window, uint8_t event, void *param);

#define pw ((screen_printing_serial_data_t *)screen->pdata)

screen_t screen_printing_serial = {
    0,
    0,
    screen_printing_serial_init,
    screen_printing_serial_done,
    screen_printing_serial_draw,
    screen_printing_serial_event,
    sizeof(screen_printing_serial_data_t), //data_size
    0,                                     //pdata
};
extern "C" screen_t *const get_scr_printing_serial() { return &screen_printing_serial; }

static void set_icon_and_label(item_id_t id_to_set, int16_t btn_id, int16_t lbl_id) {
    size_t index_id = static_cast<size_t>(id_to_set);
    if (window_get_icon_id(btn_id) != serial_printing_icons[index_id])
        window_set_icon_id(btn_id, serial_printing_icons[index_id]);
    //compare pointers to text, compare texts would take too long
    if (window_get_text(lbl_id) != serial_printing_labels[index_id])
        window_set_text(lbl_id, serial_printing_labels[index_id]);
}

void screen_printing_serial_init(screen_t *screen) {
    pw->disconnect_pressed = false;
    pw->last_tick = 0;
    int16_t id;

    int16_t root = window_create_ptr(WINDOW_CLS_FRAME, -1,
        rect_ui16(0, 0, 0, 0),
        &(pw->root));
    id = window_create_ptr(WINDOW_CLS_HEADER, root, gui_defaults.header_sz, &(pw->header));
    p_window_header_set_icon(&(pw->header), IDR_PNG_status_icon_printing);
    p_window_header_set_text(&(pw->header), "SERIAL PRT.");

    //octo icon
    point_ui16_t pt_ico = icon_meas(resource_ptr(IDR_PNG_serial_printing));
    id = window_create_ptr(
        WINDOW_CLS_ICON, root,
        rect_ui16((240 - pt_ico.x) / 2, gui_defaults.scr_body_sz.y, pt_ico.x, pt_ico.y),
        &(pw->octo_icon));
    window_enable(id);
    window_set_icon_id(id, IDR_PNG_serial_printing);
    pw->octo_icon.win.f_enabled = 0;
    pw->octo_icon.win.f_disabled = 0;

    for (unsigned int col = 0; col < static_cast<size_t>(item_id_t::count); col++) {
        id = window_create_ptr(
            WINDOW_CLS_ICON, root,
            rect_ui16(8 + (15 + 64) * col, 185, 64, 64),
            &(pw->w_buttons[col]));
        window_set_color_back(id, COLOR_GRAY);
        window_set_tag(id, col + 1);
        window_enable(id);

        id = window_create_ptr(
            WINDOW_CLS_TEXT, root,
            rect_ui16(80 * col, 196 + 48 + 8, 80, 22),
            &(pw->w_labels[col]));
        pw->w_labels[col].font = resource_font(IDR_FNT_SMALL);
        window_set_padding(id, padding_ui8(0, 0, 0, 0));
        window_set_alignment(id, ALIGN_CENTER);
    }

    // -- CONTROLS
    window_icon_t *sp_button;
    // -- tune button
    static_assert(static_cast<size_t>(buttons_t::TUNE) < static_cast<size_t>(item_id_t::count), "buttons_t::TUNE not in range of buttons array");
    sp_button = &pw->w_buttons[static_cast<size_t>(buttons_t::TUNE)];
    set_icon_and_label(item_id_t::tune, sp_button->win.id, pw->w_labels[static_cast<size_t>(buttons_t::TUNE)].win.id);
    // -- pause
    static_assert(static_cast<size_t>(buttons_t::PAUSE) < static_cast<size_t>(item_id_t::count), "PAUSE not in range of buttons array");
    sp_button = &pw->w_buttons[static_cast<size_t>(buttons_t::PAUSE)];
    set_icon_and_label(item_id_t::pause, sp_button->win.id, pw->w_labels[static_cast<size_t>(buttons_t::PAUSE)].win.id);
    // -- disconnect
    static_assert(static_cast<size_t>(buttons_t::DISCONNECT) < static_cast<size_t>(item_id_t::count), "DISCONNECT not in range of buttons array");
    sp_button = &pw->w_buttons[static_cast<size_t>(buttons_t::DISCONNECT)];
    set_icon_and_label(item_id_t::disconnect, sp_button->win.id, pw->w_labels[static_cast<size_t>(buttons_t::DISCONNECT)].win.id);

    status_footer_init(&(pw->footer), root);
}

void screen_printing_serial_done(screen_t *screen) {
    window_destroy(pw->root.win.id);
}

void screen_printing_serial_draw(screen_t *screen) {
}

static void disable_button(screen_t *screen, buttons_t b) {
    window_icon_t *p_button = &pw->w_buttons[static_cast<size_t>(b)];
    if (!p_button->win.f_disabled) {
        p_button->win.f_disabled = 1;
        window_invalidate(p_button->win.id);
    }
}

int screen_printing_serial_event(screen_t *screen, window_t *window, uint8_t event, void *param) {
    window_header_events(&(pw->header));

    /// end sequence waiting for empty marlin gcode queue
    /// parking -> cooldown hotend & bed -> turn off print fan
    if (pw->disconnect_pressed && marlin_get_gqueue() < 1) {
        marlin_gcode("G27 P2");     /// park nozzle and raise Z axis
        marlin_gcode("M104 S0 D0"); /// set temperatures to zero
        marlin_gcode("M140 S0");    /// set temperatures to zero
        marlin_gcode("M107");       /// print fan off
        screen_close();
        return 1;
    }

    if (status_footer_event(&(pw->footer), window, event, param)) {
        return 1;
    }
    if (event != WINDOW_EVENT_CLICK) {
        return 0;
    }

    int p = reinterpret_cast<int>(param) - 1;
    switch (static_cast<buttons_t>(p)) {
    case buttons_t::TUNE:
        screen_open(get_scr_menu_tune()->id);
        return 1;
        break;
    case buttons_t::PAUSE:
        marlin_gcode("M118 A1 action:pause");
        return 1;
        break;
    case buttons_t::DISCONNECT:
        if (gui_msgbox(_("Really Disconnect?"), MSGBOX_BTN_YESNO | MSGBOX_ICO_WARNING | MSGBOX_DEF_BUTTON1) == MSGBOX_RES_YES) {
            pw->disconnect_pressed = true;

            disable_button(screen, buttons_t::TUNE);
            disable_button(screen, buttons_t::PAUSE);
            disable_button(screen, buttons_t::DISCONNECT);

            marlin_gcode("M118 A1 action:disconnect");
        }
        return 1;
        break;
    }

    return 0;
}
