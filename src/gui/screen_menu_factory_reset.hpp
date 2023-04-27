#pragma once

#include "screen_menu.hpp"
#include "MItem_tools.hpp"

namespace detail {
using ScreenMenuFactoryReset = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN, MI_FACTORY_SOFT_RESET, MI_FACTORY_HARD_RESET>;
}

class ScreenMenuFactoryReset : public detail::ScreenMenuFactoryReset {
public:
    constexpr static const char *label = N_("FACTORY RESET");
    ScreenMenuFactoryReset();
};
