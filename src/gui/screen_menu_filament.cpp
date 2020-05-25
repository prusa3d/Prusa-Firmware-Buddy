// screen_menu_filament.c

#include "gui.h"
#include "screen_menu.hpp"
#include "filament.h"
#include "filament_sensor.h"
#include "marlin_client.h"
#include "menu_vars.h"
#include "window_dlg_load_unload.h"
#include "screens.h"
#include "dbg.h"
#include "status_footer.h"

enum { F_EEPROM = 0x01,
    F_SENSED = 0x02 };

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

template <MI_t ENUM>
static void disable_item(screen_t *screen) {
    if (!(psmd->items[ENUM].item.type & WI_DISABLED)) {
        psmd->items[ENUM].item.type |= WI_DISABLED;
        psmd->menu.win.f_invalid = 1;
    }
}
template <MI_t ENUM>
static void enable_item(screen_t *screen) {
    if (psmd->items[ENUM].item.type & WI_DISABLED) {
        psmd->items[ENUM].item.type &= ~WI_DISABLED;
        psmd->menu.win.f_invalid = 1;
    }
}

/*
 * +---------+--------+------+--------+--------+-------+--------------------------------------------------------+
 * | FSENSOR | EEPROM | load | unload | change | purge | comment                                                |
 * +---------+--------+------+--------+--------+-------+--------------------------------------------------------+
 * |       0 |      0 |  YES |    YES |     NO |    NO | filament not loaded                                    |
 * |       0 |      1 |   NO |    YES |    YES |    NO | filament loaded but just runout                        |
 * |       1 |      0 |  YES |    YES |     NO |    NO | user pushed filament into sensor, but it is not loaded |
 * |       1 |      1 |   NO |    YES |    YES |   YES | filament loaded                                        |
 * +---------+--------+------+--------+--------+-------+--------------------------------------------------------+
 */
static void _deactivate_item(screen_t *screen) {

    uint8_t filament = 0;
    filament |= get_filament() != FILAMENT_NONE ? F_EEPROM : 0;
    filament |= fs_get_state() == FS_NO_FILAMENT ? 0 : F_SENSED;
    switch (filament) {
    case 0: //filament not loaded
        enable_item<MI_LOAD>(screen);
        disable_item<MI_CHANGE>(screen);
        disable_item<MI_PURGE>(screen);
        break;
    case F_EEPROM: //filament loaded but just runout
        disable_item<MI_LOAD>(screen);
        enable_item<MI_CHANGE>(screen);
        disable_item<MI_PURGE>(screen);
        break;
    case F_SENSED: //user pushed filament into sensor, but it is not loaded
        enable_item<MI_LOAD>(screen);
        disable_item<MI_CHANGE>(screen);
        disable_item<MI_PURGE>(screen);
        break;
    case F_SENSED | F_EEPROM: //filament loaded
    default:
        disable_item<MI_LOAD>(screen);
        enable_item<MI_CHANGE>(screen);
        enable_item<MI_PURGE>(screen);
        break;
    }
    gui_invalidate();
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

/// Sets temperature of nozzle not to ooze before print (MBL)
void setPreheatTemp() {
    //FIXME temperatures should be swapped
    marlin_gcode_printf("M104 S%d D%d", (int)PREHEAT_TEMP, (int)filaments[get_filament()].nozzle);
}

int screen_menu_filament_event(screen_t *screen, window_t *window, uint8_t event, void *param) {
    _deactivate_item(screen);

    if (event != WINDOW_EVENT_CLICK)
        return screen_menu_event(screen, window, event, param);

    if (psmd->items[(int)param].item.type & WI_DISABLED)
        return screen_menu_event(screen, window, event, param);

    switch ((int)param) {
    case MI_LOAD:
        p_window_header_set_text(&(psmd->header), "LOAD FILAMENT");
        gui_dlg_load();
        setPreheatTemp();
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
        setPreheatTemp();
        p_window_header_set_text(&(psmd->header), "FILAMENT");
        break;
    case MI_PURGE:
        p_window_header_set_text(&(psmd->header), "PURGE FILAM.");
        gui_dlg_purge();
        setPreheatTemp();
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

extern "C" screen_t *const get_scr_menu_filament() { return &screen_menu_filament; }
