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
#include <option/has_control_menu.h>

#if HAS_MMU2
    #include "MItem_mmu.hpp"
#endif

/*****************************************************************************/

#if (PRINTER_TYPE == PRINTER_PRUSA_MK4 || PRINTER_TYPE == PRINTER_PRUSA_MK3_5 || PRINTER_TYPE == PRINTER_PRUSA_XL || PRINTER_TYPE == PRINTER_PRUSA_iX)
using ScreenMenuSettings__ = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN,
    #if not HAS_CONTROL_MENU()
    MI_TEMPERATURE, MI_MOVE_AXIS, MI_DISABLE_STEP,
    #endif
    MI_FILAMENT_SENSOR,
    #if HAS_MMU2
    MI_MMU_ENABLE,
    #endif
    MI_FAN_CHECK,
    #if ENABLED(CRASH_RECOVERY)
    MI_CRASH_DETECTION,
    #endif // ENABLED(CRASH_RECOVERY)
    #if HAS_TOOLCHANGER()
    MI_TOOLS_SETUP,
    #endif
    MI_USER_INTERFACE, MI_LANG_AND_TIME, MI_NETWORK, MI_HARDWARE, MI_SYSTEM
    #if PRINTER_TYPE == PRINTER_PRUSA_MK4
    ,
    MI_INPUT_SHAPER
    #endif
    #ifdef _DEBUG
    ,
    MI_TEST
    #endif
    >;
#else // PRINTER_TYPE == PRINTER_PRUSA_MK4 || PRINTER_TYPE == PRINTER_PRUSA_XL || PRINTER_TYPE == PRINTER_PRUSA_iX

    #ifdef _DEBUG
using ScreenMenuSettings__ = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN, MI_TEMPERATURE, MI_CURRENT_PROFILE, MI_MOVE_AXIS, MI_DISABLE_STEP,
    MI_FOOTER_SETTINGS, MI_SERVICE, MI_HW_SETUP, MI_TEST, MI_FW_UPDATE, MI_FILAMENT_SENSOR, MI_FS_AUTOLOAD, MI_TIMEOUT, MI_FAN_CHECK,
        #ifdef BUDDY_ENABLE_ETHERNET
    MI_NETWORK,
    MI_TIMEZONE,
    MI_LOAD_SETTINGS,
        #endif // BUDDY_ENABLE_ETHERNET
        #ifdef BUDDY_ENABLE_DFU_ENTRY
    MI_ENTER_DFU,
        #endif
    MI_USB_MSC_ENABLE,
    MI_SAVE_DUMP, MI_SOUND_MODE, MI_SOUND_VOLUME,
    MI_DEVHASH_IN_QR, MI_LANGUAGE, MI_LANGUAGUE_USB, MI_LANGUAGUE_XFLASH, MI_LOAD_LANG, MI_SORT_FILES,
    MI_SOUND_TYPE, MI_XFLASH_RESET, MI_HF_TEST_0, MI_HF_TEST_1,
    MI_EEPROM, MI_EXPERIMENTAL_SETTINGS, MI_OPEN_FACTORY_RESET>;
    #else
using ScreenMenuSettings__ = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN, MI_TEMPERATURE, MI_CURRENT_PROFILE, MI_MOVE_AXIS, MI_DISABLE_STEP,
    MI_FOOTER_SETTINGS, MI_HW_SETUP, MI_FW_UPDATE, MI_FILAMENT_SENSOR, MI_FS_AUTOLOAD, MI_TIMEOUT, MI_FAN_CHECK,
        #ifdef BUDDY_ENABLE_DFU_ENTRY
    MI_ENTER_DFU,
        #endif
        #ifdef BUDDY_ENABLE_ETHERNET
    MI_USB_MSC_ENABLE,
    MI_NETWORK,
    MI_TIMEZONE,
    MI_LOAD_SETTINGS,
        #endif // BUDDY_ENABLE_ETHERNET
        #if ENABLED(CRASH_RECOVERY)
    MI_CRASH_DETECTION,
        #endif // ENABLED(CRASH_RECOVERY)
    MI_SAVE_DUMP, MI_SOUND_MODE, MI_SOUND_VOLUME, MI_DEVHASH_IN_QR,
    MI_LANGUAGE, MI_OPEN_FACTORY_RESET>;
    #endif     // _DEBUG
#endif         // PRINTER_TYPE == PRINTER_PRUSA_MK4 || PRINTER_TYPE == PRINTER_PRUSA_MK3_5 || PRINTER_TYPE == PRINTER_PRUSA_XL || PRINTER_TYPE == PRINTER_PRUSA_iX

class ScreenMenuSettings : public ScreenMenuSettings__ {
    gui::knob::screen_action_cb old_action;

public:
    constexpr static const char *label = N_("SETTINGS");
    ScreenMenuSettings();
    ~ScreenMenuSettings();
};
