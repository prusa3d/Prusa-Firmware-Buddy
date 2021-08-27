#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

/// Ticks with second precision
///
/// Overflows every 136 years
uint32_t ticks_s();

/// Ticks with millisecond precision
///
/// Overflows every 49 days
uint32_t ticks_ms();

/// Ticks with microsecond precision
///
/// Overflows every 71 minutes
uint32_t ticks_us();

/// Ticks with nanosecond precision
///
/// Overflows every 4.29 seconds
uint32_t ticks_ns();

/// Return difference ticks_b - ticks_a while handling overflows
///
/// Helper function to properly handle overflows when using the
/// ticks_xx() functions.
///
/// Example:
///
///     scheduled_ticks_us = ...
///     if (ticks_diff(scheduled_ticks_us, ticks_us()) > 0)
///         // too early
///     else if (ticks_diff(scheduled_ticks_us, ticks_us()) == 0)
///         // right at time
///     elif (ticks_diff(scheduled_ticks_us, ticks_us()) < 0)
///         // too late
///
int32_t ticks_diff(uint32_t ticks_a, uint32_t ticks_b);

/// Time since the start of the system in nanoseconds
///
/// Given the datatype, overflows every ~584 years
uint64_t timestamp_ns();

/// Sys timer's overflow interrupt callback
///
void app_tick_timer_overflow();

#ifdef __cplusplus
}
#endif //__cplusplus
