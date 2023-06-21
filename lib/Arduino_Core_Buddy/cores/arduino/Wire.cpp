// Wire.cpp - Buddy/STM32

#include "Wire.h"
#include "cmsis_os.h"

uint16_t i2c_dev_address = 0;

TwoWire::TwoWire(void) {
}

void TwoWire::begin(void) {
}

void TwoWire::beginTransmission(uint8_t address) {
    i2c_dev_address = address << 1;
}

uint8_t TwoWire::endTransmission(void) {
    return 0;
}

uint8_t TwoWire::requestFrom(uint8_t address, uint8_t quantity) {
    return 0;
}

extern "C" {
extern void lcdsim_expander_write(uint8_t data);
}

size_t TwoWire::write(uint8_t data) {
    // HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(&hi2c1, i2c_dev_address, &data, 1, 100);
    return 1;
}

size_t TwoWire::write(const uint8_t *data, size_t quantity) {
    return 0;
}

int TwoWire::available(void) {
    return 0;
}

int TwoWire::read(void) {
    return 0;
}

int TwoWire::peek(void) {
    return 0;
}

void TwoWire::flush(void) {
}

TwoWire Wire = TwoWire();
