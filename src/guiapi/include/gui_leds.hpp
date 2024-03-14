/**
 * @file gui_leds.hpp
 * @author Radek Vana
 * @brief Container to handle LEDs and remember their value
 * @date 2021-08-01
 */
#pragma once
#include <cstddef> //size_t
#include <cstdint>
#include <cmath>
#include <tuple>
#include "bsod.h"
#include "led_animations/led_types.h"

namespace leds {

// names D1 D21 and D10 are from eletric schema
enum class index { backlight,
    l0_D2,
    l1_D21,
    l2_D10,
    count_ };

/**
 * @brief count of LEDs
 */
inline constexpr size_t Count = size_t(index::count_);

/**
 * @brief Initializes leds
 */
void Init();

/**
 * @brief call periodically, writes LED values
 */
void TickLoop();

/**
 * @brief Set the Nth LED
 *
 * @param clr color
 * @param n index, index::count_ sets all leds
 */
void SetNth(Color clr, index n);

/**
 * @brief Set the Brightness of display
 *
 * @param percent Brightness in range 0 - 100
 */
void SetBrightness(unsigned percent);

/**
 * @brief Forces write on next call of tick loop
 * @param cnt How many leds to refresh
 */
void ForceRefresh(size_t cnt);

// TODO rename afte LED have a meaning (represent a function like print status)
inline void Set0th(Color clr) { SetNth(clr, index::l0_D2); }
inline void Set1st(Color clr) { SetNth(clr, index::l1_D21); }
inline void Set2nd(Color clr) { SetNth(clr, index::l2_D10); }

/**
 * @brief Called from power panic module to quickly turn off leds from AC fault task
 */
void enter_power_panic();

}; // namespace leds
