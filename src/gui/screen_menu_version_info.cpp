/*
 * 	screen_version_info.cpp
 *
 *  Created on: 2019-10-14
 *      Author: Michal Rudolf
 */
//todo THIS SHOULD NOT BE MENU!!!
#include "gui.h"
#include "config.h"
#include "screen_menu.hpp"
#include <stdlib.h>
#include "version.h"
#include "resource.h"
#include "screens.h"
#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"

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

using parent = screen_menu_data_t<true, true, true, MI_RETURN>;

#pragma pack(push, 1)
class ScreenMenuVersionInfo : public parent {
public:
    constexpr static const char *label = "VERSION INFO";
    static void Init(screen_t *screen);
    static int CEvent(screen_t *screen, window_t *window, uint8_t event, void *param);
    static void CDone(screen_t *screen);
};
#pragma pack(pop)

/*****************************************************************************/
//static member method definition
void ScreenMenuVersionInfo::Init(screen_t *screen) {
    //=============SCREEN INIT===============
    Create(screen, label);
    ScreenMenuVersionInfo *const ths = reinterpret_cast<ScreenMenuVersionInfo *>(screen->pdata);
    version_info_str = (char *)gui_malloc(VERSION_INFO_STR_MAXLEN * sizeof(char));

    p_window_header_set_icon(&(ths->header), IDR_PNG_header_icon_info);

    //uint16_t id = window_create_ptr(WINDOW_CLS_TEXT, ths->root.win.id, rect_ui16(10, 80, 220, 200), &(ths->help));//do I need this line?
    ths->help.font = resource_font(IDR_FNT_NORMAL);

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
    snprintf(version_info_str, VERSION_INFO_STR_MAXLEN, "Firmware Version\n");

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
        "\nBootloader Version\n%d.%d.%d\n\nBuddy Board\n%d.%d.%d\n%s",
        bootloader->major, bootloader->minor, bootloader->patch,
        board_version[0], board_version[1], board_version[2],
        serial_numbers);

    window_set_text(ths->help.win.id, version_info_str);
}

int ScreenMenuVersionInfo::CEvent(screen_t *screen, window_t *window, uint8_t event, void *param) {
    ScreenMenuVersionInfo *const ths = reinterpret_cast<ScreenMenuVersionInfo *>(screen->pdata);
    if (event == WINDOW_EVENT_LOOP) {
        //todo handle if FS disconnects
    }

    return ths->Event(window, event, param);
}

void ScreenMenuVersionInfo::CDone(screen_t *screen) {
    ScreenMenuVersionInfo *const ths = reinterpret_cast<ScreenMenuVersionInfo *>(screen->pdata);
    if (version_info_str) {
        gui_free(version_info_str);
        version_info_str = nullptr;
    }
    ths->Done();
}

screen_t screen_version_info = {
    0,
    0,
    ScreenMenuVersionInfo::Init,
    ScreenMenuVersionInfo::CDone,
    ScreenMenuVersionInfo::CDraw,
    ScreenMenuVersionInfo::CEvent,
    sizeof(ScreenMenuVersionInfo), //data_size
    0,                             //pdata
};

extern "C" screen_t *const get_scr_version_info() { return &screen_version_info; }
