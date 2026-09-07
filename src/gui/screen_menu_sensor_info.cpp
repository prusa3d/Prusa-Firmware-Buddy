/// @file
#include "screen_menu_sensor_info.hpp"

#include <img_resources.hpp>
#include <common/sensor_data.hpp>
#include <option/has_indx.h>

#if HAS_DWARF() || HAS_INDX()

MI_INFO_HEAD_PCB_TEMPERATURE::MI_INFO_HEAD_PCB_TEMPERATURE()
    : MenuItemAutoUpdatingLabel {
        _("Head PCB Temperature"),
        standard_print_format::temp_c,
        [](auto) { return SensorData::head_pcb_temperature(); },
    } {}

MI_INFO_HEAD_MCU_TEMPERATURE::MI_INFO_HEAD_MCU_TEMPERATURE()
    : MenuItemAutoUpdatingLabel {
        _("Head MCU Temperature"),
        standard_print_format::temp_c,
        [](auto) { return SensorData::head_mcu_temperature(); },
    } {}

#endif

#if HAS_INDX()

MI_INFO_HEAD_AMBIENT_TEMPERATURE::MI_INFO_HEAD_AMBIENT_TEMPERATURE()
    : MenuItemAutoUpdatingLabel {
        _("Head Ambient Temperature"),
        standard_print_format::temp_c,
        [](auto) { return SensorData::head_ambient_temperature(); },
    } {}

MI_INFO_NOZZLE_TEMP_UNCOMPENSATED::MI_INFO_NOZZLE_TEMP_UNCOMPENSATED()
    : MenuItemAutoUpdatingLabel {
        _("Nozzle Raw Temperature"),
        standard_print_format::temp_c,
        [](auto) { return SensorData::nozzle_temp_uncompensated(); },
    } {}

MI_INFO_NOZZLE_POWER::MI_INFO_NOZZLE_POWER()
    : MenuItemAutoUpdatingLabel {
        _("Nozzle Power"),
        "%.1f W",
        [](auto) { return sensor_data().nozzle_power_W(); },
    } {}

MI_INFO_RINGDOWN_DECAY::MI_INFO_RINGDOWN_DECAY()
    : MenuItemAutoUpdatingLabel {
        _("Ringdown Decay"),
        "%d",
        [](auto) { return SensorData::ringdown_decay(); },
    } {}

#endif

ScreenMenuSensorInfo::ScreenMenuSensorInfo()
    : ScreenMenuSensorInfo_ {
        _("SENSOR INFO"),
        &img::info_16x16,
    } {
    EnableLongHoldScreenAction();
    ClrMenuTimeoutClose();
}
