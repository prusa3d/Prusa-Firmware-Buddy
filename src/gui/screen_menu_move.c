// screen_menu_move.c

#include "gui.h"
#include "screen_menu.h"
#include "marlin_client.h"

#include "menu_vars.h"

typedef enum {
    MI_RETURN,
    MI_MOVE_X,
    MI_MOVE_Y,
    MI_MOVE_Z,
    MI_MOVE_E,
    MI_COUNT
} MI_t;

const menu_item_t _menu_move_items[] = {
    { { "Move X", 0, WI_SPIN, .wi_spin = { 0, move_x } }, SCREEN_MENU_NO_SCREEN },
    { { "Move Y", 0, WI_SPIN, .wi_spin = { 0, move_y } }, SCREEN_MENU_NO_SCREEN },
    { { "Move Z", 0, WI_SPIN, .wi_spin = { 0, move_z } }, SCREEN_MENU_NO_SCREEN },
    { { "Extruder", 0, WI_SPIN, .wi_spin = { 0, move_e } }, SCREEN_MENU_NO_SCREEN },
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

void screen_menu_move_init(screen_t *screen) {
    marlin_vars_t *vars;
    screen_menu_init(screen, "MOVE AXIS", ((this_screen_data_t *)screen->pdata)->items, MI_COUNT, 1, 0);
    psmd->items[0] = menu_item_return;
    memcpy(psmd->items + 1, _menu_move_items, (MI_COUNT - 1) * sizeof(menu_item_t));

    vars = marlin_update_vars(MARLIN_VAR_MSK_POS_XYZE | MARLIN_VAR_MSK(MARLIN_VAR_TEMP_NOZ));
    psmd->items[MI_MOVE_X].item.wi_spin.value = (int32_t)(vars->pos[0] * 1000);
    psmd->items[MI_MOVE_Y].item.wi_spin.value = (int32_t)(vars->pos[1] * 1000);
    psmd->items[MI_MOVE_Z].item.wi_spin.value = (int32_t)(vars->pos[2] * 1000);
    if (vars->temp_nozzle < extrude_min_temp)
        psmd->items[MI_MOVE_E].item.type |= WI_DISABLED;
    gui_timer_create_periodical(500, 0);
}

int screen_menu_move_event(screen_t *screen, window_t *window, uint8_t event, void *param) {
    marlin_vars_t *vars;
    if (screen_menu_event(screen, window, event, param))
        return 1;
    if (event == WINDOW_EVENT_CHANGING) {
        char axis[4] = { 'X', 'Y', 'Z', 'E' };
        marlin_gcode_printf("G0 %c%.4f F%d", axis[(int)param - 1], psmd->items[(int)param].item.wi_spin.value / 1000.0, manual_feedrate[(int)param - 1]);
    } else if ((event == WINDOW_EVENT_CHANGE) && ((int)param == MI_MOVE_E)) {
        // marlin_gcode("G92 E0"); // Reset position after change - not necessary
    } else if (event == WINDOW_EVENT_CLICK) {
        marlin_gcode("G90"); // Set to Absolute Positioning
        if ((int)param == MI_MOVE_E) {
            marlin_gcode("M82");    // Set extruder to absolute mode
            marlin_gcode("G92 E0"); // Reset position before change
        }
        psmd->items[MI_MOVE_E].item.wi_spin.value = 0; // Reset spin before change
        psmd->menu.win.flg &= ~WINDOW_FLG_INVALID;
    } else if (event == WINDOW_EVENT_TIMER) {
        vars = marlin_vars();
        if (vars->target_nozzle > extrude_min_temp) {
            if (psmd->items[MI_MOVE_E].item.type & WI_DISABLED) {
                psmd->items[MI_MOVE_E].item.type ^= WI_DISABLED;
                psmd->menu.win.flg &= ~WINDOW_FLG_INVALID;
            }
        } else {
            if (!(psmd->items[MI_MOVE_E].item.type & WI_DISABLED)) {
                psmd->items[MI_MOVE_E].item.type ^= WI_DISABLED;
                psmd->menu.win.flg &= ~WINDOW_FLG_INVALID;
            }
        }
    }
    return 0;
}

screen_t screen_menu_move = {
    0,
    0,
    screen_menu_move_init,
    screen_menu_done,
    0,
    screen_menu_move_event,
    sizeof(this_screen_data_t), //data_size
    0,                          //pdata
};

const screen_t *pscreen_menu_move = &screen_menu_move;
