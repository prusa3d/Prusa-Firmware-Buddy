/**
 * @file screen_menu_sensor_info_MK4.cpp
 */

#include "screen_menu_sensor_info.hpp"
#include "ScreenHandler.hpp"
#include "MItem_tools.hpp"
#include "DialogMoveZ.hpp"

#include "i_window_menu_container.hpp"
#include <tuple>

#include "marlin_client.hpp"
#include "cmath_ext.h"
#include "../Marlin/src/module/configuration_store.h"

#include <filament_sensor.hpp>
#include <filament_sensors_handler.hpp>

void ScreenMenuSensorInfo::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::LOOP) {
#if HAS_TEMP_BOARD
        Item<MI_INFO_BOARD_TEMP>().UpdateValue(sensor_data().boardTemp);
#endif
        Item<MI_INFO_MCU_TEMP>().UpdateValue(sensor_data().MCUTemp);
        Item<MI_INFO_BED_TEMP>().UpdateValue(sensor_data().bedTemp);
        Item<MI_INFO_HEATBREAK_TEMP>().UpdateValue(marlin_vars().hotend(0).temp_heatbreak.get());
        Item<MI_INFO_NOZZLE_TEMP>().UpdateValue(marlin_vars().hotend(0).temp_nozzle.get());
        Item<MI_INFO_LOADCELL>().UpdateValue(sensor_data().loadCell);
        Item<MI_INFO_PRINTER_FILL_SENSOR>().UpdateValue(GetExtruderFSensor(marlin_vars().active_extruder.get()));
        Item<MI_INFO_FINDA>().UpdateValue(marlin_vars().mmu2_finda);
        Item<MI_INFO_PRINT_FAN>().UpdateValue(sensor_data().printFan, sensor_data().printFanAct);
        Item<MI_INFO_HBR_FAN>().UpdateValue(sensor_data().hbrFan, sensor_data().hbrFanAct);
        Item<MI_INFO_HEATER_VOLTAGE>().UpdateValue(sensor_data().heaterVoltage);
        Item<MI_INFO_INPUT_VOLTAGE>().UpdateValue(sensor_data().inputVoltage);
        Item<MI_INFO_HEATER_CURRENT>().UpdateValue(sensor_data().heaterCurrent);
        Item<MI_INFO_INPUT_CURRENT>().UpdateValue(sensor_data().inputCurrent);
        Item<MI_INFO_MMU_CURRENT>().UpdateValue(sensor_data().mmuCurrent);
    }

    if (event == GUI_event_t::HELD_RELEASED) {
        DialogMoveZ::Show();
        return;
    }

    ScreenMenu::windowEvent(sender, event, param);
}
