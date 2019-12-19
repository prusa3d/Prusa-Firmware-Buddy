//wiring_analog.c - A3ides/STM32

#include "wiring_analog.h"

extern uint32_t hwio_arduino_analogRead(uint32_t ulPin);
extern void hwio_arduino_analogWrite(uint32_t ulPin, uint32_t ulValue);

uint32_t analogRead(uint32_t ulPin) {
    return hwio_arduino_analogRead(ulPin);
}

void analogWrite(uint32_t ulPin, uint32_t ulValue) {
    hwio_arduino_analogWrite(ulPin, ulValue);
}
