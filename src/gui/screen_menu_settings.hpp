/// @file
#pragma once

#include <basic_screen_menu.hpp>
#include "MItem_menus.hpp"
#include "MItem_tools.hpp"
#include "knob_event.hpp"
#include "MItem_crash.hpp"
#include "Configuration_adv.h"
#include <option/has_crash_detection.h>
#include <option/has_mmu2.h>
#include <option/xbuddy_extension_variant.h>
#include <option/has_indx.h>
#include <option/has_lights_menu.h>
#include <option/has_wastebin_fill_tracking.h>
#include <option/development_items.h>

#if HAS_MMU2()
    #include "MItem_mmu.hpp"
#endif

#if XBUDDY_EXTENSION_VARIANT_IS_STANDARD()
    #include <gui/menu_item/specific/menu_items_xbuddy_extension.hpp>
#endif

#include <option/has_chamber_filtration_api.h>
#if HAS_CHAMBER_FILTRATION_API()
    #include <gui/menu_item/specific/menu_items_chamber_filtration.hpp>
#endif

#include <option/has_switchable_auto_retract.h>
#if HAS_SWITCHABLE_AUTO_RETRACT()
    #include <gui/menu_item/specific/menu_items_auto_retract.hpp>
#endif

#include <option/has_anfc.h>
#if HAS_ANFC()
    #include <screen/openprinttag/screen_opt_settings.hpp>
#endif

using ScreenMenuSettingsBase = BasicScreenMenu<
    MI_USER_INTERFACE,
    MI_TOOLHEAD_SETTINGS,
#if HAS_FILAMENT_SENSORS_MENU()
    MI_FILAMENT_SENSORS,
#else
    MI_FILAMENT_SENSOR,
#endif
#if HAS_LIGHTS_MENU()
    MI_LIGHTS,
#endif
    MI_NETWORK,
#if HAS_LOADCELL() && !HAS_INDX()
    MI_STUCK_FILAMENT_DETECTION,
#endif
#if HAS_SWITCHABLE_AUTO_RETRACT()
    MI_AUTO_RETRACT_ENABLE,
#endif
#if HAS_ANFC()
    buddy::openprinttag::MI_OPT_SETTINGS,
#endif
#if HAS_MMU2()
    MI_MMU_ENABLE,
    MI_MMU_BOOTLOADER_RESULT,
    MI_MMU_CUTTER,
#endif
#if XBUDDY_EXTENSION_VARIANT_IS_STANDARD()
    MI_CAM_USB_PWR,
#endif
    MI_STEALTH_MODE,
    MI_FAN_CHECK,
    MI_GCODE_VERIFY,
#if HAS_WASTEBIN_FILL_TRACKING()
    MI_WASTEBIN,
#endif
#if HAS_CHAMBER_FILTRATION_API()
    MI_CHAMBER_FILTRATION,
#endif
#if HAS_CRASH_DETECTION()
    MI_CRASH_DETECTION,
#endif
    MI_LANG_AND_TIME,
    MI_HARDWARE,
#if DEVELOPMENT_ITEMS()
    MI_DEVELOPMENT,
#endif
    // MI_SYSTEM needs to be last to ensure we can safely hit factory reset even in presence of unknown languages
    MI_SYSTEM>;

class ScreenMenuSettings final : public ScreenMenuSettingsBase {
    gui::knob::screen_action_cb old_action;

public:
    ScreenMenuSettings();
    ~ScreenMenuSettings();
};
