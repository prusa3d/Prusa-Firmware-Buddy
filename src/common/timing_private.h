/**
 * @file timing_private.h
 * @author Radek Vana
 * @brief functions internally used in timing, not meant to be included outside timing source files
 * @date 2021-07-23
 */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

/**
 * @brief converts TICK_TIMERs value to nano seconds
 *
 * @param cnt value to be converted
 * @return uint64_t
 */
uint64_t clock_to_ns(uint32_t counter);

/**
 * @brief converts value in miliseconds to TICK_TIMER ticks
 *
 * @param miliseconds
 * @return uint64_t
 */
uint64_t ms_to_clock(uint32_t ms);

#ifdef __cplusplus
}
#endif //__cplusplus
