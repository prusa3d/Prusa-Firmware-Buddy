/// @file
#include "screen_menu_development.hpp"

#include <img_resources.hpp>
#include <lang/string_view_utf8.hpp>

ScreenMenuDevelopment::ScreenMenuDevelopment()
    : ScreenMenuDevelopmentBase {
        /// dev item intentionally not translated
        string_view_utf8::MakeCPUFLASH("DEVELOPMENT"),
        &img::settings_16x16,
    } {}
