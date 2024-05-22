#pragma once
#include <stdint.h>
#include <stddef.h>
#include "device/hal.h"

//***********************
//*** RS485 configuration
// TODO: Implement DMA reading and decrease this priority. Currently it has to be high so that

inline constexpr size_t RS485_BUFFER_SIZE = 256; // Modbus specification needs 256 Bytes

inline constexpr uint32_t RS485_BAUDRATE = 230400;
inline constexpr uint32_t RS485_STOP_BITS = UART_STOPBITS_1;
inline constexpr uint32_t RS485_PARITY = UART_PARITY_NONE;

inline constexpr uint32_t RS485_ASSERTION_TIME = 16;
inline constexpr uint32_t RS485_DEASSERTION_TIME = 16;

// Modbus specification requires at least 3.5 char delay between frames, one character uses 11 bits
inline constexpr uint32_t RS485_RX_TIMEOUT_BITS = 33;

#define RS485_DE_SIGNAL_GPIO_PORT GPIOA
#define RS485_DE_SIGNAL_GPIO_PIN  GPIO_PIN_12

//***********************
//*** Modbus configuration
inline constexpr int32_t MODBUS_WATCHDOG_TIMEOUT = 30000;
inline constexpr auto MODBUS_FIFO_MAX_COUNT = 31; // Same value as MODBUS_FIFO_LEN defined in fifo_coder.hpp, maximal value allowed by Modbus specification is 31

//*******************************
//*** Safety checks configuration

static constexpr bool ENABLE_HEATER_CURRENT_CHECKS = true;
static constexpr float MAX_HEATER_CURRENT_A = 2.0f;
