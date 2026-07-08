/**
 * @file screen_menu_temperature.hpp
 */
#pragma once

#include <option/has_chamber_api.h>
#include <option/has_indx.h>
#include <option/has_toolchanger.h>
#include <option/xbuddy_extension_variant.h>

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_print.hpp"
#include "MItem_filament.hpp"
#include <gui/menu_item/menu_item_virtual_submenu.hpp>
#include <gui/menu_item/menu_item_extensions/with_icon.hpp>
#include <window_menu_callback_item.hpp>

#if HAS_CHAMBER_API()
    #include <gui/menu_item/specific/menu_items_chamber.hpp>
#endif
#if XBUDDY_EXTENSION_VARIANT_IS_STANDARD()
    #include <gui/menu_item/specific/menu_items_xbuddy_extension.hpp>
#endif

/*****************************************************************************/
// parent alias
namespace screen_menu_temperature {
#if HAS_CHAMBER_API()
using MI_CHAMBER_TARGET_TEMP = WithConstructorArgs<::MI_CHAMBER_TARGET_TEMP, HAS_MINI_DISPLAY() ? N_("Chamber") : N_("Chamber Temperature")>;
#endif

using MI_COOLDOWN = WithConstructorArgs<WindowMenuCallbackItem, N_("Cooldown"), nullptr>;

using ScreenBase = ScreenMenu<
    EFooter::On, MI_RETURN,
    MI_NOZZLE_TARGET_TEMP,
#if HAS_TOOLCHANGER()
    // Multi-tool: additional submenu to set target temp for all tools, not just the active one.
    WithIcon<MenuItemVirtualSubmenu<N_("Nozzle Temperatures"), MI_NOZZLE_TARGET_TEMP, PhysicalToolIndex::count, PhysicalToolIndex::from_raw>,
        &img::nozzle_16x16>,
#endif
    MI_HEATBED,
#if HAS_CHAMBER_API()
    MI_CHAMBER_TARGET_TEMP,
#endif
    MI_PRINTFAN,
#if HAS_INDX()
    MI_DOCKFAN,
#endif
#if XBUDDY_EXTENSION_VARIANT_IS_STANDARD()
    MI_XBUDDY_EXTENSION_COOLING_FANS,
    MI_XBUDDY_EXTENSION_COOLING_FANS_CONTROL_MAX,
    MI_XBE_FILTRATION_FAN,
#endif
    MI_AUTO_COOLDOWN,
    MI_COOLDOWN>;

} // namespace screen_menu_temperature

class ScreenMenuTemperature : public screen_menu_temperature::ScreenBase {
public:
    ScreenMenuTemperature();
};
