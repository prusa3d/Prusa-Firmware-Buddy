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

#include "../inc/MarlinConfig.h"

#if ENABLED(EXPERIMENTAL_I2CBUS)

#include "twibus.h"
#include "hwio_pindef.h"
#include "i2c.hpp"
#include <option/has_i2c_expander.h>

static constexpr uint16_t i2c_timeout_ms = 100;

FORCE_INLINE char hex_nybble(const uint8_t n) {
  return (n & 0xF) + ((n & 0xF) < 10 ? '0' : 'A' - 10);
}

TWIBus twibus;

TWIBus::TWIBus() {
  reset();
}

void TWIBus::reset() {
  buffer_s = 0;
  buffer[0] = 0x00;
  read_buffer_available = 0;
  read_buffer_pos = 0;
}

bool TWIBus::read_buffer_has_byte() {
  return read_buffer_pos < read_buffer_available;
}

uint8_t TWIBus::read_buffer_read_byte() {
  if (!read_buffer_has_byte()) {
    return 0;
  }
  return read_buffer[read_buffer_pos++];
}

bool TWIBus::address(const uint8_t adr) {
  if (!WITHIN(adr, 8, 127)) {
    SERIAL_ECHO_MSG("Bad I2C address (8-127)");
    return false;
  }

  if (isRestrictedAddress(adr)) {
    SERIAL_ECHO_MSG("Restricted I2C address.");
    return false;
  }

  addr = adr;

  debug(F("address"), adr);
  return true;
}

bool TWIBus::isRestrictedAddress(uint8_t addr) {
  switch (addr) {
    case 0x53:
    case 0x57:
      // EEPROM
      return true;
    case 0x22:
    case 0x23:
      // USBC
      return true;
     case 0x18:
     case 0x19:
     case 0x1A:
     case 0x1B:
     case 0x1C:
     case 0x1D:
     case 0x1E:
     case 0x1F:
      // IO Extender
      return true;
  }

  return false;
}

void TWIBus::addbyte(const char c) {
  if (buffer_s >= COUNT(buffer)) return;
  buffer[buffer_s++] = c;
  debug(F("addbyte"), c);
}

void TWIBus::addbytes(char src[], uint8_t bytes) {
  debug(F("addbytes"), bytes);
  while (bytes--) addbyte(*src++);
}

void TWIBus::addstring(char str[]) {
  debug(F("addstring"), str);
  while (char c = *str++) addbyte(c);
}

void TWIBus::send() {
  debug(F("send"), addr);
  
  #if HAS_I2C_EXPANDER()
  if (addr == buddy::hw::io_expander2.fixed_addr) [[unlikely]] {
    // Only full byte can be written here
    buddy::hw::io_expander2.write(buffer[0]);
  } else [[likely]] 
  #endif // HAS_I2C_EXPANDER()
  {
    i2c::Result ret = i2c::Transmit(I2C_HANDLE_FOR(gcode), addr << 1, buffer, buffer_s, i2c_timeout_ms);
    check_hal_response(ret);
  }

  reset();
}

bool TWIBus::check_hal_response(i2c::Result response) {
  if (response == i2c::Result::ok) {
    return true;
  }

  switch (response) {
  case i2c::Result::error:
    SERIAL_ERROR_MSG("TWIBus::send failed with: ERROR");
    break;
  case i2c::Result::busy_after_retries:
    SERIAL_ERROR_MSG("TWIBus::send failed with: BUSY");
    break;
  case i2c::Result::timeout:
     SERIAL_ERROR_MSG("TWIBus::send failed with: TIMEOUT");
    break;
  default:
    SERIAL_ERROR_MSG("TWIBus::send failed with: UNKNOWN");
  }
  return false;
}

