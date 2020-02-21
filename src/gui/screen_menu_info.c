// screen_menu_info.c

#include "gui.h"
#include "screen_menu.h"

extern screen_t screen_sysinfo;
extern screen_t screen_version_info;

typedef enum {
    MI_RETURN,
#ifdef _DEBUG
    MI_STATISTIC,
#endif //_DEBUG
    MI_SYS_INFO,
#ifdef _DEBUG
    MI_FAIL_STAT,
    MI_SUPPORT,
#endif //_DEBUG
    MI_VERSIONS
} MI_t;

const menu_item_t _menu_info_items[] = {
#ifdef _DEBUG
    { { "Statistic", 0, WI_LABEL | WI_DISABLED }, SCREEN_MENU_NO_SCREEN },
#endif //_DEBUG
    { {
          "System Info",
          0,
          WI_LABEL,
      },
        &screen_sysinfo },
#ifdef _DEBUG
    { { "Fail Stats", 0, WI_LABEL | WI_DISABLED }, SCREEN_MENU_NO_SCREEN },
    { { "Support", 0, WI_LABEL | WI_DISABLED }, SCREEN_MENU_NO_SCREEN },
#endif //_DEBUG
    { {
          "Version Info",
          0,
          WI_LABEL,
      },
        &screen_version_info },
};

void screen_menu_info_init(screen_t *screen) {
    int count = sizeof(_menu_info_items) / sizeof(menu_item_t);
    screen_menu_init(screen, "INFO", count + 1, 1, 0);
    psmd->items[MI_RETURN] = menu_item_return;
    memcpy(psmd->items + 1, _menu_info_items, count * sizeof(menu_item_t));
}

int screen_menu_info_event(screen_t *screen, window_t *window, uint8_t event, void *param) {
    return screen_menu_event(screen, window, event, param);
}

screen_t screen_menu_info = {
    0,
    0,
    screen_menu_info_init,
    screen_menu_done,
    screen_menu_draw,
    screen_menu_info_event,
    sizeof(screen_menu_data_t), //data_size
    0,                          //pdata
};

const screen_t *pscreen_menu_info = &screen_menu_info;
