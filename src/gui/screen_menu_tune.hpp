/**
 * @file screen_menu_tune.hpp
 */
#pragma once

#include "screen_menu.hpp"
#include "MItem_hardware.hpp"
#include "MItem_print.hpp"
#include "MItem_tools.hpp"
#include <gui/menu_item/menu_item_virtual_submenu.hpp>
#include <gui/menu_item/menu_item_extensions/with_icon.hpp>
#include "MItem_crash.hpp"
#include "MItem_menus.hpp"
#include "MItem_mmu.hpp"
#include <device/board.h>
#include "config_features.h"
#include <option/has_crash_detection.h>
#include <option/has_emergency_stop.h>
#include <option/has_chamber_api.h>
#include <option/has_loadcell.h>
#include <option/has_power_panic.h>
#include <option/has_toolchanger.h>
#include <option/has_mmu2.h>
#include <option/xbuddy_extension_variant.h>
#include <option/has_chamber_filtration_api.h>
#include <option/has_indx.h>
#include <option/has_wastebin_fill_tracking.h>
#include <option/has_lights_menu.h>
#include <device/board.h>
#include <gui/screen/screen_m600.hpp>

#if XL_ENCLOSURE_SUPPORT()
    #include "MItem_enclosure.hpp"
#endif
#if HAS_CHAMBER_API()
    #include <gui/menu_item/specific/menu_items_chamber.hpp>
#endif
#if XBUDDY_EXTENSION_VARIANT_IS_STANDARD()
    #include <gui/menu_item/specific/menu_items_xbuddy_extension.hpp>
#endif

#include <option/has_cancel_object.h>
#if HAS_CANCEL_OBJECT()
    #include <gui/screen/screen_cancel_objects.hpp>
#endif

/*****************************************************************************/
// parent alias
using ScreenMenuTune__ = ScreenMenu<EFooter::On, MI_RETURN,
#if !HAS_LOADCELL()
    MI_LIVE_ADJUST_Z, // position without loadcell
#endif
    MI_M600,
#if HAS_WASTEBIN_FILL_TRACKING()
    MI_WASTEBIN,
#endif

#if HAS_CANCEL_OBJECT()
    MI_CO_CANCEL_OBJECT,
#endif
    MI_SPEED,
    MI_NOZZLE_TARGET_TEMP,
    MI_HEATBED,
    MI_PRINTFAN,
    MI_TEMPERATURE_AND_FANS,
#if HAS_CHAMBER_FILTRATION_API()
    MI_CHAMBER_FILTRATION,
#endif
#if HAS_LOADCELL()
    MI_LIVE_ADJUST_Z, // position with loadcell
#endif
    MI_FLOW_FACTOR,
#if EXTRUDERS > 1
    MenuItemVirtualSubmenu<N_("Flow Factors"), MI_FLOW_FACTOR, VirtualToolIndex::count, VirtualToolIndex::from_raw>,
#endif
#if HAS_FILAMENT_SENSORS_MENU()
    MI_FILAMENT_SENSORS,
#else
    MI_FILAMENT_SENSOR,
#endif
#if HAS_LOADCELL() && !HAS_INDX()
    MI_STUCK_FILAMENT_DETECTION,
#endif
#if XL_ENCLOSURE_SUPPORT()
    MI_ENCLOSURE_ENABLE,
    MI_ENCLOSURE,
#endif
    MI_STEALTH_MODE,
    MI_INPUT_SHAPER,
    MI_FAN_CHECK,
    MI_GCODE_VERIFY,
#if HAS_EMERGENCY_STOP()
    MI_EMERGENCY_STOP_ENABLE,
#endif
#if HAS_MMU2()
    MI_MMU_CUTTER,
    MI_MMU_INVOKE_MAINTENANCE,
#endif // HAS_MMU2()
#if HAS_CRASH_DETECTION()
    MI_CRASH_DETECTION,
    MI_CRASH_SENSITIVITY_XY,
#endif
    MI_USER_INTERFACE,
#if HAS_LIGHTS_MENU()
    MI_LIGHTS,
#endif
    MI_NETWORK,
#if DEVELOPMENT_ITEMS()
    MI_HARDWARE_TUNE,
#endif
    MI_LANG_AND_TIME,
    MI_INFO,
#if HAS_POWER_PANIC()
    MI_TRIGGER_POWER_PANIC,
#endif
    MI_MESSAGES>;

class ScreenMenuTune : public ScreenMenuTune__ {
public:
    constexpr static const char *label = N_("TUNE");
    ScreenMenuTune();

protected:
    virtual void windowEvent(window_t *sender, GUI_event_t event, void *param) override;
};