void TWIBus::echodata(uint8_t bytes, FSTR_P const pref, uint8_t adr, const uint8_t style/*=0*/) {
  union TwoBytesToInt16 { uint8_t bytes[2]; int16_t integervalue; };
  TwoBytesToInt16 ConversionUnion;

  while (bytes-- && read_buffer_has_byte()) {
    int value = read_buffer_read_byte();
    switch (style) {

      // Style 1, HEX DUMP
      case 1:
        SERIAL_CHAR(hex_nybble((value & 0xF0) >> 4));
        SERIAL_CHAR(hex_nybble(value & 0x0F));
        if (bytes) SERIAL_CHAR(' ');
        break;

      // Style 2, signed two byte integer (int16)
      case 2:
        if (bytes == 1)
          ConversionUnion.bytes[1] = (uint8_t)value;
        else if (bytes == 0) {
          ConversionUnion.bytes[0] = (uint8_t)value;
          // Output value in base 10 (standard decimal)
          SERIAL_ECHO(ConversionUnion.integervalue);
        }
        break;

      // Style 3, unsigned byte, base 10 (uint8)
      case 3:
        SERIAL_ECHO(value);
        if (bytes) SERIAL_CHAR(' ');
        break;

      // Default style (zero), raw serial output
      default:
        // This can cause issues with some serial consoles, Pronterface is an example where things go wrong
        SERIAL_CHAR(value);
        break;
    }
  }

  SERIAL_EOL();
}

bool TWIBus::request(const uint8_t bytes) {
  if (!addr) return false;

  debug(F("request"), bytes);

  if (bytes > TWIBUS_BUFFER_SIZE) {
    SERIAL_ERROR_MSG("TWIBus::request Tried to read more than max buffer size.");

    return false;
  }

  flush();

#if HAS_I2C_EXPANDER()  
  if (addr == buddy::hw::io_expander2.fixed_addr) [[unlikely]] {
    const auto byte = buddy::hw::io_expander2.read();
    if (!byte.has_value()) {
      SERIAL_ECHO_MSG("I/O Expander: Read failure");
      return false;
    }
    
    read_buffer[0] = byte.value();
    for (uint8_t i = 1; i < bytes; i++) {
      read_buffer[i] = 0;
    }
  } else [[likely]]
#endif // HAS_I2C_EXPANDER()
  {
    i2c::Result ret = i2c::Receive(I2C_HANDLE_FOR(gcode), addr << 1 | 0x1, read_buffer, bytes, i2c_timeout_ms);

    if (!check_hal_response(ret)) {
      return false;
    }
  }

  read_buffer_available = bytes;
  return true;
}

void TWIBus::relay(const uint8_t bytes, const uint8_t style/*=0*/) {
  debug(F("relay"), bytes);

  if (request(bytes))
    echodata(bytes, F("i2c-reply"), addr, style);
}

uint8_t TWIBus::capture(char *dst, const uint8_t bytes) {
  reset();
  uint8_t count = 0;
  while (count < bytes && read_buffer_has_byte())
    dst[count++] = read_buffer_read_byte();

  debug(F("capture"), count);

  return count;
}

void TWIBus::flush() {
  read_buffer_available = 0;
  read_buffer_pos = 0;
}

#if ENABLED(DEBUG_TWIBUS)

  // static
  void TWIBus::prefix(FSTR_P const func) {
    SERIAL_ECHOPGM("TWIBus::", func, ": ");
  }
  void TWIBus::debug(FSTR_P const func, uint32_t adr) {
    if (DEBUGGING(INFO)) { prefix(func); SERIAL_ECHOLN(adr); }
  }
  void TWIBus::debug(FSTR_P const func, char c) {
    if (DEBUGGING(INFO)) { prefix(func); SERIAL_ECHOLN(c); }
  }
  void TWIBus::debug(FSTR_P const func, char str[]) {
    if (DEBUGGING(INFO)) { prefix(func); SERIAL_ECHOLN(str); }
  }

#endif

#endif // EXPERIMENTAL_I2CBUS
