/**
 * @file
 * @date Oct 1, 2020
 * @author Marek Bel
 */

#pragma once

#include "Pin.hpp"

/**
 * @name Use these macros only for pins used from Marlin.
 *
 * @{
 */
#define MARLIN_PIN_NR_0  0
#define MARLIN_PIN_NR_1  1
#define MARLIN_PIN_NR_2  2
#define MARLIN_PIN_NR_3  3
#define MARLIN_PIN_NR_4  4
#define MARLIN_PIN_NR_5  5
#define MARLIN_PIN_NR_6  6
#define MARLIN_PIN_NR_7  7
#define MARLIN_PIN_NR_8  8
#define MARLIN_PIN_NR_9  9
#define MARLIN_PIN_NR_10 10
#define MARLIN_PIN_NR_11 11
#define MARLIN_PIN_NR_12 12
#define MARLIN_PIN_NR_13 13
#define MARLIN_PIN_NR_14 14
#define MARLIN_PIN_NR_15 15

#define MARLIN_PORT_A 0
#define MARLIN_PORT_B 1
#define MARLIN_PORT_C 2
#define MARLIN_PORT_D 3
#define MARLIN_PORT_E 4
#define MARLIN_PORT_F 5
/**
 * @brief Virtual port not associated with any physical pin
 *
 * Mapping of virtual pin needs to be implemented in hwio_buddy_2209_02.cpp
 *
 */
#define MARLIN_PORT_V 7

#define MARLIN_PORT_PIN(port, pin) ((16 * (port)) + (pin))

#define MARLIN_PIN(name) MARLIN_PORT_PIN(MARLIN_PORT_##name, MARLIN_PIN_NR_##name)
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
