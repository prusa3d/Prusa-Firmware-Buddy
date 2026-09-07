/// @file
#include "screen_menu_info.hpp"

#include <img_resources.hpp>

ScreenMenuInfo::ScreenMenuInfo()
    : ScreenMenuInfo__ {
        _("INFO"),
        &img::info_16x16,
    } {
    EnableLongHoldScreenAction();
}
