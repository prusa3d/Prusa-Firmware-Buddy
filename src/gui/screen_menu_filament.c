// screen_menu_filament.c

#include "gui.h"
#include "screen_menu.h"
#include "filament.h"
#include "marlin_client.h"
#include "menu_vars.h"
#include "window_dlg_load.h"
#include "window_dlg_unload.h"
#include "window_dlg_purge.h"
#include "dbg.h"

extern screen_t screen_menu_preheat;
extern screen_t screen_preheating;
extern uint8_t menu_preheat_type;

//#define OLD_LOAD_UNLOAD

typedef enum {
    MI_RETURN,
#ifdef OLD_LOAD_UNLOAD
    MI_LOAD,
    MI_UNLOAD,
    MI_M701,
    MI_M702,
#else
    MI_LOAD,
    MI_UNLOAD,
#endif
    MI_PURGE,
} MI_t;

const menu_item_t _menu_filament_items[] = {
#ifdef OLD_LOAD_UNLOAD
    { { "Load Filament", 0, WI_LABEL }, &screen_menu_preheat },
    { { "Unload Filament", 0, WI_LABEL }, &screen_preheating },
    { { "M701 Load", 0, WI_LABEL }, SCREEN_MENU_NO_SCREEN },
    { { "M702 Unload", 0, WI_LABEL }, SCREEN_MENU_NO_SCREEN },
#else
    { { "Load Filament", 0, WI_LABEL }, SCREEN_MENU_NO_SCREEN },
    { { "Unload Filament", 0, WI_LABEL }, SCREEN_MENU_NO_SCREEN },
#endif
    { { "Purge Filament", 0, WI_LABEL }, SCREEN_MENU_NO_SCREEN },
};

void screen_menu_filament_init(screen_t *screen) {
    int count = sizeof(_menu_filament_items) / sizeof(menu_item_t);
    screen_menu_init(screen, "FILAMENT", count + 1, 1, 0);
    psmd->items[0] = menu_item_return;
    memcpy(psmd->items + 1, _menu_filament_items, count * sizeof(menu_item_t));
}

int screen_menu_filament_event(screen_t *screen, window_t *window, uint8_t event, void *param) {
    if (event == WINDOW_EVENT_CLICK)
        switch ((int)param) {
#ifdef OLD_LOAD_UNLOAD
        case MI_LOAD:
        case MI_UNLOAD:
            menu_preheat_type = (int)param;
            if (menu_preheat_type == 2) { // we know what type of filament is in printer :)
                int16_t temp = filaments[get_filament()].nozzle;
                if (temp < extrude_min_temp)
                    temp = filaments[1].nozzle; // first filament is PLA
                marlin_gcode_printf("M104 S%d", (int)temp);
                // we really don't know, if user want to print after unload filament
            }
            break;
#endif
#ifndef OLD_LOAD_UNLOAD
        case MI_LOAD:
#else
        case MI_M701:
#endif
            p_window_header_set_text(&(psmd->header), "LOAD FILAMENT");
            gui_dlg_load();
            p_window_header_set_text(&(psmd->header), "FILAMENT");
            break;
#ifndef OLD_LOAD_UNLOAD
        case MI_UNLOAD:
#else
        case MI_M702:
#endif
            p_window_header_set_text(&(psmd->header), "UNLOAD FILAM.");
            gui_dlg_unload();
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
