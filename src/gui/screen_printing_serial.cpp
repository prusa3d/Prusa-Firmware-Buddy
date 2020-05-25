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

#define BUTTON_TUNE       0
#define BUTTON_PAUSE      1
#define BUTTON_DISCONNECT 2

#pragma pack(push)

enum item_id_t {
    iid_tune,
    iid_pause,
    // iid_stop,
    // iid_resume,
    iid_disconnect,
    iid_count
};

const uint16_t serial_printing_icons[iid_count] = {
    IDR_PNG_menu_icon_settings,
    IDR_PNG_menu_icon_pause,
    // IDR_PNG_menu_icon_stop,
    // IDR_PNG_menu_icon_resume,
    IDR_PNG_menu_icon_stop // disconnect
};

const char *serial_printing_labels[iid_count] = {
    "Tune",
    "Pause",
    // "Stop",
    // "Resume",
    "Disconnect"
};

#pragma pack(push, 1)

typedef struct
{
    window_frame_t root;

    window_header_t header;
    status_footer_t footer;

    window_text_t w_message; //Messages from onStatusChanged()
    window_icon_t octo_icon;

    window_icon_t w_buttons[3];
    window_text_t w_labels[3];
    int last_tick;

} screen_printing_serial_data_t;

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
    if (window_get_icon_id(btn_id) != serial_printing_icons[id_to_set])
        window_set_icon_id(btn_id, serial_printing_icons[id_to_set]);
    //compare pointers to text, compare texts would take too long
    if (window_get_text(lbl_id) != serial_printing_labels[id_to_set])
        window_set_text(lbl_id, serial_printing_labels[id_to_set]);
}

void screen_printing_serial_init(screen_t *screen) {
    pw->last_tick = 0;
    int16_t id;

    int16_t root = window_create_ptr(WINDOW_CLS_FRAME, -1,
        rect_ui16(0, 0, 0, 0),
        &(pw->root));
    id = window_create_ptr(WINDOW_CLS_HEADER, root,
        rect_ui16(0, 0, 240, 31), &(pw->header));
    p_window_header_set_icon(&(pw->header), IDR_PNG_status_icon_printing);
    p_window_header_set_text(&(pw->header), "SERIAL PRT.");

    //octo icon
    point_ui16_t pt_ico = icon_meas(resource_ptr(IDR_PNG_serial_printing));
    id = window_create_ptr(
        WINDOW_CLS_ICON, root,
        rect_ui16((240 - pt_ico.x) / 2, gui_defaults.msg_box_sz.y, pt_ico.x, pt_ico.y),
        &(pw->octo_icon));
    window_enable(id);
    window_set_icon_id(id, IDR_PNG_serial_printing);
    pw->octo_icon.win.f_enabled = 0;
    pw->octo_icon.win.f_disabled = 0;

    for (unsigned int col = 0; col < 3; col++) {
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
    sp_button = &pw->w_buttons[BUTTON_TUNE];
    set_icon_and_label(iid_tune, sp_button->win.id, pw->w_labels[0].win.id);
    // -- pause
    sp_button = &pw->w_buttons[BUTTON_PAUSE];
    set_icon_and_label(iid_pause, sp_button->win.id, pw->w_labels[1].win.id);
    // -- disconnect
    sp_button = &pw->w_buttons[BUTTON_DISCONNECT];
    set_icon_and_label(iid_disconnect, sp_button->win.id, pw->w_labels[2].win.id);

    status_footer_init(&(pw->footer), root);
}

void screen_printing_serial_done(screen_t *screen) {
    window_destroy(pw->root.win.id);
}

void screen_printing_serial_draw(screen_t *screen) {
}

int screen_printing_serial_event(screen_t *screen, window_t *window, uint8_t event, void *param) {
    window_header_events(&(pw->header));

    if (status_footer_event(&(pw->footer), window, event, param)) {
        return 1;
    }
    if (event != WINDOW_EVENT_CLICK) {
        return 0;
    }

    int p = reinterpret_cast<int>(param) - 1;
    switch (p) {
    case BUTTON_TUNE:
        screen_open(get_scr_menu_tune()->id);
        return 1;
        break;
    case BUTTON_PAUSE:
        marlin_gcode("M118 A1 action:pause");
        return 1;
        break;
    case BUTTON_DISCONNECT:
        marlin_gcode("M118 A1 action:disconnect");
        screen_close();
        return 1;
        break;
    }

    return 0;
}
