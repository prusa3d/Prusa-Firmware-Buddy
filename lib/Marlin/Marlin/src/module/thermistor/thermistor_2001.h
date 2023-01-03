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

// R25 = 100 kOhm, beta25/85 = 4550K ± 3%, 4.7 kOhm pull-up
// 100k thermistor NTCG NTC Chip Thermistor (NTCG104LH104JTDS)
// TDK Corporation Piezo & Protection Device B. Grp.
// Working temperature -40°C -  +125°C
// Calculated using 4.7kohm pullup, voltage divider math, and manufacturer provided temp/resistance
const short temptable_2000[][2] PROGMEM = {
{ OV(  0), 700 },
{ OV(312), 125 },
{ OV(346), 120 },
{ OV(383), 115 },
{ OV(422), 110 },
{ OV(463), 105 },
{ OV(505), 100 },
{ OV(549), 95 },
{ OV(593), 90 },
{ OV(637), 85 },
{ OV(680), 80 },
{ OV(722), 75 },
{ OV(761), 70 },
{ OV(798), 65 },
{ OV(832), 60 },
{ OV(862), 55 },
{ OV(889), 50 },
{ OV(913), 45 },
{ OV(933), 40 },
{ OV(950), 35 },
{ OV(965), 30 },
{ OV(977), 25 },
{ OV(987), 20 },
{ OV(995), 15 },
{ OV(1001), 10 },
{ OV(1007), 5 },
{ OV(1011), 0 },
{ OV(1014), -5 },
{ OV(1016), -10 },
{ OV(1018), -15 },
{ OV(1019), -20 },
{ OV(1020), -25 },
{ OV(1021), -30 }

};
