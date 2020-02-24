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

//uncomment to use tune button
//#define BUTTON_TUNE 1
#ifdef BUTTON_TUNE
extern screen_t *pscreen_menu_tune;
#endif //BUTTON_TUNE

#pragma pack(push)
#pragma pack(1)

typedef struct
{
    window_frame_t root;

    window_header_t header;
    status_footer_t footer;

    window_text_t w_message; //Messages from onStatusChanged()
    window_icon_t octo_icon;
#ifdef BUTTON_TUNE
    window_icon_t tune_button;
    window_text_t tune_label;
#endif //BUTTON_TUNE
    int last_tick;

} screen_printing_serial_data_t;

#pragma pack(pop)

void screen_printing_serial_init(screen_t *screen);
void screen_printing_serial_done(screen_t *screen);
void screen_printing_serial_draw(screen_t *screen);
int screen_printing_serial_event(screen_t *screen, window_t *window, uint8_t event, void *param);

static const char txt0[] = "Remote printing\nin progress    ";
static const char txt1[] = "Remote printing\nin progress .  ";
static const char txt2[] = "Remote printing\nin progress .. ";
static const char txt3[] = "Remote printing\nin progress ...";

static const char *txts[] = { txt0, txt1, txt2, txt3 };

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
const screen_t *pscreen_printing_serial = &screen_printing_serial;

#define pw ((screen_printing_serial_data_t *)screen->pdata)

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
    //window_set_color_back(id, COLOR_GRAY);
    window_enable(id);
    window_set_icon_id(id, IDR_PNG_serial_printing);
    pw->octo_icon.win.f_enabled = 0;
    pw->octo_icon.win.f_disabled = 0;

    //text
    point_ui16_t pt_txt = font_meas_text(resource_font(IDR_FNT_NORMAL), txt0);
    id = window_create_ptr(WINDOW_CLS_TEXT, root,
        rect_ui16((240 - pt_txt.x) / 2, gui_defaults.msg_box_sz.y + pt_ico.y, 240, 95),
        &(pw->w_message));
    window_set_alignment(id, ALIGN_LEFT_TOP);
    //window_set_padding(id, padding_ui8(0, 2, 0, 2));
    window_set_text(id, txts[0]);

#ifdef BUTTON_TUNE
    id = window_create_ptr(
        WINDOW_CLS_ICON, root,
        rect_ui16(8, 185, 64, 64),
        &(pw->tune_button));
    window_set_color_back(id, COLOR_GRAY);
    window_set_tag(id, BUTTON_TUNE);
    window_enable(id);
    window_set_icon_id(id, IDR_PNG_menu_icon_settings);

    id = window_create_ptr(
        WINDOW_CLS_TEXT, root,
        rect_ui16(0, 196 + 48 + 8, 80, 22),
        &(pw->tune_label));
    pw->tune_label.font = resource_font(IDR_FNT_SMALL);
    window_set_padding(id, padding_ui8(0, 0, 0, 0));
    window_set_alignment(id, ALIGN_CENTER);

    window_set_text(id, "Tune");
#endif //BUTTON_TUNE
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
#ifdef BUTTON_TUNE
    switch ((int)param) {
        //todo click
    case BUTTON_TUNE:
        screen_open(pscreen_menu_tune->id);
        break;
    default:
        break;
    }
#endif //BUTTON_TUNE
    static char lock = 0;
    if (lock)
        return 0;
    lock = 1;

    int now = HAL_GetTick();

    if ((now - pw->last_tick) > 500) {
        pw->last_tick = now;
        static int i = 0;
        ++i;
        i %= (sizeof(txts) / sizeof(txts[0]));
        window_set_text(pw->w_message.win.id, txts[i]);
    }

    lock = 0;
    return 0;
}
