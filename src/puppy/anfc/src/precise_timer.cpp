#include "timing.h"

#include <stm32c0xx_hal.h>

namespace hal::peripherals {
extern TIM_HandleTypeDef htim2;
}

namespace precise_timing {

template <uint64_t time_scale>
uint64_t Timer::calc_units(uint64_t secs) const {
    static constexpr uint64_t units_scale = (precise_timing::counter_period / time_scale);
    return secs * time_scale + (__HAL_TIM_GET_COUNTER(&hal::peripherals::htim2) / units_scale);
}

template <uint64_t time_scale>
uint64_t Timer::calc_timestamp_with_overflow_check() const {
    const auto secs = static_cast<uint64_t>(seconds.load());
    uint64_t res = calc_units<time_scale>(secs);
    const auto new_secs = seconds.load();
    if (secs != new_secs) [[unlikely]] {
        // Oops the timer triggered while reading the value so we need to read the values again.
        res = calc_units<time_scale>(new_secs);
    }
    return res;
}

uint32_t Timer::get_timestamp_s() const {
    return seconds.load();
}

uint64_t Timer::get_timestamp_ms() const {
    return calc_timestamp_with_overflow_check<1000>();
}

uint64_t Timer::get_timestamp_us() const {
    return calc_timestamp_with_overflow_check<1'000'000>();
}

uint64_t Timer::get_timestamp_ns() const {
    static constexpr uint64_t step = 1000 / (precise_timing::counter_period / 1'000'000);
    const auto secs = seconds.load();
    auto res = static_cast<uint64_t>(secs) * 1'000'000'000 + (__HAL_TIM_GET_COUNTER(&hal::peripherals::htim2) * step);
    const auto new_secs = seconds.load();
    if (secs != new_secs) [[unlikely]] {
        res = static_cast<uint64_t>(new_secs) * 1'000'000'000 + (__HAL_TIM_GET_COUNTER(&hal::peripherals::htim2) * step);
    }
    return res;
}

/// IRQ callback - when timer overflows
void Timer::increment_seconds() {
    seconds.fetch_add(1);
}

} // namespace precise_timing

precise_timing::Timer precise_timer {};

extern "C" timestamp_t get_timestamp() {
    const uint64_t us = precise_timer.get_timestamp_us();
    return {
        .sec = static_cast<uint32_t>(us / 1'000'000),
        .us = static_cast<uint32_t>(us % 1'000'000),
    };
}

extern "C" void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim == &hal::peripherals::htim2) {
        precise_timer.increment_seconds();
        return;
    }
}
