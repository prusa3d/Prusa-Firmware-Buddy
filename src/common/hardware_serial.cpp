// HardwareSerial.cpp - Buddy/STM32
#include <option/has_tmc_uart.h>

#if HAS_TMC_UART()
    #include <Arduino.h>
    #include "buffered_serial.hpp"
    #include "cmsis_os.h"
    #include "bsod.h"

extern buddy::hw::BufferedSerial uart2;

HardwareSerial::HardwareSerial([[maybe_unused]] void *peripheral) {
}

void HardwareSerial::begin([[maybe_unused]] unsigned long baud) {
    uart2.Open();
}

void HardwareSerial::begin([[maybe_unused]] unsigned long baud, [[maybe_unused]] byte config) {
}

void HardwareSerial::close() {
    uart2.Close();
}

int HardwareSerial::available(void) {
    bsod("HardwareSerial::available() not implemented");
    return 0;
}

int HardwareSerial::peek(void) {
    return -1;
}

int HardwareSerial::read(void) {
    char ch;
    int read = uart2.Read(&ch, 1);
    return read ? ch : -1;
}

void HardwareSerial::flush() {
    uart2.Flush();
}

size_t HardwareSerial::write(const uint8_t c) {
    return uart2.Write((const char *)&c, 1);
}

size_t HardwareSerial::write(const uint8_t *buffer, size_t size) {
    return uart2.Write((const char *)buffer, size);
}

HardwareSerial::operator bool() {
    return true;
}

HardwareSerial Serial3(USART3);
#endif
