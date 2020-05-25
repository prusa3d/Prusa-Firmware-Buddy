/*
 *******************************************************************************
 * Copyright (c) 2017, STMicroelectronics
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of STMicroelectronics nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *******************************************************************************
 */

#ifndef _VARIANT_ARDUINO_STM32_
#define _VARIANT_ARDUINO_STM32_

/*----------------------------------------------------------------------------
 *        Headers
 *----------------------------------------------------------------------------*/
//#include "PeripheralPins.h"
#include "stm32_def.h"
#include <PinNames.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/*----------------------------------------------------------------------------
 *        Pins
 *----------------------------------------------------------------------------*/
extern const PinName digitalPin[];

// This must be a literal
#define NUM_DIGITAL_PINS 94
// This must be a literal with a value less than or equal to MAX_ANALOG_INPUTS
#define NUM_ANALOG_INPUTS 14
#define NUM_ANALOG_FIRST  80

// Below ADC, DAC and PWM definitions already done in the core
// Could be redefined here if needed
// ADC resolution is 12bits
//#define ADC_RESOLUTION          12
//#define DACC_RESOLUTION         12

// PWM resolution
//#define PWM_RESOLUTION          8
//#define PWM_FREQUENCY           1000
//#define PWM_MAX_DUTY_CYCLE      255

// On-board LED pin number
#define LED_BUILTIN TPC13

// On-board user button
//#define BTN_K_UP                PA0   - no user button

// Below SPI and I2C definitions already done in the core
// Could be redefined here if differs from the default one
// SPI Definitions

//TODO remove following temporary definitions
#define TPA0  0
#define TPA1  1
#define TPA2  2
#define TPA3  3
#define TPA4  4
#define TPA5  5
#define TPA6  6
#define TPA7  7
#define TPA8  8
#define TPA9  9
#define TPA10 10
#define TPA11 11
#define TPA12 12
#define TPA13 13
#define TPA14 14
#define TPA15 15
#define TPB0  16
#define TPB1  17
#define TPB2  18
#define TPB3  19
#define TPB4  20
#define TPB5  21
#define TPB6  22
#define TPB7  23
#define TPB8  24
#define TPB9  25
#define TPB10 26
#define TPB11 27
#define TPB12 28
#define TPB13 29
#define TPB14 30
#define TPB15 31
#define TPC0  32
#define TPC1  33
#define TPC2  34
#define TPC3  35
#define TPC4  36
#define TPC5  37
#define TPC6  38
#define TPC7  39
#define TPC8  40
#define TPC9  41
#define TPC10 42
#define TPC11 43
#define TPC12 44
#define TPC13 45
#define TPC14 46
#define TPC15 47
#define TPD0  48
#define TPD1  49
#define TPD2  50
#define TPD3  51
#define TPD4  52
#define TPD5  53
#define TPD6  54
#define TPD7  55
#define TPD8  56
#define TPD9  57
#define TPD10 58
#define TPD11 59
#define TPD12 60
#define TPD13 61
#define TPD14 62
#define TPD15 63
#define TPE0  64
#define TPE1  65
#define TPE2  66
#define TPE3  67
#define TPE4  68
#define TPE5  69
#define TPE6  70
#define TPE7  71
#define TPE8  72
#define TPE9  73
#define TPE10 74
#define TPE11 75
#define TPE12 76
#define TPE13 77
#define TPE14 78
#define TPE15 79

#define PIN_SPI_MOSI TPB15
#define PIN_SPI_MISO TPB14
#define PIN_SPI_SCK  TPB13
#define PIN_SPI_SS   TPB12

// I2C Definitions
#define PIN_WIRE_SDA TPB7
#define PIN_WIRE_SCL TPB6

// Timer Definitions
//Do not use timer used by PWM pins when possible. See PinMap_PWM in PeripheralPins.c
#define TIMER_TONE TIM7

// Do not use basic timer: OC is required
#define TIMER_SERVO TIM2 //TODO: advanced-control timers don't work

// UART Definitions
// Define here Serial instance number to map on Serial generic name
//#define SERIAL_UART_INSTANCE    1 //ex: 2 for Serial2 (USART2)
// DEBUG_UART could be redefined to print on another instance than 'Serial'
//#define DEBUG_UART              ((USART_TypeDef *) U(S)ARTX) // ex: USART3
// DEBUG_UART baudrate, default: 9600 if not defined
//#define DEBUG_UART_BAUDRATE     x
// DEBUG_UART Tx pin name, default: the first one found in PinMap_UART_TX for DEBUG_UART
//#define DEBUG_PINNAME_TX        PX_n // PinName used for TX

// Default pin used for 'Serial' instance (ex: ST-Link)
// Mandatory for Firmata
//#define PIN_SERIAL_RX           PC11
//#define PIN_SERIAL_TX           PC10

//#define PIN_SERIAL3_RX           PC11
//#define PIN_SERIAL3_TX           PC10
#define PIN_SERIAL3_RX PB11
#define PIN_SERIAL3_TX PB10

#ifdef __cplusplus
} // extern "C"
#endif
/*----------------------------------------------------------------------------
 *        Arduino objects - C++ only
 *----------------------------------------------------------------------------*/

#ifdef __cplusplus

    // These serial port names are intended to allow libraries and architecture-neutral
    // sketches to automatically default to the correct port name for a particular type
    // of use.  For example, a GPS module would normally connect to SERIAL_PORT_HARDWARE_OPEN,
    // the first hardware serial port whose RX/TX pins are not dedicated to another use.
    //
    // SERIAL_PORT_MONITOR        Port which normally prints to the Arduino Serial Monitor
    //
    // SERIAL_PORT_USBVIRTUAL     Port which is USB virtual serial
    //
    // SERIAL_PORT_LINUXBRIDGE    Port which connects to a Linux system via Bridge library
    //
    // SERIAL_PORT_HARDWARE       Hardware serial port, physical RX & TX pins.
    //
    // SERIAL_PORT_HARDWARE_OPEN  Hardware serial ports which are open for use.  Their RX & TX
    //                            pins are NOT connected to anything by default.
    //#define SERIAL_PORT_MONITOR     Serial
    #define SERIAL_PORT_HARDWARE Serial3
#endif

#endif /* _VARIANT_ARDUINO_STM32_ */
