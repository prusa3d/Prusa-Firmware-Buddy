/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2019 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "../HAL/HAL.h"
#include "../core/utility.h"
#include "safety_timer_stubbed.hpp"

/**
 * Set the interval after which the safety timer will expire
 * and reset the timer. If set to zero (default), the safety
 * timer is disabled.
 */
void safety_timer_set_interval(millis_t ms);

/**
 * Whether the safety timer is expired.
 */
bool safety_timer_is_expired();

/**
 * Reset the internal counter of the safety timer.
 */
void safety_timer_reset();
