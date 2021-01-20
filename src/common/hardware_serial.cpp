//HardwareSerial.cpp - A3ides/STM32
#include <Arduino.h>
#include "buffered_serial.hpp"
#include "HardwareSerial.h"
#include "cmsis_os.h"
#include "bsod.h"

using namespace buddy::hw;

HardwareSerial::HardwareSerial(void *peripheral) {
}

void HardwareSerial::begin(unsigned long baud) {
    BufferedSerial::uart2.Open();
}

void HardwareSerial::begin(unsigned long baud, byte config) {
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

#ifdef USE_UART1_SERIAL

extern "C" {
extern UART_HandleTypeDef huart1;
}

void HardwareSerial2::begin(unsigned long baud) {
    // Do nothing, the MX_UART_Init() function takes care of this.
}

int HardwareSerial2::read(void) {
    if (huart1.Instance->SR & UART_FLAG_RXNE)
        return huart1.Instance->DR;
    else
        return -1;
}

int HardwareSerial2::available() {
    return (huart1.Instance->SR & UART_FLAG_RXNE) > 0;
}

void HardwareSerial2::flush() {
    // Nothing to do here, the TX is blocking.
}

size_t HardwareSerial2::write(uint8_t c) {
    if (HAL_UART_Transmit(&huart1, &c, 1, HAL_MAX_DELAY) == HAL_OK) {
        return 1;
    } else {
        return 0;
    }
}

size_t HardwareSerial2::write(uint8_t *buffer, size_t size) {
    if (HAL_UART_Transmit(&huart1, buffer, size, HAL_MAX_DELAY) == HAL_OK) {
        return size;
    } else {
        return 0;
    }
}

HardwareSerial2 SerialUART1(USART1);

#endif // SERIAL_PORT == 1

HardwareSerial Serial3(USART3);
