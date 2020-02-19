#include "dbg.h"
#include "gui.h"
#include "config.h"
#include "window_header.h"
#include "status_footer.h"
#include "marlin_client.h"
#include "filament.h"
#include "marlin_server.h"


#define COLOR_VALUE_VALID COLOR_WHITE
//#define COLOR_VALUE_INVALID COLOR_YELLOW
#define COLOR_VALUE_INVALID COLOR_WHITE

#pragma pack(push)
#pragma pack(1)

typedef struct
{
    window_frame_t root;

    window_header_t header;
    status_footer_t footer;

    window_text_t w_message; //Messages from onStatusChanged()
    uint32_t message_timer;
    uint8_t message_flag;

} screen_printing_serial_data_t;

#pragma pack(pop)



void screen_printing_serial_init(screen_t *screen);
void screen_printing_serial_done(screen_t *screen);
void screen_printing_serial_draw(screen_t *screen);
int screen_printing_serial_event(screen_t *screen, window_t *window, uint8_t event, void *param);


screen_t screen_printing_serial = {
    0,
    0,
    screen_printing_serial_init,
    screen_printing_serial_done,
    screen_printing_serial_draw,
    screen_printing_serial_event,
    sizeof(screen_printing_serial_data_t), //data_size
    0, //pdata
};
const screen_t *pscreen_printing_serial = &screen_printing_serial;

#define pw ((screen_printing_serial_data_t *)screen->pdata)


void screen_printing_serial_init(screen_t *screen) {
    int16_t id;

    int16_t root = window_create_ptr(WINDOW_CLS_FRAME, -1,
        rect_ui16(0, 0, 0, 0),
        &(pw->root));

    id = window_create_ptr(WINDOW_CLS_HEADER, root,
        rect_ui16(0, 0, 240, 31), &(pw->header));
    //p_window_header_set_icon(&(pw->header), IDR_PNG_status_icon_printing_serial);


    id = window_create_ptr(WINDOW_CLS_TEXT, root,
        rect_ui16(10, 75, 230, 95),
        &(pw->w_message));
    window_set_alignment(id, ALIGN_LEFT_TOP);
    window_set_padding(id, padding_ui8(0, 2, 0, 2));
    window_set_text(id, "No messages");
    window_hide(id);
    pw->message_flag = 0;



    status_footer_init(&(pw->footer), root);
}

void screen_printing_serial_done(screen_t *screen) {
}

void screen_printing_serial_draw(screen_t *screen) {
}


static void close_popup_message(screen_t *screen) {
    window_set_text(pw->w_message.win.id, "");

    window_hide(pw->w_message.win.id);
    pw->message_flag = 0;
}

#ifdef DEBUG_FSENSOR_IN_HEADER
extern int _is_in_M600_flg;
extern uint32_t* pCommand;
#endif

int screen_printing_serial_event(screen_t *screen, window_t *window, uint8_t event, void *param) {
    return 0;
}
