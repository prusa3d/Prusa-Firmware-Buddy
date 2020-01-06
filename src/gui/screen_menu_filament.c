// screen_menu_filament.c

#include "gui.h"
#include "screen_menu.h"
#include "filament.h"
#include "filament_sensor.h"
#include "marlin_client.h"
#include "menu_vars.h"
#include "window_dlg_load.h"
#include "window_dlg_unload.h"
#include "window_dlg_purge.h"
#include "dbg.h"

extern screen_t screen_menu_preheat;
extern screen_t screen_preheating;

typedef enum {
    MI_RETURN,
    MI_LOAD,
    MI_UNLOAD,
    MI_CHANGE,
    MI_PURGE,
} MI_t;

const menu_item_t _menu_filament_items[] = {
    { { "Load Filament", 0, WI_LABEL }, SCREEN_MENU_NO_SCREEN },
    { { "Unload Filament", 0, WI_LABEL }, SCREEN_MENU_NO_SCREEN },
    { { "Change Filament", 0, WI_LABEL }, SCREEN_MENU_NO_SCREEN },
    { { "Purge Filament", 0, WI_LABEL }, SCREEN_MENU_NO_SCREEN },
};

static uint8_t filament_not_loaded = -1;//should be in data, but this sis simpler and it is only 1 BYTE

static void _deactivate_item(screen_t *screen) {
    uint8_t _filament_not_loaded = get_filament() == FILAMENT_NONE && fs_get_state() == FS_NO_FILAMENT ? 1 : 0;
    if(_filament_not_loaded != filament_not_loaded){
        filament_not_loaded = _filament_not_loaded;
        if(_filament_not_loaded){//disalbe CHANGE FILAMENT
            // window_enable(psmd->items[MI_LOAD].item->window.win.id);
            psmd->items[MI_LOAD].item.type &= ~WI_DISABLED;
            psmd->items[MI_CHANGE].item.type |= WI_DISABLED;
        }else{//disable LOAD FILAMENT
            psmd->items[MI_CHANGE].item.type &= ~WI_DISABLED;
            psmd->items[MI_LOAD].item.type |= WI_DISABLED;
        }
    }
}

void screen_menu_filament_init(screen_t *screen) {
    filament_not_loaded = -1;
    int count = sizeof(_menu_filament_items) / sizeof(menu_item_t);
    screen_menu_init(screen, "FILAMENT", count + 1, 1, 0);
    psmd->items[0] = menu_item_return;
    memcpy(psmd->items + 1, _menu_filament_items, count * sizeof(menu_item_t));
    _deactivate_item(screen);
}


int screen_menu_filament_event(screen_t *screen, window_t *window, uint8_t event, void *param) {
    _deactivate_item(screen);
    if (event == WINDOW_EVENT_CLICK)
        if(!(psmd->items[(int)param].item.type & WI_DISABLED))
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
            gui_dlg_purge();
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
    sizeof(screen_menu_data_t), //data_size
    0, //pdata
};

const screen_t *pscreen_menu_filament = &screen_menu_filament;
