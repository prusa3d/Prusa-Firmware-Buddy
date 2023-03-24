/**
 * @file screen_menu_sensor_info.cpp
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

#if HAS_TEMP_BOARD && PRINTER_TYPE != PRINTER_PRUSA_MINI
        SensorData::Value boardRes = buffer.GetValue(SensorData::Sensor::boardTemp);
        Item<MI_INFO_BOARD_TEMP>().UpdateValue(boardRes);
#endif

#if PRINTER_TYPE == PRINTER_PRUSA_MK4 || PRINTER_TYPE == PRINTER_PRUSA_IXL
    #if (TEMP_SENSOR_HEATBREAK > 0)
        Item<MI_INFO_HEATBREAK_N_TEMP<0>>().UpdateValue(marlin_vars()->hotend(0).temp_heatbreak.get());
    #endif

        SensorData::Value res = buffer.GetValue(SensorData::Sensor::bedTemp);
        Item<MI_INFO_BED_TEMP>().UpdateValue(res);

        Item<MI_INFO_NOZZLE_TEMP>().UpdateValue(marlin_vars()->hotend(0).temp_nozzle.get());

        Item<MI_INFO_LOADCELL>().UpdateValue(buffer.GetValue(SensorData::Sensor::loadCell));

        if (auto fsensor = GetExtruderFSensor(marlin_vars()->active_extruder.get()); fsensor) { // Try to get extruder filament sensor
            Item<MI_INFO_PRINTER_FILL_SENSOR>().UpdateValue(std::make_pair(static_cast<int>(fsensor->Get()), static_cast<int>(fsensor->GetFilteredValue())));
        } else {
            Item<MI_INFO_PRINTER_FILL_SENSOR>().UpdateValue({ {}, {} });
        }

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
#elif PRINTER_TYPE == PRINTER_PRUSA_XL
        Item<MI_INFO_HEATBREAK_N_TEMP<0>>().UpdateValue(marlin_vars()->hotend(0).temp_heatbreak.get());
        Item<MI_INFO_HEATBREAK_N_TEMP<1>>().UpdateValue(marlin_vars()->hotend(1).temp_heatbreak.get());
        Item<MI_INFO_HEATBREAK_N_TEMP<2>>().UpdateValue(marlin_vars()->hotend(2).temp_heatbreak.get());
        Item<MI_INFO_HEATBREAK_N_TEMP<3>>().UpdateValue(marlin_vars()->hotend(3).temp_heatbreak.get());
        Item<MI_INFO_HEATBREAK_N_TEMP<4>>().UpdateValue(marlin_vars()->hotend(4).temp_heatbreak.get());

        SensorData::Value res = buffer.GetValue(SensorData::Sensor::bedTemp);
        Item<MI_INFO_BED_TEMP>().UpdateValue(res);

        Item<MI_INFO_NOZZLE_N_TEMP<0>>().UpdateValue(marlin_vars()->hotend(0).temp_nozzle.get());
        Item<MI_INFO_NOZZLE_N_TEMP<1>>().UpdateValue(marlin_vars()->hotend(1).temp_nozzle.get());
        Item<MI_INFO_NOZZLE_N_TEMP<2>>().UpdateValue(marlin_vars()->hotend(2).temp_nozzle.get());
        Item<MI_INFO_NOZZLE_N_TEMP<3>>().UpdateValue(marlin_vars()->hotend(3).temp_nozzle.get());
        Item<MI_INFO_NOZZLE_N_TEMP<4>>().UpdateValue(marlin_vars()->hotend(4).temp_nozzle.get());

        Item<MI_INFO_DWARF_BOARD_TEMPERATURE>().UpdateValue(buffer.GetValue(SensorData::Sensor::dwarfBoardTemperature));

        Item<MI_INFO_MODULAR_BED_BOARD_TEMPERATURE>().UpdateValue(buffer.GetValue(SensorData::Sensor::mbedMCUTemperature));

        Item<MI_INFO_LOADCELL>().UpdateValue(buffer.GetValue(SensorData::Sensor::loadCell));

        if (auto fsensor = GetExtruderFSensor(marlin_vars()->active_extruder.get()); fsensor) { // Try to get extruder filament sensor
            Item<MI_INFO_PRINTER_FILL_SENSOR>().UpdateValue(std::make_pair(static_cast<int>(fsensor->Get()), static_cast<int>(fsensor->GetFilteredValue())));
        } else {
            Item<MI_INFO_PRINTER_FILL_SENSOR>().UpdateValue({ {}, {} });
        }

        if (auto fsensor = GetSideFSensor(marlin_vars()->active_extruder.get()); fsensor) { // Try to get side filament sensor
            Item<MI_INFO_SIDE_FILL_SENSOR>().UpdateValue(std::make_pair(static_cast<int>(fsensor->Get()), static_cast<int>(fsensor->GetFilteredValue())));
        } else {
            Item<MI_INFO_SIDE_FILL_SENSOR>().UpdateValue({ {}, {} });
        }

        Item<MI_INFO_PRINT_FAN>().UpdateValue(std::make_pair(
            buffer.GetValue(SensorData::Sensor::printFan),
            buffer.GetValue(SensorData::Sensor::printFanAct)));

        Item<MI_INFO_HBR_FAN>().UpdateValue(std::make_pair(
            buffer.GetValue(SensorData::Sensor::hbrFan),
            buffer.GetValue(SensorData::Sensor::hbrFanAct)));

        res = buffer.GetValue(SensorData::Sensor::inputVoltage);
        Item<MI_INFO_INPUT_VOLTAGE>().UpdateValue(res);

        res = buffer.GetValue(SensorData::Sensor::sandwich5VVoltage);
        Item<MI_INFO_5V_VOLTAGE>().UpdateValue(res);

        res = buffer.GetValue(SensorData::Sensor::sandwich5VCurrent);
        Item<MI_INFO_SANDWICH_5V_CURRENT>().UpdateValue(res);

        res = buffer.GetValue(SensorData::Sensor::buddy5VCurrent);
        Item<MI_INFO_BUDDY_5V_CURRENT>().UpdateValue(res);

#endif
    }

    if (event == GUI_event_t::HELD_RELEASED) {
        DialogMoveZ::Show();
        return;
    }

    SuperWindowEvent(sender, event, param);
}

ScreenMenuSensorInfo::ScreenMenuSensorInfo()
    : ScreenMenuSensorInfo__(_(label)) {
    EnableLongHoldScreenAction();
    ClrMenuTimeoutClose();
}
