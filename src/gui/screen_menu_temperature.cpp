// screen_menu_temperature.c

#include "gui.h"
#include "screen_menu.hpp"
#include "marlin_client.h"
#include "screens.h"

#include "menu_vars.h"

typedef enum {
    MI_RETURN,
    MI_NOZZLE,
    MI_HEATBED,
    MI_PRINTFAN,
    MI_COOLDOWN,
    MI_COUNT
} MI_t;

//"C inheritance" of screen_menu_data_t with data items
#pragma pack(push)
#pragma pack(1)

typedef struct
{
    screen_menu_data_t base;
    menu_item_t items[MI_COUNT];

} this_screen_data_t;

#pragma pack(pop)

void screen_menu_temperature_init(screen_t *screen) {
    marlin_vars_t *vars;
    screen_menu_init(screen, "TEMPERATURE", ((this_screen_data_t *)screen->pdata)->items, MI_COUNT, 1, 0);
    psmd->items[MI_RETURN] = menu_item_return;
    //memcpy(psmd->items + 1, _menu_temperature_items, (MI_COUNT - 1) * sizeof(menu_item_t));

    vars = marlin_update_vars(
        MARLIN_VAR_MSK(MARLIN_VAR_TTEM_NOZ) | MARLIN_VAR_MSK(MARLIN_VAR_TTEM_BED) | MARLIN_VAR_MSK(MARLIN_VAR_FANSPEED));
    psmd->items[MI_NOZZLE] = (menu_item_t) { { "Nozzle", 0, WI_SPIN, 0 }, SCREEN_MENU_NO_SCREEN };
    psmd->items[MI_NOZZLE].item.wi_spin.value = (int32_t)(vars->target_nozzle * 1000);
    psmd->items[MI_NOZZLE].item.wi_spin.range = nozzle_range;

    psmd->items[MI_HEATBED] = (menu_item_t) { { "Heatbed", 0, WI_SPIN, 0 }, SCREEN_MENU_NO_SCREEN };
    psmd->items[MI_HEATBED].item.wi_spin.value = (int32_t)(vars->target_bed * 1000);
    psmd->items[MI_HEATBED].item.wi_spin.range = heatbed_range;

    psmd->items[MI_PRINTFAN] = (menu_item_t) { { "Print Fan", 0, WI_SPIN, 0 }, SCREEN_MENU_NO_SCREEN };
    psmd->items[MI_PRINTFAN].item.wi_spin.value = (int32_t)(vars->fan_speed * 1000);
    psmd->items[MI_PRINTFAN].item.wi_spin.range = printfan_range;

    psmd->items[MI_COOLDOWN] = (menu_item_t) { { "Cooldown", 0, WI_LABEL }, SCREEN_MENU_NO_SCREEN };
}

int screen_menu_temperature_event(screen_t *screen, window_t *window, uint8_t event, void *param) {
    if (screen_menu_event(screen, window, event, param))
        return 1;

    if (event == WINDOW_EVENT_CHANGE) {
        switch ((int)param) {
        case MI_NOZZLE: {
            const float temp = psmd->items[MI_NOZZLE].item.wi_spin.value / 1000.0f;
            marlin_set_target_nozzle(temp);
            marlin_set_display_nozzle(temp);
            break;
        }
        case MI_HEATBED:
            marlin_set_target_bed(psmd->items[MI_HEATBED].item.wi_spin.value / 1000);
            break;
        case MI_PRINTFAN:
            marlin_set_fan_speed(psmd->items[MI_PRINTFAN].item.wi_spin.value / 1000);
            break;
        }
    } else if ((event == WINDOW_EVENT_CLICK) && (int)param == MI_COOLDOWN) {
        marlin_set_target_nozzle(0);
        marlin_set_display_nozzle(0);
        marlin_set_target_bed(0);
        marlin_set_fan_speed(0);
        psmd->items[MI_NOZZLE].item.wi_spin.value = 0;
        psmd->items[MI_HEATBED].item.wi_spin.value = 0;
        psmd->items[MI_PRINTFAN].item.wi_spin.value = 0;
        _window_invalidate(&(psmd->root.win));
    }
    return 0;
}

screen_t screen_menu_temperature = {
    0,
    0,
    screen_menu_temperature_init,
    screen_menu_done,
    screen_menu_draw,
    screen_menu_temperature_event,
    sizeof(this_screen_data_t), //data_size
    0,                          //pdata
};

extern "C" screen_t *const get_scr_menu_temperature() { return &screen_menu_temperature; }
