/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2020 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "../core/macros.h"
#include "i2c.hpp"
#include <Wire.h>

#ifndef I2C_ADDRESS
  #define I2C_ADDRESS(A) uint8_t(A)
#endif

// Print debug messages with M111 S2 (Uses 236 bytes of PROGMEM)
//#define DEBUG_TWIBUS

typedef void (*twiReceiveFunc_t)(int bytes);
typedef void (*twiRequestFunc_t)();

#define TWIBUS_BUFFER_SIZE 256

/**
 * TWIBUS class
 *
 * This class implements a wrapper around the two wire (I2C) bus, allowing
 * Marlin to send and request data from any slave device on the bus.
 *
 * The two main consumers of this class are M260 and M261. M260 provides a way
 * to send an I2C packet to a device (no repeated starts) by caching up to 256
 * bytes in a buffer and then sending the buffer.
 * M261 requests data from a device. The received data is relayed to serial out
 * for the host to interpret.
 *
 *  For more information see
 *    - https://marlinfw.org/docs/gcode/M260.html
 *    - https://marlinfw.org/docs/gcode/M261.html
 */
class TWIBus {
  private:
    /**
     * @brief Number of bytes on buffer
     * @description Number of bytes in the buffer waiting to be flushed to the bus
     */
    uint8_t buffer_s = 0;

    /**
     * @brief Internal buffer
     * @details A fixed buffer. TWI commands can be no longer than this.
     */
    uint8_t buffer[TWIBUS_BUFFER_SIZE];

    /**
     * @brief Number of bytes available on read buffer
     * @description Number of bytes in the read buffer
     */
    uint8_t read_buffer_available = 0;

    /**
     * @brief Next position to be read from read buffer
     * @description Next position to be read from read buffer
     */
    uint8_t read_buffer_pos = 0;

    /**
     * @brief Internal read buffer
     * @details A fixed read buffer.
     */
    uint8_t read_buffer[TWIBUS_BUFFER_SIZE];

    bool read_buffer_has_byte();

    uint8_t read_buffer_read_byte();

    bool check_hal_response(i2c::Result response);

    bool isRestrictedAddress(const uint8_t addr);

  public:
    /**
     * @brief Target device address
     * @description The target device address. Persists until changed.
     */
    uint8_t addr = 0;

    /**
     * @brief Class constructor
     * @details Initialize the TWI bus and clear the buffer
     */
    TWIBus();

    /**
     * @brief Reset the buffer
     * @details Set the buffer to a known-empty state
     */
    void reset();

    /**
     * @brief Send the buffer data to the bus
     * @details Flush the buffer to the target address
     */
    void send();

    /**
     * @brief Add one byte to the buffer
     * @details Add a byte to the end of the buffer.
     *          Silently fails if the buffer is full.
     *
     * @param c a data byte
     */
    void addbyte(const char c);

    /**
     * @brief Add some bytes to the buffer
     * @details Add bytes to the end of the buffer.
     *          Concatenates at the buffer size.
     *
     * @param src source data address
     * @param bytes the number of bytes to add
     */
    void addbytes(char src[], uint8_t bytes);

    /**
     * @brief Add a null-terminated string to the buffer
     * @details Add bytes to the end of the buffer up to a nul.
     *          Concatenates at the buffer size.
     *
     * @param str source string address
     */
    void addstring(char str[]);

    /**
     * @brief Set the target slave address (if address is not restricted)
     * @details The target slave address for sending the full packet
     *
     * @param adr 7-bit integer address
     * @return status of the request: true=success, false=fail
     */
    bool address(const uint8_t adr);

    /**
     * @brief Echo data on the bus to serial
     * @details Echo some number of bytes from the bus
     *          to serial in a parser-friendly format.
     *
     * @param bytes the number of bytes to request
     * @param style Output format for the bytes, 0 = Raw byte [default], 1 = Hex characters, 2 = uint16_t
     */
    void echodata(uint8_t bytes, FSTR_P const prefix, uint8_t adr, const uint8_t style=0);

    /**
     * @brief Request data from the slave device and wait.
     * @details Request a number of bytes from a slave device.
     *          Wait for the data to arrive, and return true
     *          on success.
     *
     * @param bytes the number of bytes to request
     * @return status of the request: true=success, false=fail
     */
    bool request(const uint8_t bytes);

    /**
     * @brief Capture data from the bus into the buffer.
     * @details Capture data after a request has succeeded.
     *
     * @param bytes the number of bytes to request
     * @return the number of bytes captured to the buffer
     */
    uint8_t capture(char *dst, const uint8_t bytes);

    /**
     * @brief Flush the i2c bus.
     * @details Get all bytes on the bus and throw them away.
     */
    void flush();

    /**
     * @brief Request data from the slave device, echo to serial.
     * @details Request a number of bytes from a slave device and output
     *          the returned data to serial in a parser-friendly format.
     * @style Output format for the bytes, 0 = raw byte [default], 1 = Hex characters, 2 = uint16_t
     *
     * @param bytes the number of bytes to request
     */
    void relay(const uint8_t bytes, const uint8_t style=0);

    #if ENABLED(DEBUG_TWIBUS)
      /**
       * @brief Prints a debug message
       * @details Prints a simple debug message "TWIBus::function: value"
       */
      static void prefix(FSTR_P const func);
      static void debug(FSTR_P const func, uint32_t adr);
      static void debug(FSTR_P const func, char c);
      static void debug(FSTR_P const func, char adr[]);
    #else
      static void debug(FSTR_P const, uint32_t) {}
      static void debug(FSTR_P const, char) {}
      static void debug(FSTR_P const, char[]) {}
    #endif
    static void debug(FSTR_P const func, uint8_t v) { debug(func, (uint32_t)v); }
};

extern TWIBus twibus;
