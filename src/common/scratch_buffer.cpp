// scratch_buffer.cpp
#include "scratch_buffer.hpp"

uint8_t scratch_buffer[SCRATCH_BUFFER_SIZE] __attribute__((section(".ccmram")));
