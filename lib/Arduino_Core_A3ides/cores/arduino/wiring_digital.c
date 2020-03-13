//wiring_digital.c - A3ides/STM32

#include "wiring_digital.h"
#include "../../src/common/hwio.h"

void digitalWrite(uint32_t ulPin, uint32_t ulVal) {
    hwio_arduino_digitalWrite(ulPin, ulVal);
}

int digitalRead(uint32_t ulPin) {
    return hwio_arduino_digitalRead(ulPin);
}

void digitalToggle(uint32_t ulPin) {
    hwio_arduino_digitalToggle(ulPin);
}

void pinMode(uint32_t ulPin, uint32_t ulMode) {
    hwio_arduino_pinMode(ulPin, ulMode);
}
