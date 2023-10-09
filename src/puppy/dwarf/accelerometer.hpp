#pragma once

#include "puppies/fifo_coder.hpp"
#include <stdint.h>

namespace dwarf::accelerometer {

void set_enable(bool enabled);
bool is_enabled();
void set_high_sample_rate(bool high);
bool is_high_sample_rate();
void accelerometer_loop();
void accelerometer_irq();
bool accelerometer_get_sample(AccelerometerRecord &sample);
size_t get_num_samples();
void irq();
} // namespace dwarf::accelerometer
