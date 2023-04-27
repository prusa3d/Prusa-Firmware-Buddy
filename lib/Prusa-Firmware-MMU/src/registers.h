/// @file
#pragma once
#include <stdint.h>

bool ReadRegister(uint8_t address, uint16_t &value);
bool WriteRegister(uint8_t address, uint16_t value);
