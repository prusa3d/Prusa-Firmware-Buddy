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
#include "resource.h"

#define BOOTLOADER_VERSION_ADDRESS 0x801FFFA
#define OTP_START_ADDR             0x1FFF7800
#define SERIAL_NUM_ADDR            0x1FFF7808

enum {
    TAG_QUIT = 10
};

struct version_t {
    uint8_t major;
    uint8_t minor;
    uint8_t patch;
};

#define VERSION_INFO_STR_MAXLEN 150
char *version_info_str = nullptr;

#pragma pack(push)
#pragma pack(1)

//"C inheritance" of screen_menu_data_t with data items
typedef struct
{
    screen_menu_data_t base;
    menu_item_t items[1];

} this_screen_data_t;

#pragma pack(pop)

void screen_menu_version_info_init(screen_t *screen) {
    //=============SCREEN INIT===============
    screen_menu_init(screen, "VERSION INFO", ((this_screen_data_t *)screen->pdata)->items, 1, 0, 0);
    version_info_str = (char *)gui_malloc(VERSION_INFO_STR_MAXLEN * sizeof(char));

    p_window_header_set_icon(&(psmd->header), IDR_PNG_header_icon_info);

    psmd->items[0] = menu_item_return;

    uint16_t id = window_create_ptr(WINDOW_CLS_TEXT, psmd->root.win.id, rect_ui16(10, 80, 220, 200), &(psmd->help));
    psmd->help.font = resource_font(IDR_FNT_NORMAL);

    //=============VARIABLES=================

    uint8_t board_version[3];
    char serial_numbers[15];
    const version_t *bootloader = (const version_t *)BOOTLOADER_VERSION_ADDRESS;

    //=============ACCESS IN ADDR=================

    for (uint8_t i = 0; i < 14; i++) {
        if (i < 3) {
            board_version[i] = *(volatile uint8_t *)(OTP_START_ADDR + i);
        }
        serial_numbers[i] = *(volatile char *)(SERIAL_NUM_ADDR + i);
    }
    serial_numbers[14] = '\0';

    //=============SET TEXT================
    snprintf(version_info_str, VERSION_INFO_STR_MAXLEN, "Firmware version\n");

    // TODO: Oh, this is bad. Someone really has to fix text wrapping.
    const int max_chars_per_line = 18;
    int project_version_full_len = strlen(project_version_full);
    for (int i = 0; i < project_version_full_len; i += max_chars_per_line) {
        int line_length;
        if ((project_version_full_len - i) < max_chars_per_line)
            line_length = (project_version_full_len - i);
        else
            line_length = max_chars_per_line;
        snprintf(version_info_str + strlen(version_info_str),
            VERSION_INFO_STR_MAXLEN - strlen(version_info_str),
            "%.*s\n", line_length, project_version_full + i);
    }

    snprintf(version_info_str + strlen(version_info_str),
        VERSION_INFO_STR_MAXLEN - strlen(version_info_str),
        "\nBootloader version\n%d.%d.%d\n\nBuddy board\n%d.%d.%d\n%s",
        bootloader->major, bootloader->minor, bootloader->patch,
        board_version[0], board_version[1], board_version[2],
        serial_numbers);

    window_set_text(id, version_info_str);
}

void screen_menu_version_info_done(screen_t *screen) {

    if (version_info_str) {
        gui_free(version_info_str);
        version_info_str = nullptr;
    }
    screen_menu_done(screen);
}

screen_t screen_version_info = {
    0,
    0,
    screen_menu_version_info_init,
    screen_menu_version_info_done,
    screen_menu_draw,
    screen_menu_event,
    sizeof(this_screen_data_t), //data_size
    0,                          //pdata
};

const screen_t *pscreen_version_info = &screen_version_info;
