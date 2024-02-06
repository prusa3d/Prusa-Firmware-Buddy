#pragma once

#include "puppies/fifo_coder.hpp"
#include <stdint.h>

namespace dwarf::accelerometer {

void set_enable(bool enabled);
bool accelerometer_get_sample(AccelerometerRecord &sample);
size_t get_num_samples();
float measured_sampling_rate();
void irq();
} // namespace dwarf::accelerometer
