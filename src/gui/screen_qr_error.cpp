#include "gui.h"
#include "config.h"
#include "screen_menu.h"
#include <stdlib.h>

#include "utils.h"
#include "errors.h"


#pragma pack(push)
#pragma pack(1)

typedef struct
{
     window_frame_t root;
     window_text_t errText;
     window_text_t errDescription;
     bool first_run_flag;
} screen_qr_error_data_t;

#pragma pack(pop)

#define pd ((screen_qr_error_data_t*)screen->pdata)


void screen_menu_qr_error_init(screen_t* screen)
{
     int16_t id, root;

     root = window_create_ptr(WINDOW_CLS_FRAME, -1, rect_ui16(0, 0, 0, 0), &(pd->root));
     window_set_color_back(root, COLOR_RED_ALERT);

     id = window_create_ptr(WINDOW_CLS_TEXT, root, rect_ui16(13, 9, 154, 18), &(pd->errText));
     window_set_color_back(id, COLOR_RED_ALERT);
     pd->errText.font = resource_font(IDR_FNT_BIG);
     window_set_text(id, errors[0].error_text);

     id = window_create_ptr(WINDOW_CLS_TEXT, root, rect_ui16(12, 40, 226, 72), &(pd->errDescription));
     window_set_color_back(id, COLOR_RED_ALERT);
     window_set_text(id, errors[0].error_description);

     pd->first_run_flag=true;
}

void screen_menu_qr_error_draw(screen_t* screen)
{
     char qr_str[MAX_LEN_4QR+1];

     display->fill_rect(rect_ui16(11 ,33, 219, 2), COLOR_WHITE);
     create_path_info(qr_str, 1);
     createQR(qr_str);
}

void screen_menu_qr_error_done(screen_t* screen)
{
     window_destroy(pd->root.win.id);
}

int screen_menu_qr_error_event(screen_t* screen, window_t* window, uint8_t event, void* param)
{
     if ((event == WINDOW_EVENT_CLICK) || (event == WINDOW_EVENT_BTN_DN))
     {
          screen_close();
          return(1);
     }
     if (!pd->first_run_flag)
          return(0);
     pd->first_run_flag=false;
     screen_menu_qr_error_draw(screen);
     return(0);
}


screen_t screen_qr_error =
{
     0,
     0,
     screen_menu_qr_error_init,
     screen_menu_qr_error_done,
     screen_menu_qr_error_draw,
     screen_menu_qr_error_event,
     sizeof(screen_qr_error_data_t), //data_size
     0, //pdata
};

const screen_t* pscreen_qr_error = &screen_qr_error;
