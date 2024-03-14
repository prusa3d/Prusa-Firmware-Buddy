/**
 * @file tick_timer_api.h
 * @author Radek Vana
 * @brief api functions of tick timer
 * @date 2021-07-24
 */

#include <stdint.h>
#include <device/hal.h>

#pragma once
#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

/**
 * @brief tick timers initialization
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef tick_timer_init();

/// Sys timer's overflow interrupt callback
///
void app_tick_timer_overflow();

#ifdef __cplusplus
}
#endif //__cplusplus
