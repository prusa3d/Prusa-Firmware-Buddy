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

// R25 = 100 kOhm, beta25/85 = 4267K ą 2%, 1000 Ohm pull-up
// 100k thermistor NTCG NTC Chip Thermistor (104NT-4-R025H42G)
// Working temperature -50°C -  +300°C
// Calculated using 1000 Ohm pullup, voltage divider math, and manufacturer provided temp/resistance
// Thermistor table for PRUSA Inteligent extruder
// Calculated values are for 10 bit ADC reading
// NOTE Please ! OV function, recalculates the ADC read value based on oversamplenr factor

const short temptable_2005[][2] PROGMEM = {
{ OV(60), 320 }, // Projected value just to trigger error on short circuit
{ OV(67), 310 },
{ OV(78), 300 },
{ OV(102), 280 },
{ OV(134), 260 },
{ OV(178), 240 },
{ OV(237), 220 },
{ OV(315), 200 },
{ OV(414), 180 },
{ OV(530), 160 },
{ OV(654), 140 },
{ OV(771), 120 },
{ OV(867), 100 },
{ OV(922), 85 },
{ OV(936), 80 },
{ OV(979), 60 },
{ OV(993), 50 },
{ OV(1003), 40 },
{ OV(1013), 25 },
{ OV(1018), 10 },
{ OV(1020), 0 },
{ OV(1021), -10 },
{ OV(1023), -30 },
};
