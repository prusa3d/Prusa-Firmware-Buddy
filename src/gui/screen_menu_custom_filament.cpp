/**
 * @file screen_menu_filament.cpp
 */

#include "screen_menu_custom_filament.hpp"
#include "filament.hpp"
#include "filament_sensors_handler.hpp"
#include "custom_filament_tools.hpp"
#include "img_resources.hpp"

#include "DialogHandler.hpp"
ScreenMenuCustomFilament::ScreenMenuCustomFilament()
    : ScreenMenuCustomFilament__(_(label)) {
    EnableLongHoldScreenAction();
}

void ScreenMenuCustomFilament::windowEvent(EventLock /*has private ctor*/, [[maybe_unused]] window_t *sender, [[maybe_unused]] GUI_event_t event, [[maybe_unused]] void *param) {
    if (event == GUI_event_t::CHILD_CLICK) {
        // uint8_t slot = *(int8_t *)param;
        Item<MI_CUSTOM_FILAMENT_NOZZLE_TEMP>().SetVal(custom_filament_tools::GetSlotTemp(custom_filament_tools::CustomFilamentTemperatures::nozzle));
        Item<MI_CUSTOM_FILAMENT_NOZZLE_PREHEAT_TEMP>().SetVal(custom_filament_tools::GetSlotTemp(custom_filament_tools::CustomFilamentTemperatures::nozzle_preheat));
        Item<MI_CUSTOM_FILAMENT_HEATBED_TEMP>().SetVal(custom_filament_tools::GetSlotTemp(custom_filament_tools::CustomFilamentTemperatures::heatbed));
    }
}
