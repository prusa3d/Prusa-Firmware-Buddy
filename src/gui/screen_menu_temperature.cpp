/**
 * @file screen_menu_temperature.cpp
 */

#include "screen_menu_temperature.hpp"
#include "marlin_client.hpp"
#include "ScreenHandler.hpp"
#include "DialogMoveZ.hpp"
#include "png_resources.hpp"
#include <option/has_toolchanger.h>

ScreenMenuTemperature::ScreenMenuTemperature()
    : ScreenMenuTemperature__(_(label)) {
    EnableLongHoldScreenAction();

#if (PRINTER_TYPE != PRINTER_PRUSA_MINI)
    header.SetIcon(&png::temperature_white_16x16);
#endif //PRINTER_PRUSA_MINI
}

void ScreenMenuTemperature::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::CHILD_CLICK) {
        HOTEND_LOOP() {
            marlin_set_target_nozzle(0, e);
            marlin_set_display_nozzle(0, e);
        }
        marlin_set_target_bed(0);
        marlin_set_fan_speed(0);
        Item<MI_NOZZLE<0>>().SetVal(0);
#if HAS_TOOLCHANGER()
        Item<MI_NOZZLE<1>>().SetVal(0);
        Item<MI_NOZZLE<2>>().SetVal(0);
        Item<MI_NOZZLE<3>>().SetVal(0);
        Item<MI_NOZZLE<4>>().SetVal(0);
#endif
        Item<MI_HEATBED>().SetVal(0);
        Item<MI_PRINTFAN>().SetVal(0);
    } else {
        SuperWindowEvent(sender, event, param);
    }
}
