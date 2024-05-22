/**
 * @file screen_menu_version_info_mini.hpp
 */

#pragma once

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "fonts.hpp"

using MenuInfoContainer = WinMenuContainer<MI_RETURN>;

class ScreenMenuVersionInfo : public AddSuperWindow<screen_t> {
    static constexpr uint8_t VERSION_INFO_STR_MAXLEN = 150;
    static constexpr uint8_t blank_space_h = 10; // Visual bottom padding for HELP string
    std::array<char, VERSION_INFO_STR_MAXLEN> version_info_str;
    constexpr static const char *label = N_("VERSION INFO");
    static constexpr size_t helper_lines = 10;
    static constexpr Font helper_font = Font::normal;

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
