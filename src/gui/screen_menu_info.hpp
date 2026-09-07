/// @file
#pragma once

#include <basic_screen_menu.hpp>
#include "MItem_menus.hpp"
#include <screen_menu_statistics.hpp>

using ScreenMenuInfo__ = BasicScreenMenu<
    MI_STATISTICS,
    MI_NETWORK_STATUS,
    MI_SENSOR_INFO,
    MI_VERSION_INFO,
    MI_HELP_FW_UPDATE>;

class ScreenMenuInfo final : public ScreenMenuInfo__ {
public:
    ScreenMenuInfo();
};
