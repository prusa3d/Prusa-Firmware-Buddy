//screen_can.c
#include "can.h"
#ifdef HAL_CAN_MODULE_ENABLED

    #include "config.h"
    #include "gui.h"
    #include "status_footer.h"
    #include "math.h"

    #define CHAR_W 12
    #define ROW_H  22
extern screen_t *pscreen_test_term;

    #pragma pack(push)
    #pragma pack(1)

typedef struct
{
    window_frame_t frame;
    window_text_t textMenuName;

    //TX
    window_spin_t text_11bitID;
    window_spin_t spin_11bitID[3];
    window_spin_t spin_data[8][2];
    window_text_t bt_send;

    //RX
    window_text_t text_11bit_filter_list;
    window_spin_t spin_11bit_filter_list[3];
    window_text_t bt_open_rx_term;

    window_text_t textExit;
    uint8_t data[8];

} screen_can_data_t;

    #pragma pack(pop)

    #define pd ((screen_can_data_t *)screen->pdata)

// #define AUTO_TN_DEFAULT_CL COLOR_WHITE
// #define AUTO_TN_ACTIVE_CL  COLOR_RED

enum {
    TAG_QUIT = 10,
    TAG_TX_SEND,
    TAG_RX_TERM
};

static void create_float_spin(window_spin_t *p_spin, int16_t id0, point_ui16_t pt, uint8_t maxval) {
    int16_t id = window_create_ptr(WINDOW_CLS_SPIN, id0, rect_ui16(pt.x, pt.y, CHAR_W, ROW_H), p_spin);
    p_spin->window.win.flg |= WINDOW_FLG_NUMB_FLOAT2INT;
    window_set_format(id, "%X");
    window_set_min_max_step(id, 0.0F, (float)maxval, 1.0F);
    window_set_value(id, 0.0F);
}

//1 byte == max 0xff
static void create_float_spin2digit(window_spin_t *p_spin, int16_t id0, point_ui16_t pt) {
    create_float_spin(&p_spin[1], id0, pt, 15);
    pt.x += CHAR_W;
    create_float_spin(&p_spin[0], id0, pt, 15);
}

void screen_can_init(screen_t *screen) {
    int16_t id;
    uint16_t col = 2;
    uint16_t row2draw = 0;

    int16_t id0 = window_create_ptr(WINDOW_CLS_FRAME,
        -1, rect_ui16(0, 0, 0, 0), &(pd->frame));

    id = window_create_ptr(WINDOW_CLS_TEXT,
        id0, rect_ui16(0, 0, display->w, ROW_H), &(pd->textMenuName));
    pd->textMenuName.font = resource_font(IDR_FNT_BIG);
    window_set_text(id, (const char *)"CAN");

    row2draw = ROW_H;

    //text can ID
    id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(col, row2draw, 180, ROW_H),
        &(pd->text_11bitID));
    window_set_text(id, "11bit ID");
    window_enable(id);

    //ID 11bit 7ff max
    col = 180 + 2;
    create_float_spin(&(pd->spin_11bitID[2]), id0, point_ui16(col, row2draw), 7);
    col += CHAR_W;
    create_float_spin(&(pd->spin_11bitID[1]), id0, point_ui16(col, row2draw), 15);
    col += CHAR_W;
    create_float_spin(&(pd->spin_11bitID[0]), id0, point_ui16(col, row2draw), 15);

    row2draw += ROW_H;

    //TX data
    col = 2;
    create_float_spin2digit(pd->spin_data[0], id0, point_ui16(col, row2draw));
    col += CHAR_W * 2 + 2;
    create_float_spin2digit(pd->spin_data[1], id0, point_ui16(col, row2draw));
    col += CHAR_W * 2 + 2;
    create_float_spin2digit(pd->spin_data[2], id0, point_ui16(col, row2draw));
    col += CHAR_W * 2 + 2;
    create_float_spin2digit(pd->spin_data[3], id0, point_ui16(col, row2draw));
    col += CHAR_W * 2 + 2;
    create_float_spin2digit(pd->spin_data[4], id0, point_ui16(col, row2draw));
    col += CHAR_W * 2 + 2;
    create_float_spin2digit(pd->spin_data[5], id0, point_ui16(col, row2draw));
    col += CHAR_W * 2 + 2;
    create_float_spin2digit(pd->spin_data[6], id0, point_ui16(col, row2draw));
    col += CHAR_W * 2 + 2;
    create_float_spin2digit(pd->spin_data[7], id0, point_ui16(col, row2draw));

    col = 2;
    row2draw += ROW_H;

    //TX button
    id = window_create_ptr(WINDOW_CLS_TEXT,
        id0, rect_ui16(col, row2draw, 68, ROW_H),
        &(pd->bt_send));
    window_set_text(id, "Send");
    window_enable(id);
    window_set_tag(id, TAG_TX_SEND);

    row2draw += ROW_H * 2;
    //RX filter can ID
    id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(col, row2draw, 180, ROW_H),
        &(pd->text_11bit_filter_list));
    window_set_text(id, "RX filter list");
    window_enable(id);

    //ID 11bit 7ff max
    col = 180 + 2;
    create_float_spin(&(pd->spin_11bit_filter_list[2]), id0, point_ui16(col, row2draw), 7);
    col += CHAR_W;
    create_float_spin(&(pd->spin_11bit_filter_list[1]), id0, point_ui16(col, row2draw), 15);
    col += CHAR_W;
    create_float_spin(&(pd->spin_11bit_filter_list[0]), id0, point_ui16(col, row2draw), 15);

    col = 2;
    row2draw += ROW_H;

    //RX button -terminal
    id = window_create_ptr(WINDOW_CLS_TEXT,
        id0, rect_ui16(col, row2draw, 200, ROW_H),
        &(pd->bt_open_rx_term));
    window_set_text(id, "Open RX terminal");
    window_enable(id);
    window_set_tag(id, TAG_RX_TERM);

    //exit
    id = window_create_ptr(WINDOW_CLS_TEXT,
        id0, rect_ui16(180, 290, 60, ROW_H), &(pd->textExit));
    pd->textExit.font = resource_font(IDR_FNT_BIG);
    window_set_text(id, (const char *)"EXIT");
    window_enable(id);
    window_set_tag(id, TAG_QUIT);
}

