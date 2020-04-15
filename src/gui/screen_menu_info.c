// screen_menu_info.c

#include "gui.h"
#include "screen_menu.h"

extern screen_t screen_sysinfo;
extern screen_t screen_version_info;
extern screen_t screen_qr_info;
extern screen_t screen_qr_error;

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
    MI_VERSIONS,
    MI_COUNT
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
#ifdef _DEBUG
    { { "Send Info by QR", 0, WI_LABEL }, &screen_qr_info },
    { { "QR test", 0, WI_LABEL }, &screen_qr_error },
#endif //_DEBUG
};

//"C inheritance" of screen_menu_data_t with data items
#pragma pack(push)
#pragma pack(1)

typedef struct
{
    screen_menu_data_t base;
    menu_item_t items[MI_COUNT];

} this_screen_data_t;

#pragma pack(pop)

void screen_menu_info_init(screen_t *screen) {
    screen_menu_init(screen, "INFO", ((this_screen_data_t *)screen->pdata)->items, MI_COUNT, 1, 0);
    psmd->items[MI_RETURN] = menu_item_return;
    memcpy(psmd->items + 1, _menu_info_items, (MI_COUNT - 1) * sizeof(menu_item_t));
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
    sizeof(this_screen_data_t), //data_size
    0,                          //pdata
};

const screen_t *pscreen_menu_info = &screen_menu_info;
