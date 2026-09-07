/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2019 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#define REVERSE_TEMP_SENSOR_RANGE_1010 1

// Pt1000 with 1k0 pullup
// PT1000 temperature sensor with 1k pullup resistor
// Precise ADC values calculated for PT1000 with voltage divider
const short temptable_1010[][2] PROGMEM = {
  { OV( 512),   0 },
  { OV( 536),  25 },
  { OV( 557),  50 },
  { OV( 577),  75 },
  { OV( 595), 100 },
  { OV( 611), 125 },
  { OV( 626), 150 },
  { OV( 640), 175 },
  { OV( 653), 200 },
  { OV( 665), 225 },
  { OV( 676), 250 },
  { OV( 686), 275 },
  { OV( 696), 300 },
  { OV( 705), 325 },
  { OV( 713), 350 },
  { OV( 721), 375 },
  { OV( 729), 400 },
  // Projected values above the operating range, computed from IEC 60751 (PT1000 class B,
  // R0=1000Ω, A=3.9083e-3, B=-5.775e-7) with a 1kΩ pullup. Required so
  // MarlinTemptableRawMinMax::compute() can establish a safety threshold above
  // the HT hotend max nozzle temperature (415°C) — without these the threshold collapses to the last real entry (400°C).
  { OV( 736), 425 },
  { OV( 743), 450 }
};
