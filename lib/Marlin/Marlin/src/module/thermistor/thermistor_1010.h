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

// Pt1000 with 1k0 pullup
const short temptable_1010[][2] PROGMEM = {
  PtLine(  0, 1000, 1000),
  PtLine( 10, 1000, 1000),
  PtLine( 20, 1000, 1000),
  PtLine( 30, 1000, 1000),
  PtLine( 40, 1000, 1000),
  PtLine( 50, 1000, 1000),
  PtLine( 60, 1000, 1000),
  PtLine( 70, 1000, 1000),
  PtLine( 80, 1000, 1000),
  PtLine( 90, 1000, 1000),
  PtLine(100, 1000, 1000),
  PtLine(110, 1000, 1000),
  PtLine(120, 1000, 1000),
  PtLine(130, 1000, 1000),
  PtLine(140, 1000, 1000),
  PtLine(150, 1000, 1000),
  PtLine(160, 1000, 1000),
  PtLine(170, 1000, 1000),
  PtLine(180, 1000, 1000),
  PtLine(190, 1000, 1000),
  PtLine(200, 1000, 1000),
  PtLine(210, 1000, 1000),
  PtLine(220, 1000, 1000),
  PtLine(230, 1000, 1000),
  PtLine(240, 1000, 1000),
  PtLine(250, 1000, 1000),
  PtLine(260, 1000, 1000),
  PtLine(270, 1000, 1000),
  PtLine(280, 1000, 1000),
  PtLine(290, 1000, 1000),
  PtLine(300, 1000, 1000),
  PtLine(310, 1000, 1000),
  PtLine(320, 1000, 1000),
  PtLine(330, 1000, 1000),
  PtLine(340, 1000, 1000),
  PtLine(350, 1000, 1000)
};
