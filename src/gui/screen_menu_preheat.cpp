/*
 * screen_menu_preheat.cpp
 *
 *  Created on: 19. 7. 2019
 *      Author: mcbig
 */

#include <stdio.h>
#include "gui.h"
#include "screen_menu.h"
#include "filament.h"
#include "marlin_client.h"

uint8_t menu_preheat_type = 0; // 0 - preheat, 1 - load filament, 2 - unload filament

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
    switch (menu_preheat_type) {
    case 0:
        screen_menu_init(screen, "PREHEAT", ((this_screen_data_t *)screen->pdata)->items, FILAMENTS_END + 1, 1, 0);
        break;
    case 1:
        screen_menu_init(screen, "LOAD FILAMENT", ((this_screen_data_t *)screen->pdata)->items, FILAMENTS_END, 1, 1);
        window_set_text(psmd->help.win.id,
            "The nozzle must be\npreheated before\ninserting the filament.\n"
            "Please, select the type\nof material");
        break;
    }

    psmd->items[0] = menu_item_return;

    for (size_t i = 1; i < FILAMENTS_END; i++) {
        memset((char *)psmd->items[i].item.label, ' ', sizeof(char) * 15);
        strncpy((char *)psmd->items[i].item.label, filaments[i].name,
            strlen(filaments[i].name));
        sprintf((char *)psmd->items[i].item.label + 9, "%d/%d",
            filaments[i].nozzle, filaments[i].heatbed);
    }

    if (!menu_preheat_type) {
        psmd->items[FILAMENTS_END] = (menu_item_t) { { "Cooldown", 0, WI_LABEL }, SCREEN_MENU_NO_SCREEN };
    }

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

    if (menu_preheat_type == 1) {
        set_filament(fil_id); // store the filamen to eeprom
    }
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
};

const screen_t *pscreen_menu_preheat = &screen_menu_preheat;
