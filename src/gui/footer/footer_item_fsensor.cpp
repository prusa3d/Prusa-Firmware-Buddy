/**
 * @file footer_item_fsensor.cpp
 */

#include "footer_item_fsensor.hpp"
#include "filament_sensors_handler.hpp"
#include "img_resources.hpp"
#include "i18n.h"
#include <algorithm>
#include <cmath>

FooterItemFSensor::FooterItemFSensor(window_t *parent)
    : FooterIconText_IntVal(parent, &img::filament_sensor_17x16, static_makeView, static_readValue) {
}

FooterItemFSensorSide::FooterItemFSensorSide(window_t *parent)
    : FooterIconText_IntVal(parent, &img::side_filament_sensor_17x16, static_makeView, static_readValue) {
}

int FooterItemFSensor::static_readValue() {
    if (IFSensor *sensor = FSensors_instance().sensor(LogicalFilamentSensor::extruder)) {
        return static_cast<int>(sensor->get_state());
    }
    return no_tool_value;
}

int FooterItemFSensorSide::static_readValue() {
    if (IFSensor *sensor = FSensors_instance().sensor(LogicalFilamentSensor::side)) {
        return static_cast<int>(sensor->get_state());
    }
    return no_tool_value;
}

// TODO FIXME last character is not shown, I do not know why, added space as workaround
string_view_utf8 FooterItemFSensor::static_makeView(int value) {
    // Show --- if no tool is picked
    if (value == no_tool_value) {
        return string_view_utf8::MakeCPUFLASH(reinterpret_cast<const uint8_t *>(no_tool_str));
    }

    FilamentSensorState state = FilamentSensorState(value);
    const char *txt = N_("N/A ");

    switch (state) {
    case FilamentSensorState::HasFilament:
        txt = N_("ON ");
        break;
    case FilamentSensorState::NoFilament:
        txt = N_("OFF ");
        break;
    case FilamentSensorState::Disabled:
        txt = N_("DIS ");
        break;
#ifdef _DEBUG
    case FilamentSensorState::NotInitialized:
        txt = N_("NINIT ");
        break;
    case FilamentSensorState::NotCalibrated:
        txt = N_("NCAL ");
        break;
    case FilamentSensorState::NotConnected:
        txt = N_("NC ");
        break;
#endif //_DEBUG
    default:
        break;
    }

    return string_view_utf8(_(txt));
}
