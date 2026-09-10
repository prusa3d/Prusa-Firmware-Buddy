#pragma once

#include <cstdint>
#include <ldc1612.hpp>
#include <freertos/binary_semaphore.hpp>

namespace hal {

/// Enable CAN bit rate switch?
static constexpr const bool enable_bit_rate_switch = false;

void init();
void set_status_led(bool set);
void ldc1612_set_enabled(bool enabled);
bool ldc1612_is_enabled();

void __attribute__((noreturn)) panic();
void __attribute__((noreturn)) reset();

extern LDC1612 ldc1612;
extern freertos::BinarySemaphore ldc_data_ready;

} // namespace hal
