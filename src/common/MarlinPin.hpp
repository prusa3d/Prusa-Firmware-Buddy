/**
 * @file
 * @date Oct 1, 2020
 * @author Marek Bel
 */

#pragma once

#include "Pin.hpp"
#include "MarlinPin.h"

/**
 * @name Use these macros only for pins used from Marlin.
 *
 * @{
 */

/**
 * @brief Virtual port not associated with any physical pin
 *
 * Mapping of virtual pin needs to be implemented in hwio_buddy_2209_02.cpp
 *
 */
#define MARLIN_PORT_V 7
/**
 * @brief Convert Marlin style defined pin to be used in constructor of Pin
 */
#define BUDDY_PIN(name) static_cast<buddy::hw::IoPort>(MARLIN_PORT_##name), static_cast<buddy::hw::IoPin>(MARLIN_PIN_NR_##name)
/**@}*/

static_assert(buddy::hw::IoPort::A == static_cast<buddy::hw::IoPort>(MARLIN_PORT_A), "Marlin port doesn't match Buddy IoPort.");
static_assert(buddy::hw::IoPort::B == static_cast<buddy::hw::IoPort>(MARLIN_PORT_B), "Marlin port doesn't match Buddy IoPort.");
static_assert(buddy::hw::IoPort::F == static_cast<buddy::hw::IoPort>(MARLIN_PORT_F), "Marlin port doesn't match Buddy IoPort.");
static_assert(buddy::hw::IoPin::p0 == static_cast<buddy::hw::IoPin>(MARLIN_PIN_NR_0), "Marlin pin doesn't match Buddy IoPin.");
static_assert(buddy::hw::IoPin::p1 == static_cast<buddy::hw::IoPin>(MARLIN_PIN_NR_1), "Marlin pin doesn't match Buddy IoPin.");
static_assert(buddy::hw::IoPin::p15 == static_cast<buddy::hw::IoPin>(MARLIN_PIN_NR_15), "Marlin pin doesn't match Buddy IoPin.");

namespace buddy::hw {
bool isOutputPin(uint32_t marlinPin);
}
