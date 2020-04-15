// screen_menu_filament.c

#include "gui.h"
#include "screen_menu.h"
#include "filament.h"
#include "filament_sensor.h"
#include "marlin_client.h"
#include "menu_vars.h"
#include "window_dlg_load_unload.h"
#include "dbg.h"

#define FKNOWN      0x01 //filament is known
#define F_NOTSENSED 0x02 //filament is not in sensor

extern screen_t screen_menu_preheat;
extern screen_t screen_preheating;

typedef enum {
    MI_RETURN,
    MI_LOAD,
    MI_UNLOAD,
    MI_CHANGE,
    MI_PURGE,
    MI_COUNT
} MI_t;

const menu_item_t _menu_filament_items[] = {
    { { "Load Filament", 0, WI_LABEL }, SCREEN_MENU_NO_SCREEN },
    { { "Unload Filament", 0, WI_LABEL }, SCREEN_MENU_NO_SCREEN },
    { { "Change Filament", 0, WI_LABEL }, SCREEN_MENU_NO_SCREEN },
    { { "Purge Filament", 0, WI_LABEL }, SCREEN_MENU_NO_SCREEN },
};

static void _load_ena(screen_t *screen) {
    psmd->items[MI_LOAD].item.type &= ~WI_DISABLED;
}
static void _change_dis(screen_t *screen) {
    psmd->items[MI_CHANGE].item.type |= WI_DISABLED;
}
static void _change_ena(screen_t *screen) {
    psmd->items[MI_CHANGE].item.type &= ~WI_DISABLED;
}

static void _deactivate_item(screen_t *screen) {

    uint8_t filament = 0;
    filament |= get_filament() != FILAMENT_NONE ? FKNOWN : 0;
    filament |= fs_get_state() == FS_NO_FILAMENT ? F_NOTSENSED : 0;
    switch (filament) {
    case FKNOWN: //known and not "unsensed" - do not allow load
        _change_ena(screen);
        break;
    case FKNOWN | F_NOTSENSED: //allow both load and change
        _load_ena(screen);
        _change_ena(screen);
        break;
    case F_NOTSENSED: //allow load
    case 0:           //filament is not known but is sensed == most likely same as F_NOTSENSED, but user inserted filament into sensor
    default:
        _load_ena(screen);
        _change_dis(screen);
        break;
    }
}

//"C inheritance" of screen_menu_data_t with data items
#pragma pack(push)
#pragma pack(1)

typedef struct
{
    screen_menu_data_t base;
    menu_item_t items[MI_COUNT];

} this_screen_data_t;

#pragma pack(pop)

void screen_menu_filament_init(screen_t *screen) {
    //filament_not_loaded = -1;
    screen_menu_init(screen, "FILAMENT", ((this_screen_data_t *)screen->pdata)->items, MI_COUNT, 1, 0);
    psmd->items[0] = menu_item_return;
    memcpy(psmd->items + 1, _menu_filament_items, (MI_COUNT - 1) * sizeof(menu_item_t));
    _deactivate_item(screen);
}

int screen_menu_filament_event(screen_t *screen, window_t *window, uint8_t event, void *param) {
    _deactivate_item(screen);
    if (event == WINDOW_EVENT_CLICK)
        if (!(psmd->items[(int)param].item.type & WI_DISABLED))
            switch ((int)param) {
            case MI_LOAD:
                p_window_header_set_text(&(psmd->header), "LOAD FILAMENT");
                gui_dlg_load();
                p_window_header_set_text(&(psmd->header), "FILAMENT");
                break;
            case MI_UNLOAD:
                p_window_header_set_text(&(psmd->header), "UNLOAD FILAM.");
                gui_dlg_unload();
                p_window_header_set_text(&(psmd->header), "FILAMENT");
                break;
            case MI_CHANGE:
                p_window_header_set_text(&(psmd->header), "CHANGE FILAM.");
                gui_dlg_unload();
                gui_dlg_load();
                p_window_header_set_text(&(psmd->header), "FILAMENT");
                break;
            case MI_PURGE:
                p_window_header_set_text(&(psmd->header), "PURGE FILAM.");
                //   gui_dlg_purge();
                p_window_header_set_text(&(psmd->header), "FILAMENT");
                break;
            }
    return screen_menu_event(screen, window, event, param);
}

screen_t screen_menu_filament = {
    0,
    0,
    screen_menu_filament_init,
    screen_menu_done,
    screen_menu_draw,
    screen_menu_filament_event,
    sizeof(this_screen_data_t), //data_size
    0,                          //pdata
};

const screen_t *pscreen_menu_filament = &screen_menu_filament;
