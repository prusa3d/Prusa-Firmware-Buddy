/*
 * 	screen_version_info.cpp
 *
 *  Created on: 2019-10-14
 *      Author: Michal Rudolf
 */
//todo THIS SHOULD NOT BE MENU!!!
#include <stdlib.h>

#include "gui.hpp"
#include "screen_menus.hpp"
#include "screen_menu.hpp"
#include "config.h"
#include "version.h"
#include "resource.h"
#include "WindowMenuItems.hpp"
#include "i18n.h"
#include "shared_config.h" //BOOTLOADER_VERSION_ADDRESS
#include "../common/otp.h"

static const constexpr uint8_t VERSION_INFO_STR_MAXLEN = 150;

using MenuContainer = WinMenuContainer<MI_RETURN>;

class ScreenMenuVersionInfo : public AddSuperWindow<screen_t> {
    std::array<char, VERSION_INFO_STR_MAXLEN> version_info_str;
    constexpr static const char *label = N_("VERSION INFO");
    static constexpr size_t helper_lines = 10;
    static constexpr int helper_font = IDR_FNT_NORMAL;

    MenuContainer container;
    window_menu_t menu;
    window_header_t header;
    window_text_t help;
    status_footer_t footer;

public:
    ScreenMenuVersionInfo();

protected:
    static inline uint16_t get_help_h() {
        return helper_lines * (resource_font(helper_font)->h);
    }
};

ScreenMenuVersionInfo::ScreenMenuVersionInfo()
    : AddSuperWindow<screen_t>(nullptr)
    , menu(this, GuiDefaults::RectScreenBody - Rect16::Height_t(get_help_h()), &container)
    , header(this)
    , help(this, Rect16(GuiDefaults::RectScreen.Left(), uint16_t(GuiDefaults::RectFooter.Top()) - get_help_h(), GuiDefaults::RectScreen.Width(), get_help_h()), is_multiline::yes)
    , footer(this) {
    header.SetText(_(label));
    help.font = resource_font(helper_font);
    menu.GetActiveItem()->SetFocus(); // set focus on new item//containder was not valid during construction, have to set its index again
    CaptureNormalWindow(menu);        // set capture to list

    //=============SCREEN INIT===============
    header.SetIcon(IDR_PNG_info_16px);

    //=============VARIABLES=================

    uint8_t board_version[3];
    char serial_numbers[15];
    const version_t *bootloader = (const version_t *)BOOTLOADER_VERSION_ADDRESS;

    //=============ACCESS IN ADDR=================
    for (uint8_t i = 0; i < 3; i++) {
        board_version[i] = *(volatile uint8_t *)(OTP_BOARD_REVISION_ADDR + i);
    }
    for (uint8_t i = 0; i < 14; i++) {
        serial_numbers[i] = *(volatile char *)(OTP_SERIAL_NUMBER_ADDR + i);
    }
    serial_numbers[14] = '\0';

    //=============SET TEXT================
    auto begin = version_info_str.begin();
    auto end = version_info_str.end();
    {
        // r=1 c=20
        static const char fmt2Translate[] = N_("Firmware Version\n");
        char fmt[21];
        _(fmt2Translate).copyToRAM(fmt, sizeof(fmt)); // note the underscore at the beginning of this line
        begin += snprintf(begin, end - begin, fmt);
    }

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

    if (end > begin) {
        // c=20 r=4
        static const char fmt2Translate[] = N_("\nBootloader Version\n%d.%d.%d\n\nBuddy Board\n%d.%d.%d\n%s");
        char fmt[20 * 4];
        _(fmt2Translate).copyToRAM(fmt, sizeof(fmt)); // note the underscore at the beginning of this line
        begin += snprintf(begin, end - begin,
            fmt,
            bootloader->major, bootloader->minor, bootloader->patch,
            board_version[0], board_version[1], board_version[2],
            serial_numbers);
    }

    // this MakeRAM is safe - version_info_str is allocated in RAM for the lifetime of ths
    help.SetText(string_view_utf8::MakeRAM((const uint8_t *)version_info_str.data()));
}

ScreenFactory::UniquePtr GetScreenMenuVersionInfo() {
    return ScreenFactory::Screen<ScreenMenuVersionInfo>();
}
