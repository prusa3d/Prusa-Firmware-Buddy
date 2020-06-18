/*
 * screen_test_temperature.c
 *
 *  Created on: 2019-09-25
 *      Author: Vana Radek
 */

#include <menu_vars.h>
#include "gui.h"
#include "screen_menu.hpp"
#include "cmsis_os.h"
#include "hwio.h"
#include "screens.h"
#include "../Marlin/src/module/temperature.h"
/*
static const char *period_pwm_range[] = {
    "1.5us", "  3us", "  6us", " 12us", " 24us", " 48us", " 97us", "195us", "390us",
    "780us", "1.5ms", "3.1ms", "6.2ms", " 12ms", " 25ms", " 50ms", "100ms", NULL
};
static const size_t pwm_size = sizeof(period_pwm_range) / sizeof(const char *);

typedef enum {
    MI_RETURN,
    MI_NOZZLE,
    MI_HEAT_PWM_PERIOD,
    MI_HEATBED,
    MI_FAN_PWM_PERIOD,
    MI_PRINTFAN,
    MI_COOLDOWN,
    MI_COUNT
} MI_t;

//"C inheritance" of ScreenMenu with data items

struct this_screen_data_t {
    ScreenMenu base;
    menu_item_t items[MI_COUNT];

};

void screen_test_temperature_init(screen_t *screen) {
    screen_menu_init(screen, "TEMPERATURE", ((this_screen_data_t *)screen->pdata)->items, MI_COUNT, 1, 0);

    psmd->items[MI_RETURN] = menu_item_return;

    psmd->items[MI_NOZZLE] = (menu_item_t) { { "Nozzle", 0, WI_SPIN }, SCREEN_MENU_NO_SCREEN };
    psmd->items[MI_NOZZLE].item.data.wi_spin.value = thermalManager.degTargetHotend(0) * 1000;
    psmd->items[MI_NOZZLE].item.data.wi_spin.range = nozzle_range;

    psmd->items[MI_HEAT_PWM_PERIOD] = (menu_item_t) { { "Ht. PWM T", 0, WI_SELECT }, SCREEN_MENU_NO_SCREEN };
    psmd->items[MI_HEAT_PWM_PERIOD].item.data.wi_switch.index = hwio_pwm_get_prescaler_log2(HWIO_PWM_HEATER_BED);
    psmd->items[MI_HEAT_PWM_PERIOD].item.data.wi_switch.strings = period_pwm_range;

    psmd->items[MI_HEATBED] = (menu_item_t) { { "Heatbed", 0, WI_SPIN }, SCREEN_MENU_NO_SCREEN };
    psmd->items[MI_HEATBED].item.data.wi_spin.value = thermalManager.degTargetBed() * 1000;
    psmd->items[MI_HEATBED].item.data.wi_spin.range = heatbed_range;

    psmd->items[MI_FAN_PWM_PERIOD] = (menu_item_t) { { "Fan PWM T", 0, WI_SELECT }, SCREEN_MENU_NO_SCREEN };
    psmd->items[MI_FAN_PWM_PERIOD].item.data.wi_select.index = hwio_pwm_get_prescaler_log2(HWIO_PWM_HEATER_BED);
    psmd->items[MI_FAN_PWM_PERIOD].item.data.wi_select.strings = period_pwm_range;

    psmd->items[MI_PRINTFAN] = (menu_item_t) { { "Print Fan", 0, WI_SPIN }, SCREEN_MENU_NO_SCREEN };
    psmd->items[MI_PRINTFAN].item.data.wi_spin.value = thermalManager.fan_speed[0] * 1000;
    psmd->items[MI_PRINTFAN].item.data.wi_spin.range = printfan_range;

    psmd->items[MI_COOLDOWN] = (menu_item_t) { { "Cooldown", 0, WI_LABEL }, SCREEN_MENU_NO_SCREEN };
}

int screen_test_temperature_event(screen_t *screen, window_t *window,
    uint8_t event, void *param) {
    if (screen_menu_event(screen, window, event, param)) {
        return 1;
    }

    if (event == WINDOW_EVENT_CHANGING) {
        switch ((int)param) {
        case MI_NOZZLE:
            thermalManager.setTargetHotend(psmd->items[MI_NOZZLE].item.data.wi_spin.value / 1000, 0);
            break;
        case MI_HEATBED:
            thermalManager.setTargetBed(psmd->items[MI_HEATBED].item.data.wi_spin.value / 1000);
            break;
        case MI_PRINTFAN:
            thermalManager.set_fan_speed(0, psmd->items[MI_PRINTFAN].item.data.wi_spin.value / 1000);
            break;
        case MI_HEAT_PWM_PERIOD:
            hwio_pwm_set_prescaler_exp2(HWIO_PWM_HEATER_BED, psmd->items[MI_HEAT_PWM_PERIOD].item.data.wi_switch.index);
            break;
        case MI_FAN_PWM_PERIOD:
            hwio_pwm_set_prescaler_exp2(HWIO_PWM_FAN, psmd->items[MI_FAN_PWM_PERIOD].item.data.wi_select.index);
            break;
        }
    } else if (event == WINDOW_EVENT_CLICK && (int)param == MI_COOLDOWN) {
        thermalManager.setTargetHotend(0, 0);
        thermalManager.setTargetBed(0);
        thermalManager.set_fan_speed(0, 0);
        psmd->items[MI_NOZZLE].item.data.wi_spin.value = 0;
        psmd->items[MI_HEATBED].item.data.wi_spin.value = 0;
        psmd->items[MI_PRINTFAN].item.data.wi_spin.value = 0;
        _window_invalidate(&(psmd->root.win));
    }

    return 0;
}

screen_t screen_test_temperature = {
    0,
    0,
    screen_test_temperature_init,
    screen_menu_done,
    screen_menu_draw,
    screen_test_temperature_event,
    sizeof(this_screen_data_t), //data_size
    0,                          //pdata
};
*/

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"

using Screen = ScreenMenu<EHeader::Off, EFooter::On, HelpLines_None, MI_RETURN>;

static void init(screen_t *screen) {
    Screen::Create(screen);
}

screen_t screen_test_temperature = {
    0,
    0,
    init,
    Screen::CDone,
    Screen::CDraw,
    Screen::CEvent,
    sizeof(Screen), //data_size
    0,              //pdata
};

screen_t *const get_scr_test_temperature() { return &screen_test_temperature; }
