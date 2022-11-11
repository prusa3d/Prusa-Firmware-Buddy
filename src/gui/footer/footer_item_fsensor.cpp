/**
 * @file footer_item_fsensor.cpp
 */

#include "footer_item_fsensor.hpp"
#include "filament_sensor_api.hpp"
#include "png_resources.hpp"
#include "i18n.h"
#include <algorithm>
#include <cmath>

FooterItemFSensor::FooterItemFSensor(window_t *parent)
    : AddSuperWindow<FooterIconText_IntVal>(parent, &png::filament_sensor_17x16, static_makeView, static_readValue) {
}

int FooterItemFSensor::static_readValue() {
    return int(FSensors_instance().GetPrinter());
}

//TODO FIXME last character is not shown, I do not know why, added space as workaround
string_view_utf8 FooterItemFSensor::static_makeView(int value) {
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
#else  //!DEBUG
    default:
        break;
#endif //_DEBUG
    }

    return string_view_utf8(_(txt));
}

string_view_utf8 FooterItemFSensor::GetName() { return _("F. Sensor"); }
