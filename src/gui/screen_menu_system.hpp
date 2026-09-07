/// @file
#pragma once

#include "MItem_menus.hpp"
#include "MItem_tools.hpp"
#include <basic_screen_menu.hpp>
#include <option/has_e2ee_support.h>

using ScreenMenuSystemBase = BasicScreenMenu<
    MI_SAVE_DUMP,
    MI_LOG_TO_TXT,
    MI_DEVHASH_IN_QR,
    MI_LOAD_SETTINGS,
#if HAS_E2EE_SUPPORT()
    MI_E2EE,
#endif
    MI_OPEN_FACTORY_RESET>;

class ScreenMenuSystem final : public ScreenMenuSystemBase {
public:
    ScreenMenuSystem();
};
