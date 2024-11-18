/**
 * @file screen_menu_temperature.cpp
 */

#include "screen_menu_temperature.hpp"
#include "marlin_client.hpp"
#include "ScreenHandler.hpp"
#include "DialogMoveZ.hpp"
#include "img_resources.hpp"
#include <option/has_toolchanger.h>
#include <option/has_chamber_api.h>

#if HAS_CHAMBER_API()
    #include <feature/chamber/chamber.hpp>
#endif

ScreenMenuTemperature::ScreenMenuTemperature()
    : screen_menu_temperature::ScreenBase(_(label)) {
    EnableLongHoldScreenAction();

#if (!PRINTER_IS_PRUSA_MINI())
    header.SetIcon(&img::temperature_white_16x16);
#endif // PRINTER_IS_PRUSA_MINI()
}

void ScreenMenuTemperature::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::CHILD_CLICK) {
        HOTEND_LOOP() {
            marlin_client::set_target_nozzle(0, e);
            marlin_client::set_display_nozzle(0, e);
        }
        marlin_client::set_target_bed(0);
        marlin_client::set_fan_speed(0);
        Item<MI_NOZZLE<0>>().SetVal(0);
#if HAS_TOOLCHANGER()
        Item<MI_NOZZLE<1>>().SetVal(0);
        Item<MI_NOZZLE<2>>().SetVal(0);
        Item<MI_NOZZLE<3>>().SetVal(0);
        Item<MI_NOZZLE<4>>().SetVal(0);
#endif
        Item<MI_HEATBED>().SetVal(0);
        Item<MI_PRINTFAN>().SetVal(0);
#if HAS_CHAMBER_API()
        if (buddy::chamber().capabilities().heating) {
            Item<screen_menu_temperature::MI_CHAMBER_TARGET_TEMP>().SetVal(0);
        }
#endif
    } else {
        ScreenMenu::windowEvent(sender, event, param);
    }
}