void screen_can_done(screen_t *screen) {
    window_destroy(pd->frame.win.id);
}

void screen_can_draw(screen_t *screen) {
}

static uint8_t _get_spin_data(screen_t *screen, size_t index) {
    return (window_get_item_index(pd->spin_data[index][1].window.win.id) << 4) + window_get_item_index(pd->spin_data[index][0].window.win.id);
}

int screen_can_event(screen_t *screen, window_t *window, uint8_t event, void *param) {

    if (event == WINDOW_EVENT_CLICK) //buttons
        switch ((int)param) {
        case TAG_QUIT:
            CAN2_Stop();
            screen_close();
            return 1;
        case TAG_TX_SEND:
            CAN2_is_initialized() ? CAN2_Stop() : CAN2_Init();
            //tx id
            CAN2_set_tx_StdId(
                (window_get_item_index(pd->spin_11bitID[2].window.win.id) << 8) + (window_get_item_index(pd->spin_11bitID[1].window.win.id) << 4) + (window_get_item_index(pd->spin_11bitID[0].window.win.id)));
            CAN2_set_rx_filter_deactivate(); //deactivated filter will prevent msg receive
            CAN2_Start();
            for (size_t i = 0; i < 8; ++i)
                pd->data[i] = _get_spin_data(screen, i);
            CAN2_Tx8(pd->data);
            break;
        case TAG_RX_TERM:
            CAN2_is_initialized() ? CAN2_Stop() : CAN2_Init();
            //rx list (expected id)
            CAN2_set_rx_filter_LIST32_STD(
                (window_get_item_index(pd->spin_11bit_filter_list[2].window.win.id) << 8) + (window_get_item_index(pd->spin_11bit_filter_list[1].window.win.id) << 4) + (window_get_item_index(pd->spin_11bit_filter_list[0].window.win.id)));
            CAN2_set_rx_filter_activate();
            CAN2_Start();
            screen_open(pscreen_test_term->id);
            break;
        }

    return 0;
}

screen_t screen_can = {
    0,
    0,
    screen_can_init,
    screen_can_done,
    screen_can_draw,
    screen_can_event,
    sizeof(screen_can_data_t), //data_size
    0,                         //pdata
};

screen_t *const pscreen_can = &screen_can;
#endif //HAL_CAN_MODULE_ENABLED
