/**
 * @file filament_sensor_selftest_api.hpp
 * @brief getter for printer filament sensor, to be used in selftest only
 */

#pragma once

#include "filament_sensor.hpp"

FSensor &GetPrinterFSensor();
