// menu_service.cpp

#ifndef _EXTUI

    #include "sys.h"
    #include "cmsis_os.h"
    #include "../Marlin/src/lcd/menu/menu.h"

extern "C" {

int16_t sscg_freq_kHz = 5;
int16_t sscg_depth = 5;
int16_t spi_prescaler = 0;

extern int guimain_spi_test;
}

void menu_system_reset() {
    sys_reset();
}

void menu_pll_on() {
    sys_pll_enable();
    ui.refresh();
}

void menu_pll_off() {
    sys_pll_disable();
    ui.refresh();
}

void menu_sscg_on() {
    sys_sscg_set_config(sscg_freq_kHz * 1000, sscg_depth);
    sys_sscg_enable();
    ui.refresh();
}

void menu_sscg_off() {
    sys_sscg_disable();
    ui.refresh();
}

void menu_set_spi_prescaler() {
    sys_spi_set_prescaler(spi_prescaler);
    ui.refresh();
}

void _menu_endless_loop() {
    while (1) {
    #ifndef _DEBUG
        HAL_IWDG_Refresh(&hiwdg); //watchdog reset
    #endif                        //_DEBUG
        osDelay(1);
    }
}

void menu_spi_test1() {
    guimain_spi_test = 1;
    _menu_endless_loop();
}

void menu_spi_test2() {
    guimain_spi_test = 2;
    _menu_endless_loop();
}

void menu_go_dfu() {
    sys_dfu_boot();
}

void menu_service() {
    START_MENU();
    MENU_BACK(MSG_MAIN);
    if (sys_pll_is_enabled()) {
        MENU_ITEM(function, "System reset", menu_system_reset);
        MENU_ITEM(function, "PLL disable", menu_pll_off);
        if (sys_sscg_is_enabled()) {
            MENU_ITEM(function, "SSCG disable", menu_sscg_off);
            MENU_ITEM_EDIT(int3, "SSCG freq kHz", &sscg_freq_kHz, sscg_freq_kHz, sscg_freq_kHz);
            MENU_ITEM_EDIT(int3, "SSCG depth %", &sscg_depth, sscg_depth, sscg_depth);
        } else {
            MENU_ITEM(function, "SSCG enable", menu_sscg_on);
            MENU_ITEM_EDIT(int3, "SSCG freq kHz", &sscg_freq_kHz, 5, 200);
            MENU_ITEM_EDIT(int3, "SSCG depth %", &sscg_depth, 1, 20);
        }
    } else {
        MENU_ITEM(function, "PLL enable", menu_pll_on);
    }
    MENU_ITEM_EDIT_CALLBACK(int3, "SPI prescaler", &spi_prescaler, 0, 7, menu_set_spi_prescaler);
    MENU_ITEM(function, "SPI test1", menu_spi_test1);
    MENU_ITEM(function, "SPI test2", menu_spi_test2);
    //	MENU_ITEM(function, "DFU bootloader", menu_go_dfu);
    END_MENU();
}

#endif //_EXTUI
