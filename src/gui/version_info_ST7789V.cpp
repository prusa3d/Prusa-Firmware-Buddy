/**
 * @file version_info_ST7789V.cpp
 */

#include "screen_menu_version_info.hpp"
#include "config.h"
#include <version/version.hpp>
#include "shared_config.h" //BOOTLOADER_VERSION_ADDRESS
#include "../common/otp.hpp"
#include "img_resources.hpp"

uint16_t ScreenMenuVersionInfo::get_help_h() {
    return helper_lines * (height(helper_font) + 1); // +1 for line paddings
}

ScreenMenuVersionInfo::ScreenMenuVersionInfo()
    : screen_t(nullptr)
    , menu(this, GuiDefaults::RectScreenBody - Rect16::Height_t(get_help_h() + blank_space_h /*Avoid drawing over help text*/), &container)
    , header(this)
    , help(this, Rect16(GuiDefaults::MenuPaddingItems.left, uint16_t(GuiDefaults::RectFooter.Top()) - get_help_h() - blank_space_h, GuiDefaults::RectScreen.Width() - GuiDefaults::MenuPaddingItems.left, get_help_h()), is_multiline::yes)
    , footer(this) {
    header.SetText(_(label));
    help.set_font(helper_font);
    CaptureNormalWindow(menu); // set capture to list

    header.SetIcon(&img::info_16x16);

    const version_t *bootloader = (const version_t *)BOOTLOADER_VERSION_ADDRESS;

    serial_nr_t serial_nr;
    otp_get_serial_nr(serial_nr);

    auto begin = version_info_str.begin();
    auto end = version_info_str.end();
    {
        // r=1 c=20
        char fmt[21];
        _("Firmware Version\n").copyToRAM(fmt, sizeof(fmt)); // note the underscore at the beginning of this line
        begin += snprintf(begin, end - begin, fmt);
    }

    // TODO: Oh, this is bad. Someone really has to fix text wrapping.
    const int max_chars_per_line = 18;
    int project_version_full_len = strlen(version::project_version_full);

    for (int i = 0; i < project_version_full_len; i += max_chars_per_line) {
        int line_length;
        if ((project_version_full_len - i) < max_chars_per_line) {
            line_length = (project_version_full_len - i);
        } else {
            line_length = max_chars_per_line;
        }
        if (end > begin) {
            begin += snprintf(begin, end - begin, "%.*s\n", line_length, version::project_version_full + i);
        }
    }

    if (end > begin) {
        // c=20 r=4
        char fmt[20 * 4];
        _("\nBootloader Version\n%d.%d.%d\n\nBuddy Board\n%d\n%s").copyToRAM(fmt, sizeof(fmt)); // note the underscore at the beginning of this line
        begin += snprintf(begin, end - begin,
            fmt,
            bootloader->major, bootloader->minor, bootloader->patch,
            otp_get_board_revision().value_or(0),
            serial_nr.data());
    }

    // this MakeRAM is safe - version_info_str is allocated in RAM for the lifetime of this
    help.SetText(string_view_utf8::MakeRAM((const uint8_t *)version_info_str.data()));
    EnableLongHoldScreenAction();
}
