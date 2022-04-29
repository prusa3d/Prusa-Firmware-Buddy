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

typedef uint32_t (*time_fn_t)();

inline void delay__(time_fn_t time_fn, uint32_t delay) {
    const uint32_t time_to_wait = delay + 1; // fixes issue with maxint, now i have tu use <= instead <
    uint32_t start = time_fn();
    while ((time_fn() - start) <= time_to_wait)
        ;
}

/// @brief Delay nanoseconds
///
/// Relies on timing system has been already initialized by tick_timer_init().
/// It is not guaranteed to return otherwise.
///
/// See DELAY_NS_PRECISE if you need guaranteed return in uninitialized
/// state or you need more precise timing and can afford disabling
/// interrupts or you are already in disabled interrupts context.
///
/// @param ns time in nanoseconds
/// This function is not guaranteed to return if ns is close
/// to uint32_t MAX
inline void delay_ns(uint32_t ns) {
    delay__(ticks_ns, ns);
}

/// @brief Delay microseconds
///
/// Relies on timing system has been already initialized by tick_timer_init()
/// and tick is not suspended by HAL_SuspendTick() and interrupts are enabled.
/// It is not guaranteed to return otherwise.
///
/// See delay_us_precise() if you need guaranteed return in uninitialized
/// state or you need more precise timing and can afford disabling
/// interrupts or you are already in disabled interrupts context.
///
/// @param us time in microseconds
/// This function is not guaranteed to return if ms is very close
/// to uint32_t MAX
inline void delay_us(uint32_t us) {
    delay__(ticks_us, us);
}

/// @brief Delay milliseconds
///
/// Relies on timing system has been already initialized by tick_timer_init()
/// and tick is not suspended by HAL_SuspendTick() and interrupts are enabled.
/// It is not guaranteed to return otherwise.
///
/// @param ms time in milliseconds
/// @todo release CPU if called in task context

inline void delay_ms(uint32_t ms) {
    delay__(ticks_ms, ms);
}

/// @brief Delay seconds
///
/// Relies on timing system has been already initialized by tick_timer_init()
/// and tick is not suspended by HAL_SuspendTick() and interrupts are enabled.
/// It is not guaranteed to return otherwise.
///
/// @param s time in seconds
/// @todo release CPU if called in task context
inline void delay_s(uint32_t s) {
    delay__(ticks_s, s);
}

#ifdef __cplusplus
}
#endif //__cplusplus
