/**
 * @file screen_menu_temperature.hpp
 */
#pragma once

#include <option/has_chamber_api.h>
#include <option/has_xbuddy_extension.h>

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_print.hpp"
#include "MItem_filament.hpp"

#if HAS_CHAMBER_API()
    #include <gui/menu_item/specific/menu_items_chamber.hpp>
#endif
#if HAS_XBUDDY_EXTENSION()
    #include <gui/menu_item/specific/menu_items_xbuddy_extension.hpp>
#endif

/*****************************************************************************/
// parent alias
namespace screen_menu_temperature {
#if HAS_CHAMBER_API()
using MI_CHAMBER_TARGET_TEMP = WithConstructorArgs<::MI_CHAMBER_TARGET_TEMP, HAS_MINI_DISPLAY() ? N_("Chamber") : N_("Chamber Temperature")>;
#endif

using ScreenBase = ScreenMenu<
    EFooter::On, MI_RETURN, MI_NOZZLE<0>,
#if HAS_TOOLCHANGER()
    MI_NOZZLE<1>, MI_NOZZLE<2>, MI_NOZZLE<3>, MI_NOZZLE<4>,
#endif
    MI_HEATBED,
#if HAS_CHAMBER_API()
    MI_CHAMBER_TARGET_TEMP,
#endif
    MI_PRINTFAN,
#if HAS_XBUDDY_EXTENSION()
    MI_XBUDDY_EXTENSION_COOLING_FANS,
#endif
    MI_COOLDOWN>;

} // namespace screen_menu_temperature

class ScreenMenuTemperature : public screen_menu_temperature::ScreenBase {
public:
    constexpr static const char *label = N_("TEMPERATURE");
    ScreenMenuTemperature();

protected:
    virtual void windowEvent(window_t *sender, GUI_event_t event, void *param) override;
};
