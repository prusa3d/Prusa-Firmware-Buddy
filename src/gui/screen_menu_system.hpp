/**
 * @file screen_menu_system.hpp
 */

#pragma once

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_tools.hpp"
#include "MItem_menus.hpp"
#include <option/has_file_log.h>

using ScreenMenuSystem__ = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN, MI_SAVE_DUMP,
#if HAS_FILE_LOG()
    MI_LOG_TO_TXT,
#endif
    MI_DEVHASH_IN_QR, MI_FW_UPDATE, MI_LOAD_SETTINGS, MI_OPEN_FACTORY_RESET>;

class ScreenMenuSystem : public ScreenMenuSystem__ {
    virtual void windowEvent(window_t *sender, GUI_event_t event, void *param) override;

public:
    constexpr static const char *label = N_("SYSTEM");
    ScreenMenuSystem();
};
