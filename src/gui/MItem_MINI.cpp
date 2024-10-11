/**
 * @file MItem_MINI.cpp
 */

#include "MItem_MINI.hpp"
#include "img_resources.hpp"
#include "filament_sensors_handler.hpp"
#include "filament_sensor.hpp"
#include "str_utils.hpp"
#include "fonts.hpp"

MI_MINDA::MI_MINDA()
    : MenuItemAutoUpdatingLabel(_("M.I.N.D.A."), "%i", [] {
        return buddy::hw::zMin.read() == buddy::hw::Pin::State::high;
    }) //
{}
