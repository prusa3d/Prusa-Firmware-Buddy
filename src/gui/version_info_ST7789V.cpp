/**
 * @file version_info_ST7789V.cpp
 */

#include "screen_menu_version_info.hpp"
#include "config.h"
#include "version.h"
#include "shared_config.h" //BOOTLOADER_VERSION_ADDRESS
#include "../common/otp.h"
#include "png_resources.hpp"

uint16_t ScreenMenuVersionInfo::get_help_h() {
    return helper_lines * (resource_font(helper_font)->h + 1); // +1 for line paddings
}

ScreenMenuVersionInfo::ScreenMenuVersionInfo()
    : AddSuperWindow<screen_t>(nullptr)
    , menu(this, GuiDefaults::RectScreenBody - Rect16::Height_t(get_help_h()), &container)
    , header(this)
    , help(this, Rect16(GuiDefaults::RectScreen.Left(), uint16_t(GuiDefaults::RectFooter.Top()) - get_help_h() - blank_space_h, GuiDefaults::RectScreen.Width(), get_help_h()), is_multiline::yes)
    , footer(this) {
    header.SetText(_(label));
    help.set_font(resource_font(helper_font));
    CaptureNormalWindow(menu); // set capture to list

    //=============SCREEN INIT===============
    header.SetIcon(&png::info_16x16);

    //=============VARIABLES=================

    const version_t *bootloader = (const version_t *)BOOTLOADER_VERSION_ADDRESS;

    //=============ACCESS IN ADDR=================
    board_revision_t board_revision;
    if (otp_get_board_revision(&board_revision) == false) {
        board_revision.br = 0;
    }

    serial_nr_t serial_nr;
    otp_get_serial_nr(&serial_nr);

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
        static const char fmt2Translate[] = N_("\nBootloader Version\n%d.%d.%d\n\nBuddy Board\n%d.%d\n%s");
        char fmt[20 * 4];
        _(fmt2Translate).copyToRAM(fmt, sizeof(fmt)); // note the underscore at the beginning of this line
        begin += snprintf(begin, end - begin,
            fmt,
            bootloader->major, bootloader->minor, bootloader->patch,
            board_revision.bytes[0], board_revision.bytes[1],
            serial_nr.txt);
    }

    // this MakeRAM is safe - version_info_str is allocated in RAM for the lifetime of this
    help.SetText(string_view_utf8::MakeRAM((const uint8_t *)version_info_str.data()));
    EnableLongHoldScreenAction();
}
