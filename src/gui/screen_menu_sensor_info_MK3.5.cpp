/**
 * @file screen_menu_sensor_info_MK3.5.cpp
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

void ScreenMenuSensorInfo::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::LOOP) {

        SensorData::Value boardRes = buffer.GetValue(SensorData::Sensor::boardTemp);
        Item<MI_INFO_BOARD_TEMP>().UpdateValue(boardRes);

        SensorData::Value res = buffer.GetValue(SensorData::Sensor::bedTemp);
        Item<MI_INFO_BED_TEMP>().UpdateValue(res);

        Item<MI_INFO_MCU_TEMP>().UpdateValue(buffer.GetValue(SensorData::Sensor::MCUTemp));

        Item<MI_INFO_NOZZLE_TEMP>().UpdateValue(marlin_vars()->hotend(0).temp_nozzle.get());

        if (auto fsensor = GetExtruderFSensor(marlin_vars()->active_extruder.get()); fsensor) { // Try to get extruder filament sensor
            Item<MI_INFO_PRINTER_FILL_SENSOR>().UpdateValue(std::make_pair(static_cast<int>(fsensor->get_state()), static_cast<int>(fsensor->GetFilteredValue())));
        } else {
            Item<MI_INFO_PRINTER_FILL_SENSOR>().UpdateValue({ {}, {} });
        }

        Item<MI_INFO_FINDA>().UpdateValue(marlin_vars()->mmu2_finda);

        res = buffer.GetValue(SensorData::Sensor::printFan);
        SensorData::Value res1 = buffer.GetValue(SensorData::Sensor::printFanAct);
        Item<MI_INFO_PRINT_FAN>().UpdateValue(std::make_pair(res, res1));

        res = buffer.GetValue(SensorData::Sensor::hbrFan);
        res1 = buffer.GetValue(SensorData::Sensor::hbrFanAct);
        Item<MI_INFO_HBR_FAN>().UpdateValue(std::make_pair(res, res1));

        res = buffer.GetValue(SensorData::Sensor::heaterVoltage);
        Item<MI_INFO_HEATER_VOLTAGE>().UpdateValue(res);

        res = buffer.GetValue(SensorData::Sensor::inputVoltage);
        Item<MI_INFO_INPUT_VOLTAGE>().UpdateValue(res);

        res = buffer.GetValue(SensorData::Sensor::heaterCurrent);
        Item<MI_INFO_HEATER_CURRENT>().UpdateValue(res);

        res = buffer.GetValue(SensorData::Sensor::inputCurrent);
        Item<MI_INFO_INPUT_CURRENT>().UpdateValue(res);

        res = buffer.GetValue(SensorData::Sensor::mmuCurrent);
        Item<MI_INFO_MMU_CURRENT>().UpdateValue(res);

        Item<MI_PINDA>().UpdateValue(buddy::hw::zMin.read() == buddy::hw::Pin::State::low);
    }

    if (event == GUI_event_t::HELD_RELEASED) {
        DialogMoveZ::Show();
        return;
    }

    SuperWindowEvent(sender, event, param);
}
