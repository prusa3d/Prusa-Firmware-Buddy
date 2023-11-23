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
    : AddSuperWindow<FooterIconText_IntVal>(parent, &img::filament_sensor_17x16, static_makeView, static_readValue) {
}

FooterItemFSensorSide::FooterItemFSensorSide(window_t *parent)
    : AddSuperWindow<FooterIconText_IntVal>(parent, &img::side_filament_sensor_17x16, static_makeView, static_readValue) {
}

int FooterItemFSensor::static_readValue() {
    if (IFSensor *sensor = get_active_printer_sensor(); sensor) {
        return static_cast<int>(sensor->Get());
    }
    return no_tool_value;
}

int FooterItemFSensorSide::static_readValue() {
    if (IFSensor *sensor = get_active_side_sensor(); sensor) {
        return static_cast<int>(sensor->Get());
    }
    return no_tool_value;
}

// TODO FIXME last character is not shown, I do not know why, added space as workaround
string_view_utf8 FooterItemFSensor::static_makeView(int value) {
    // Show --- if no tool is picked
    if (value == no_tool_value) {
        return string_view_utf8::MakeCPUFLASH(reinterpret_cast<const uint8_t *>(no_tool_str));
    }

    fsensor_t state = fsensor_t(value);
    const char *txt = N_("N/A ");

    switch (state) {
    case fsensor_t::HasFilament:
        txt = N_("ON ");
        break;
    case fsensor_t::NoFilament:
        txt = N_("OFF ");
        break;
    case fsensor_t::Disabled:
        txt = N_("DIS ");
        break;
#ifdef _DEBUG
    case fsensor_t::NotInitialized:
        txt = N_("NINIT ");
        break;
    case fsensor_t::NotCalibrated:
        txt = N_("NCAL ");
        break;
    case fsensor_t::NotConnected:
        txt = N_("NC ");
        break;
#else //! DEBUG
    default:
        break;
#endif //_DEBUG
    }

    return string_view_utf8(_(txt));
}
