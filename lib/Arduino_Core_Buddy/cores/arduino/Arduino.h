/*
  Arduino.h - Main include file for the Arduino SDK
  Copyright (c) 2005-2013 Arduino Team.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#pragma once

// #include "variant.h"
// #include "spi_com.h"
// #include "wiring_constants.h"
// #include "wiring_time.h"
// #include "wiring_digital.h"
#include "wiring.h"
#include "variant.h"
// #include "HardwareSerial.h"
// #include "binary.h"

// typedef uint8_t byte;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
extern void setup(void);
extern void loop(void);

void yield(void);
#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus
