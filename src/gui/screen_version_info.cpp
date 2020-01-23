/*
 * 	screen_version_info.cpp
 *
 *  Created on: 2019-10-14
 *      Author: Michal Rudolf
 */

#include "gui.h"
#include "config.h"
#include "screen_menu.h"
#include <stdlib.h>
#include "version.h"

#define BOOTLOADER_VERSION_ADDRESS 0x801FFFA
#define OTP_START_ADDR 0x1FFF7800
#define SERIAL_NUM_ADDR 0x1FFF7808

enum {
    TAG_QUIT = 10
};

struct version_t {
    uint8_t major;
    uint8_t minor;
    uint8_t patch;
};

char *version_info_str = nullptr;

void screen_menu_version_info_init(screen_t *screen) {
    //=============SCREEN INIT===============
    screen_menu_init(screen, "VERSION INFO", 1, 0, 0);

    p_window_header_set_icon(&(psmd->header), IDR_PNG_header_icon_info);

    psmd->items[0] = menu_item_return;

    psmd->phelp = (window_text_t *)gui_malloc(sizeof(window_text_t));
    uint16_t id = window_create_ptr(WINDOW_CLS_TEXT, psmd->root.win.id, rect_ui16(10, 80, 220, 200), &(psmd->phelp[0]));
    psmd->phelp[0].font = resource_font(IDR_FNT_NORMAL);

    //=============VARIABLES=================

    uint8_t board_version[3];
    char serial_numbers[15];
    uint8_t FW_version[3];
    uint16_t fw_parser = FW_VERSION;
    char FW_version_str[22] = { '\0' };
    version_info_str = (char *)gui_malloc(150 * sizeof(char));
    const version_t *bootloader = (const version_t *)BOOTLOADER_VERSION_ADDRESS;

    //=============ACCESS IN ADDR=================

    for (uint8_t i = 0; i < 14; i++) {
        if (i < 3) {
            board_version[i] = *(volatile uint8_t *)(OTP_START_ADDR + i);
        }
        serial_numbers[i] = *(volatile char *)(SERIAL_NUM_ADDR + i);
    }
    serial_numbers[14] = '\0';

    //=============FW VERSION PARSING=============

    FW_version[0] = (uint8_t)(fw_parser / 100);
    fw_parser -= FW_version[0] * 100;
    FW_version[1] = (uint8_t)(fw_parser / 10);
    fw_parser -= FW_version[1] * 10;
    FW_version[2] = (uint8_t)fw_parser;

#ifdef PRERELEASE_STR
    sprintf(FW_version_str, "%d.%d.%d-%s+%d",
        FW_version[0], FW_version[1], FW_version[2],
        PRERELEASE_STR, version_build_nr);
#else
    sprintf(FW_version_str, "%d.%d.%d", FW_version[0],
        FW_version[1], FW_version[2]);
#endif

    //=============SET TEXT================

    sprintf(version_info_str,
        "Firmware version\n%s\n\nBootloader version\n%d.%d.%d\n\nBuddy board\n%d.%d.%d\n%s",
        FW_version_str,
        bootloader->major, bootloader->minor, bootloader->patch,
        board_version[0], board_version[1], board_version[2],
        serial_numbers);

    window_set_text(id, version_info_str);
}

void screen_menu_version_info_done(screen_t *screen) {

    if (version_info_str)
        free(version_info_str);
    screen_menu_done(screen);
}

screen_t screen_version_info = {
    0,
    0,
    screen_menu_version_info_init,
    screen_menu_version_info_done,
    screen_menu_draw,
    screen_menu_event,
    sizeof(screen_menu_data_t), //data_size
    0, //pdata
};

const screen_t *pscreen_version_info = &screen_version_info;
