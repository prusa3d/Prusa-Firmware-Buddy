/**
 * @file screen_menu_temperature.hpp
 */
#pragma once

#include <option/has_chamber_api.h>

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_print.hpp"
#include "MItem_filament.hpp"
#include <window_menu_callback_item.hpp>

#if HAS_CHAMBER_API()
    #include <gui/menu_item/specific/menu_items_chamber.hpp>
#endif

/*****************************************************************************/
// parent alias
namespace screen_menu_temperature {
#if HAS_CHAMBER_API()
using MI_CHAMBER_TARGET_TEMP = WithConstructorArgs<::MI_CHAMBER_TARGET_TEMP, HAS_MINI_DISPLAY() ? N_("Chamber") : N_("Chamber Temperature")>;
#endif

using MI_COOLDOWN = WithConstructorArgs<WindowMenuCallbackItem, N_("Cooldown"), nullptr>;

using ScreenBase = ScreenMenu<
    EFooter::On, MI_RETURN, MI_NOZZLE<0>,
#if HAS_TOOLCHANGER()
    MI_NOZZLE<1>, MI_NOZZLE<2>, MI_NOZZLE<3>, MI_NOZZLE<4>,
#endif
    MI_HEATBED,
#if HAS_CHAMBER_API()
    MI_CHAMBER_TARGET_TEMP,
#endif
    MI_PRINTFAN, MI_COOLDOWN>;

} // namespace screen_menu_temperature

class ScreenMenuTemperature : public screen_menu_temperature::ScreenBase {
public:
    ScreenMenuTemperature();
};
