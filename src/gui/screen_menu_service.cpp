// screen_menu_service.c

#include "config.h"
#include "gui.h"
#include "screen_menu.hpp"
#include "stm32f4xx_hal.h"
#include "sys.h"
#include "dbg.h"
#include "bsod.h"
#include "eeprom.h"
#include "screens.h"
/*
typedef enum {
    MI_RETURN,
    MI_SYS_RESET,
    MI_CLR_EEPROM,
    MI_WDG_TEST,
    MI_PLL,
    MI_SSCG,
    MI_SSCG_FREQ,
    MI_SSCG_DEPTH,
    MI_SPI_PRESC,
#ifdef PIDCALIBRATION
    MI_PID,
#endif //PIDCALIBRATION
    MI_MESH,
    MI_BSOD,
    MI_BSOD_HARD_FAULT,
    MI_COUNT
} MI_t;

//"C inheritance" of ScreenMenu with data items

struct this_screen_data_t {
    ScreenMenu base;
    menu_item_t items[MI_COUNT];
};

int16_t sscg_freq_kHz = 5;
int16_t sscg_depth = 5;
int16_t spi_prescaler = 0;

const char *opt_enable_disable[] = { "Enable", "Disable", NULL };
const int32_t opt_sscg_freq[] = { 5000, 150000, 1000 };
const int32_t opt_sscg_depth[] = { 1000, 20000, 1000 };
const char *opt_spi[] = { "21M", "10.5M", "5.25M", "2.63M", "1.31M", "656k", "328k", "164k", NULL };

void screen_menu_service_init(screen_t *screen) {
    screen_menu_init(screen, "SERVICE", ((this_screen_data_t *)screen->pdata)->items, MI_COUNT, 0, 0);

    psmd->items[MI_RETURN] = menu_item_return;
    psmd->items[MI_SYS_RESET] = (menu_item_t) { { "System reset", 0, WI_LABEL }, SCREEN_MENU_NO_SCREEN };
    psmd->items[MI_CLR_EEPROM] = (menu_item_t) { { "Clear EEPROM", 0, WI_LABEL }, SCREEN_MENU_NO_SCREEN };
    psmd->items[MI_WDG_TEST] = (menu_item_t) { { "Watchdog test", 0, WI_LABEL }, SCREEN_MENU_NO_SCREEN };
    psmd->items[MI_PLL] = (menu_item_t) { { "PLL", 0, WI_SWITCH }, SCREEN_MENU_NO_SCREEN };
    psmd->items[MI_PLL].item.data.wi_switch.index = sys_pll_is_enabled() ? 1 : 0;
    psmd->items[MI_PLL].item.data.wi_switch.strings = opt_enable_disable;

    psmd->items[MI_SSCG] = (menu_item_t) { { "SSCG", 0, WI_SWITCH }, SCREEN_MENU_NO_SCREEN };
    psmd->items[MI_SSCG].item.data.wi_switch.index = sys_sscg_is_enabled() ? 1 : 0;
    psmd->items[MI_SSCG].item.data.wi_switch.strings = opt_enable_disable;

    psmd->items[MI_SSCG_FREQ] = (menu_item_t) { { "SSCG freq", 0, WI_SPIN }, SCREEN_MENU_NO_SCREEN };
    psmd->items[MI_SSCG_FREQ].item.data.wi_spin.value = sscg_freq_kHz * 1000;
    psmd->items[MI_SSCG_FREQ].item.data.wi_spin.range = opt_sscg_freq;

    psmd->items[MI_SSCG_DEPTH] = (menu_item_t) { { "SSCG depth", 0, WI_SPIN }, SCREEN_MENU_NO_SCREEN };
    psmd->items[MI_SSCG_DEPTH].item.data.wi_spin.value = sscg_depth * 1000;
    psmd->items[MI_SSCG_DEPTH].item.data.wi_spin.range = opt_sscg_depth;

    psmd->items[MI_SPI_PRESC] = (menu_item_t) { { "SPI freq", 0, WI_SELECT }, SCREEN_MENU_NO_SCREEN };
    psmd->items[MI_SPI_PRESC].item.data.wi_select.index = spi_prescaler;
    psmd->items[MI_SPI_PRESC].item.data.wi_select.strings = opt_spi;
#ifdef PIDCALIBRATION
    psmd->items[MI_PID] = (menu_item_t) { { "PID calibration", 0, WI_LABEL }, get_scr_PID() };
#endif //PIDCALIBRATION
    psmd->items[MI_MESH] = (menu_item_t) { { "Mesh bed leveling", 0, WI_LABEL }, get_scr_mesh_bed_lv() };
    psmd->items[MI_BSOD] = (menu_item_t) { { "BSOD", 0, WI_LABEL }, SCREEN_MENU_NO_SCREEN };
    psmd->items[MI_BSOD_HARD_FAULT] = (menu_item_t) { { "BSOD_HARD_FAULT", 0, WI_LABEL }, SCREEN_MENU_NO_SCREEN };
}

int screen_menu_service_event(screen_t *screen, window_t *window, uint8_t event, void *param) {

    if (screen_menu_event(screen, window, event, param)) {
        return 1;
    }
    if (event == WINDOW_EVENT_CLICK) {
        switch ((int)param) {
        case MI_SYS_RESET:
            sys_reset();
            break;
        case MI_CLR_EEPROM:
            eeprom_clear();
            break;
        case MI_WDG_TEST:
            __disable_irq();
            while (1)
                ;
            break;
        case MI_PLL:
            if (psmd->items[MI_PLL].item.data.wi_switch.index)
                sys_pll_enable();
            else
                sys_pll_disable();
            break;
        case MI_SSCG:
            if (psmd->items[MI_SSCG].item.data.wi_switch.index) {
                sys_sscg_set_config(sscg_freq_kHz * 1000, sscg_depth);
                sys_sscg_enable();
            } else
                sys_sscg_disable();
            break;
        case MI_BSOD:
            bsod("System error! Please restart the printer.");
            break;
        case MI_BSOD_HARD_FAULT: {
            //this will cause hard fault
            //0x08000000 - flash origin
            volatile uint32_t *crash = (uint32_t *)0x20020000 + 4; //0x08000000;
            *crash = 1;
            for (int i = 0; i < 1000; ++i) {
                psmd->items[i].item.data.wi_spin.value = i;
            }
            break;
        }
        }
    } else if (event == WINDOW_EVENT_CHANGE) {
        switch ((int)param) {
        case MI_SSCG_FREQ:
        case MI_SSCG_DEPTH:
            sscg_freq_kHz = psmd->items[MI_SSCG_FREQ].item.data.wi_spin.value / 1000;
            sscg_depth = psmd->items[MI_SSCG_DEPTH].item.data.wi_spin.value / 1000;
            if (sys_sscg_is_enabled()) {
                sys_sscg_disable();
                sys_sscg_set_config(sscg_freq_kHz * 1000, sscg_depth);
                sys_sscg_enable();
            }
            break;
        case MI_SPI_PRESC:
            sys_spi_set_prescaler(psmd->items[MI_SPI_PRESC].item.data.wi_select.index);
            break;
        }
    }
    return 0;
}

screen_t screen_menu_service = {
    0,
    0,
    screen_menu_service_init,
    screen_menu_done,
    screen_menu_draw,
    screen_menu_service_event,
    sizeof(this_screen_data_t), //data_size
    0,                          //pdata
};
*/

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"

using Screen = ScreenMenu<EHeader::Off, EFooter::On, HelpLines_None, MI_RETURN, MI_TEST_DISABLED_RETURN>;

static void init(screen_t *screen) {
    Screen::Create(screen);
}

screen_t screen_menu_service = {
    0,
    0,
    init,
    Screen::CDone,
    Screen::CDraw,
    Screen::CEvent,
    sizeof(Screen), //data_size
    0,              //pdata
};

screen_t *const get_scr_menu_service() { return &screen_menu_service; }
