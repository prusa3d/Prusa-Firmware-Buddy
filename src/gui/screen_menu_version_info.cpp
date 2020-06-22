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
#include "../lang/i18n.h"
#include "shared_config.h" //BOOTLOADER_VERSION_ADDRESS

#define OTP_START_ADDR  0x1FFF7800
#define SERIAL_NUM_ADDR 0x1FFF7808

#define VERSION_INFO_STR_MAXLEN 150

constexpr static const HelperConfig HelpCfg = { 10, IDR_FNT_NORMAL };
using parent = ScreenMenu<EHeader::On, EFooter::On, HelpCfg, MI_RETURN>;

class ScreenMenuVersionInfo : public parent {
public:
    std::array<char, VERSION_INFO_STR_MAXLEN> version_info_str;
    constexpr static const char *label = N_("VERSION INFO");
    static void Init(screen_t *screen);
};

/*****************************************************************************/
//static member method definition
void ScreenMenuVersionInfo::Init(screen_t *screen) {
    //=============SCREEN INIT===============
    Create(screen, label);
    ScreenMenuVersionInfo *const ths = reinterpret_cast<ScreenMenuVersionInfo *>(screen->pdata);

    p_window_header_set_icon(&(ths->header), IDR_PNG_header_icon_info);

    //=============VARIABLES=================

    uint8_t board_version[3];
    char serial_numbers[15];
    const version_t *bootloader = (const version_t *)BOOTLOADER_VERSION_ADDRESS;

    //=============ACCESS IN ADDR=================
    for (uint8_t i = 0; i < 3; i++) {
        board_version[i] = *(volatile uint8_t *)(OTP_START_ADDR + i);
    }
    for (uint8_t i = 0; i < 14; i++) {
        serial_numbers[i] = *(volatile char *)(SERIAL_NUM_ADDR + i);
    }
    serial_numbers[14] = '\0';

    //=============SET TEXT================
    auto begin = ths->version_info_str.begin();
    auto end = ths->version_info_str.end();
    begin += snprintf(begin, end - begin, _("Firmware Version\n"));

    // TODO: Oh, this is bad. Someone really has to fix text wrapping.
    const int max_chars_per_line = 18;
    int project_version_full_len = strlen(project_version_full);

    for (int i = 0; i < project_version_full_len; i += max_chars_per_line) {
        int line_length;
        if ((project_version_full_len - i) < max_chars_per_line)
            line_length = (project_version_full_len - i);
        else
            line_length = max_chars_per_line;
        if (end > begin)
            begin += snprintf(begin, end - begin, "%.*s\n", line_length, project_version_full + i);
    }

    if (end > begin)
        begin += snprintf(begin, end - begin,
            _("\nBootloader Version\n%d.%d.%d\n\nBuddy Board\n%d.%d.%d\n%s"),
            bootloader->major, bootloader->minor, bootloader->patch,
            board_version[0], board_version[1], board_version[2],
            serial_numbers);

    window_set_text(ths->help.win.id, ths->version_info_str.data());
}

screen_t screen_version_info = {
    0,
    0,
    ScreenMenuVersionInfo::Init,
    ScreenMenuVersionInfo::CDone,
    ScreenMenuVersionInfo::CDraw,
    ScreenMenuVersionInfo::CEvent,
    sizeof(ScreenMenuVersionInfo), //data_size
    nullptr,                       //pdata
};

screen_t *const get_scr_version_info() { return &screen_version_info; }
