//wiring_digital.c - A3ides/STM32

#include "wiring_digital.h"

extern int hwio_arduino_digitalRead(uint32_t ulPin);
extern void hwio_arduino_digitalWrite(uint32_t ulPin, uint32_t ulVal);
extern void hwio_arduino_digitalToggle(uint32_t ulPin);
extern void hwio_arduino_pinMode(uint32_t ulPin, uint32_t ulMode);

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
