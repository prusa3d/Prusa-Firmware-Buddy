/**
 *
 * Macros for feature testing.
 *
 * We need our macros to be includable together with Marlin's macros. It would be hard to make sure
 * none of our macros name-colide with Marlin ones. Therefore, macros here are a strict subset of Marlin's
 * macros.
 *
 * In case you need to include feature/something and some Marlin header file, make sure to include Marlin's header first.
 *
 *
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

#if !defined(ENABLED)

    // Use NUM_ARGS(__VA_ARGS__) to get the number of variadic arguments
    #define _NUM_ARGS(_, Z, Y, X, W, V, U, T, S, R, Q, P, O, N, M, L, K, J, I, H, G, F, E, D, C, B, A, OUT, ...) OUT
    #define NUM_ARGS(V...)                                                                                       _NUM_ARGS(0, V, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)

    // Macros to chain up to 12 conditions
    #define _DO_1(W, C, A)        (_##W##_1(A))
    #define _DO_2(W, C, A, B)     (_##W##_1(A) C _##W##_1(B))
    #define _DO_3(W, C, A, V...)  (_##W##_1(A) C _DO_2(W, C, V))
    #define _DO_4(W, C, A, V...)  (_##W##_1(A) C _DO_3(W, C, V))
    #define _DO_5(W, C, A, V...)  (_##W##_1(A) C _DO_4(W, C, V))
    #define _DO_6(W, C, A, V...)  (_##W##_1(A) C _DO_5(W, C, V))
    #define _DO_7(W, C, A, V...)  (_##W##_1(A) C _DO_6(W, C, V))
    #define _DO_8(W, C, A, V...)  (_##W##_1(A) C _DO_7(W, C, V))
    #define _DO_9(W, C, A, V...)  (_##W##_1(A) C _DO_8(W, C, V))
    #define _DO_10(W, C, A, V...) (_##W##_1(A) C _DO_9(W, C, V))
    #define _DO_11(W, C, A, V...) (_##W##_1(A) C _DO_10(W, C, V))
    #define _DO_12(W, C, A, V...) (_##W##_1(A) C _DO_11(W, C, V))
    #define __DO_N(W, C, N, V...) _DO_##N(W, C, V)
    #define _DO_N(W, C, N, V...)  __DO_N(W, C, N, V)
    #define DO(W, C, V...)        _DO_N(W, C, NUM_ARGS(V), V)

    // Macros to support feature testing
    #define _CAT(a, V...)        a##V
    #define SWITCH_ENABLED_false 0
    #define SWITCH_ENABLED_true  1
    #define SWITCH_ENABLED_0     0
    #define SWITCH_ENABLED_1     1
    #define SWITCH_ENABLED_0x0   0
    #define SWITCH_ENABLED_0x1   1
    #define SWITCH_ENABLED_      1
    #define _ENA_1(O)            _CAT(SWITCH_ENABLED_, O)
    #define _DIS_1(O)            !_ENA_1(O)
    #define ENABLED(V...)        DO(ENA, &&, V)
    #define DISABLED(V...)       DO(DIS, &&, V)

    #define ANY(V...)      !DISABLED(V)
    #define NONE(V...)     DISABLED(V)
    #define ALL(V...)      ENABLED(V)
    #define BOTH(V1, V2)   ALL(V1, V2)
    #define EITHER(V1, V2) ANY(V1, V2)
#endif
