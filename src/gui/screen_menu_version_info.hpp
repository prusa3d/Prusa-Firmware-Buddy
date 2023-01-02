/**
 * @file screen_menu_version_info.hpp
 */

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "i18n.h"

#pragma once

using MenuInfoContainer = WinMenuContainer<MI_RETURN>;

class ScreenMenuVersionInfo : public AddSuperWindow<screen_t> {
    static constexpr uint8_t VERSION_INFO_STR_MAXLEN = 150;
    static constexpr uint8_t blank_space_h = 10; // Visual bottom padding for HELP string
    std::array<char, VERSION_INFO_STR_MAXLEN> version_info_str;
    constexpr static const char *label = N_("VERSION INFO");
    static constexpr size_t helper_lines = 10;
    static constexpr ResourceId helper_font = IDR_FNT_NORMAL;

    MenuInfoContainer container;
    window_menu_t menu;
    window_header_t header;
    window_text_t help;
    StatusFooter footer;

public:
    ScreenMenuVersionInfo();

protected:
    static uint16_t get_help_h();
};
