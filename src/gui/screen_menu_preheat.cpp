/*
 * screen_menu_preheat.cpp
 *
 *  Created on: 19. 7. 2019
 *      Author: mcbig
 */

#include <stdio.h>
#include "gui.h"
#include "screen_menu.hpp"
#include "filament.h"
#include "marlin_client.h"
#include "screens.h"
/*
//"C inheritance" of screen_menu_data_t with data items
#pragma pack(push)
#pragma pack(1)

typedef struct
{
    screen_menu_data_t base;
    menu_item_t items[FILAMENTS_END + 1];

} this_screen_data_t;

#pragma pack(pop)

void screen_menu_preheat_init(screen_t *screen) {
    screen_menu_init(screen, "PREHEAT", ((this_screen_data_t *)screen->pdata)->items, FILAMENTS_END + 1, 1, 0);
    psmd->items[0] = menu_item_return;

    for (size_t i = 1; i < FILAMENTS_END; i++) {
        psmd->items[i].item.label.fill(' ');
        strncpy(psmd->items[i].item.label.data(), filaments[i].name,
            strlen(filaments[i].name));
        sprintf(psmd->items[i].item.label.data() + 9, "%d/%d",
            filaments[i].nozzle, filaments[i].heatbed);
    }
    psmd->items[FILAMENTS_END] = (menu_item_t) { { "Cooldown", 0, WI_LABEL }, SCREEN_MENU_NO_SCREEN };
    window_set_item_index(psmd->menu.win.id, get_filament());
}

void screen_menu_preheat_done(screen_t *screen) {
    screen_menu_done(screen);
}

int screen_menu_preheat_event(screen_t *screen, window_t *window,
    uint8_t event, void *param) {
    if (screen_menu_event(screen, window, event, param)) {
        return 1; // Screen return return here ...
    }
    if (event != WINDOW_EVENT_CLICK) {
        return 0;
    }

    filament_t filament;
    FILAMENT_t fil_id;

    if ((uint32_t)param < (uint32_t)FILAMENTS_END) {
        fil_id = (FILAMENT_t)(uint32_t)param;
    } else {
        fil_id = FILAMENT_NONE;
    }

    filament = filaments[fil_id];

    marlin_gcode("M86 S1800"); // enable safety timer
    marlin_gcode_printf("M104 S%d", (int)filament.nozzle);
    marlin_gcode_printf("M140 S%d", (int)filament.heatbed);

    screen_close(); // skip this screen averytime
    return 1;
}

screen_t screen_menu_preheat = {
    0,
    0,
    screen_menu_preheat_init,
    screen_menu_preheat_done,
    screen_menu_draw,
    screen_menu_preheat_event,
    sizeof(this_screen_data_t), //data_size
    0,                          //pdata
};*/
#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"

using Screen = screen_menu_data_t<false, true, false, MI_RETURN>;

static void init(screen_t *screen) {
    Screen::Create(screen);
}

screen_t screen_menu_preheat = {
    0,
    0,
    init,
    Screen::CDone,
    Screen::CDraw,
    Screen::CEvent,
    sizeof(Screen), //data_size
    0,              //pdata
};

extern "C" screen_t *const get_scr_menu_preheat() { return &screen_menu_preheat; }
