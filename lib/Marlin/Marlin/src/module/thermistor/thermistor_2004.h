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

// R25 = 100 kOhm, beta25/85 = 4390K � 1%, 4.7 kOhm pull-up
// ATC Semitec 104JT-050 Jt Thermistor 100k 50 Shape
// Semitec corporaton .
// Resistance 100K/25�C
// Beta value 4390K
// Shape 50
// Highly accurate �1% tolerances
// Fast response to temperature change
// Working temperature -50�C -  +125�C
// Calculated using 4.7kohm pullup, voltage divider math, and manufacturer provided temp/resistance
const short temptable_2004[][2] PROGMEM = {
{ OV(354), 125 },
{ OV(387), 120 },
{ OV(463), 110 },
{ OV(543), 100 },
{ OV(626), 90 },
{ OV(706), 80 },
{ OV(780), 70 },
{ OV(844), 60 },
{ OV(896), 50 },
{ OV(937), 40 },
{ OV(966), 30 },
{ OV(987), 20 },
{ OV(1001), 10 },
{ OV(1010), 0 },
{ OV(1016), -10 },
{ OV(1019), -20 },
{ OV(1021), -30 },
};
