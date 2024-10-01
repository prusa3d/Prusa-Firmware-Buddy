// HardwareSerial.cpp - Buddy/STM32
#include <option/has_tmc_uart.h>

#if HAS_TMC_UART()
    #include <Arduino.h>
    #include "buffered_serial.hpp"
    #include "cmsis_os.h"
    #include "bsod.h"

extern buddy::hw::BufferedSerial uart_for_tmc;

HardwareSerial::HardwareSerial([[maybe_unused]] void *peripheral) {
}

void HardwareSerial::begin([[maybe_unused]] unsigned long baud) {
    uart_for_tmc.Open();
}

void HardwareSerial::begin([[maybe_unused]] unsigned long baud, [[maybe_unused]] byte config) {
}

void HardwareSerial::close() {
    uart_for_tmc.Close();
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
    int read = uart_for_tmc.Read(&ch, 1);
    return read ? ch : -1;
}

void HardwareSerial::flush() {
    uart_for_tmc.Flush();
}

size_t HardwareSerial::write(const uint8_t c) {
    return uart_for_tmc.Write((const char *)&c, 1);
}

size_t HardwareSerial::write(const uint8_t *buffer, size_t size) {
    return uart_for_tmc.Write((const char *)buffer, size);
}

HardwareSerial::operator bool() {
    return true;
}

HardwareSerial Serial3(USART3);
#endif
