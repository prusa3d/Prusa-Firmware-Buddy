// screen_tune_info.c

#include "gui.h"
#include "screen_menu.h"

extern screen_t screen_lan_settings;
extern screen_t screen_version_info;

typedef enum {
    MI_RETURN,
    MI_LAN,
    MI_INFO,
    MI_COUNT
} MI_t;

//sorry, unimplemented: non-trivial designated initializers not supported
const menu_item_t _menu_tune_info_items[] = {
    { { "LAN Settings", 0, WI_LABEL }, &screen_lan_settings },
    { { "Version Info", 0, WI_LABEL }, &screen_version_info },
};

#pragma pack(push)
#pragma pack(1)

typedef struct
{
    screen_menu_data_t base;
    menu_item_t items[MI_COUNT];
} this_screen_data_t;

#pragma pack(pop)

void screen_tune_info_init(screen_t *screen) {
    screen_menu_init(screen, "TUNE INFO", ((this_screen_data_t *)screen->pdata)->items, MI_COUNT, 1, 0);
    psmd->items[MI_RETURN] = menu_item_return;
    memcpy(psmd->items + 1, _menu_tune_info_items, (MI_COUNT - 1) * sizeof(menu_item_t));
}

int screen_tune_info_event(screen_t *screen, window_t *window, uint8_t event, void *param) {
    return screen_menu_event(screen, window, event, param);
}

screen_t screen_tune_info = {
    0,
    0,
    screen_tune_info_init,
    screen_menu_done,
    screen_menu_draw,
    screen_tune_info_event,
    sizeof(this_screen_data_t), //data_size
    0,                          //pdata
};

const screen_t *pscreen_tune_info = &screen_tune_info;

