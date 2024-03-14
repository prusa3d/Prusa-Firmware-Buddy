// HardwareSerial.cpp - Buddy/STM32
#include <device/board.h>

#if (BOARD_IS_BUDDY)
    #include <Arduino.h>
    #include "buffered_serial.hpp"
    #include "cmsis_os.h"
    #include "bsod.h"

using namespace buddy::hw;

HardwareSerial::HardwareSerial([[maybe_unused]] void *peripheral) {
}

void HardwareSerial::begin([[maybe_unused]] unsigned long baud) {
    BufferedSerial::uart2.Open();
}

void HardwareSerial::begin([[maybe_unused]] unsigned long baud, [[maybe_unused]] byte config) {
}

void HardwareSerial::close() {
    BufferedSerial::uart2.Close();
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
    int read = BufferedSerial::uart2.Read(&ch, 1);
    return read ? ch : -1;
}

void HardwareSerial::flush() {
    BufferedSerial::uart2.Flush();
}

size_t HardwareSerial::write(const uint8_t c) {
    return BufferedSerial::uart2.Write((const char *)&c, 1);
}

size_t HardwareSerial::write(const uint8_t *buffer, size_t size) {
    return BufferedSerial::uart2.Write((const char *)buffer, size);
}

HardwareSerial::operator bool() {
    return true;
}

HardwareSerial Serial3(USART3);
#endif
