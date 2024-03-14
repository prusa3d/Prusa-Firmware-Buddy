/**
 * @file screen_menu_settings.hpp
 */
#pragma once

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_menus.hpp"
#include "MItem_tools.hpp"
#include "menu_items_languages.hpp"
#include "knob_event.hpp"
#include "MItem_crash.hpp"
#include "Configuration_adv.h"
#include <option/has_mmu2.h>
#include <option/developer_mode.h>

#if HAS_MMU2()
    #include "MItem_mmu.hpp"
#endif

class MI_HELP_FW_UPDATE : public WI_LABEL_t {
    static constexpr const char *const label = N_("FW update");

public:
    MI_HELP_FW_UPDATE();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

/*****************************************************************************/

using ScreenMenuSettings__ = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN,
#if !HAS_LOADCELL()
    MI_CURRENT_PROFILE,
#endif
    MI_FILAMENT_SENSOR,
#if HAS_LOADCELL()
    MI_STUCK_FILAMENT_DETECTION,
#endif
#if HAS_MMU2()
    MI_MMU_ENABLE,
    MI_MMU_CUTTER,
#endif
    MI_FAN_CHECK,
#if ENABLED(CRASH_RECOVERY)
    MI_CRASH_DETECTION,
#endif // ENABLED(CRASH_RECOVERY)
#if HAS_TOOLCHANGER()
    MI_TOOLS_SETUP,
#endif
    MI_INPUT_SHAPER,
#if DEVELOPER_MODE()
    MI_ERROR_TEST,
#endif
#ifdef _DEBUG
    MI_TEST,
#endif
    MI_USER_INTERFACE, MI_LANG_AND_TIME, MI_NETWORK, MI_HARDWARE, MI_HELP_FW_UPDATE,
    // MI_SYSTEM needs to be last to ensure we can safely hit factory reset even in presence of unknown languages
    MI_SYSTEM>;
class ScreenMenuSettings : public ScreenMenuSettings__ {
    gui::knob::screen_action_cb old_action;

public:
    constexpr static const char *label = N_("SETTINGS");
    ScreenMenuSettings();
    ~ScreenMenuSettings();
};
