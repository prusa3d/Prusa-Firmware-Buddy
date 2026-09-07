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

#include <math.h>
#include <stddef.h>

#include "inc/MarlinConfigPre.h"
#include "vector.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

#define NUM_AXIS_GANG(V...) GANG_N(NUM_AXES, V)
#define NUM_AXIS_CODE(V...) CODE_N(NUM_AXES, V)
#define NUM_AXIS_LIST(V...) LIST_N(NUM_AXES, V)
#define NUM_AXIS_LIST_1(V)  LIST_N_1(NUM_AXES, V)
#define NUM_AXIS_ARRAY(V...) { NUM_AXIS_LIST(V) }
#define NUM_AXIS_ARRAY_1(V)  { NUM_AXIS_LIST_1(V) }
#define NUM_AXIS_ARGS(T...) NUM_AXIS_LIST(T x, T y, T z, T i, T j, T k, T u, T v, T w)
#define NUM_AXIS_ELEM(O)    NUM_AXIS_LIST(O.x, O.y, O.z, O.i, O.j, O.k, O.u, O.v, O.w)
#define NUM_AXIS_DEFS(T,V)  NUM_AXIS_LIST(T x=V, T y=V, T z=V, T i=V, T j=V, T k=V, T u=V, T v=V, T w=V)
#define MAIN_AXIS_NAMES     NUM_AXIS_LIST(X, Y, Z, I, J, K, U, V, W)
#define MAIN_AXIS_MAP(F)    MAP(F, MAIN_AXIS_NAMES)
#define STR_AXES_MAIN       NUM_AXIS_GANG("X", "Y", "Z", STR_I, STR_J, STR_K, STR_U, STR_V, STR_W)

#define LOGICAL_AXIS_GANG(E,V...) NUM_AXIS_GANG(V) GANG_ITEM_E(E)
#define LOGICAL_AXIS_CODE(E,V...) NUM_AXIS_CODE(V) CODE_ITEM_E(E)
#define LOGICAL_AXIS_LIST(E,V...) NUM_AXIS_LIST(V) LIST_ITEM_E(E)
#define LOGICAL_AXIS_LIST_1(V)    NUM_AXIS_LIST_1(V) LIST_ITEM_E(V)
#define LOGICAL_AXIS_ARRAY(E,V...) { LOGICAL_AXIS_LIST(E,V) }
#define LOGICAL_AXIS_ARRAY_1(V)    { LOGICAL_AXIS_LIST_1(V) }
#define LOGICAL_AXIS_ARGS(T...) LOGICAL_AXIS_LIST(T e, T x, T y, T z, T i, T j, T k, T u, T v, T w)
#define LOGICAL_AXIS_ELEM(O)    LOGICAL_AXIS_LIST(O.e, O.x, O.y, O.z, O.i, O.j, O.k, O.u, O.v, O.w)
#define LOGICAL_AXIS_DECL(T,V)  LOGICAL_AXIS_LIST(T e=V, T x=V, T y=V, T z=V, T i=V, T j=V, T k=V, T u=V, T v=V, T w=V)
#define LOGICAL_AXIS_NAMES      LOGICAL_AXIS_LIST(E, X, Y, Z, I, J, K, U, V, W)
#define LOGICAL_AXIS_MAP(F)     MAP(F, LOGICAL_AXIS_NAMES)
#define STR_AXES_LOGICAL     LOGICAL_AXIS_GANG("E", "X", "Y", "Z", STR_I, STR_J, STR_K, STR_U, STR_V, STR_W)

#define XYZ_GANG(V...) GANG_N(PRIMARY_LINEAR_AXES, V)
#define XYZ_CODE(V...) CODE_N(PRIMARY_LINEAR_AXES, V)

#define SECONDARY_AXIS_GANG(V...) GANG_N(SECONDARY_AXES, V)
#define SECONDARY_AXIS_CODE(V...) CODE_N(SECONDARY_AXES, V)

#if HAS_ROTATIONAL_AXES
  // #error dead code found by automatic analyses (see BFW-5461)
  #define ROTATIONAL_AXIS_GANG(V...) GANG_N(ROTATIONAL_AXES, V)
#endif

#if HAS_EXTRUDERS
  #define LIST_ITEM_E(N) , N
  #define CODE_ITEM_E(N) ; N
  #define GANG_ITEM_E(N) N
#else
  #define LIST_ITEM_E(N)
  #define CODE_ITEM_E(N)
  #define GANG_ITEM_E(N)
#endif

#define AXIS_COLLISION(L) (AXIS4_NAME == L || AXIS5_NAME == L || AXIS6_NAME == L || AXIS7_NAME == L || AXIS8_NAME == L || AXIS9_NAME == L)

//
// Enumerated axis indices
//
//  - X_AXIS, Y_AXIS, and Z_AXIS should be used for axes in Cartesian space
//  - A_AXIS, B_AXIS, and C_AXIS should be used for Steppers, corresponding to XYZ on Cartesians
//  - X_HEAD, Y_HEAD, and Z_HEAD should be used for Steppers on Core kinematics
//
enum AxisEnum : uint8_t {

  // Linear axes may be controlled directly or indirectly
  NUM_AXIS_LIST(X_AXIS, Y_AXIS, Z_AXIS, I_AXIS, J_AXIS, K_AXIS, U_AXIS, V_AXIS, W_AXIS)

  // Extruder axes may be considered distinctly
  #define _EN_ITEM(N) , E##N##_AXIS
  REPEAT(E_STEPPERS, _EN_ITEM)
  #undef _EN_ITEM

  // Core also keeps toolhead directions
  #if ANY(IS_CORE, MARKFORGED_XY, MARKFORGED_YX)
    , X_HEAD, Y_HEAD, Z_HEAD
  #endif

  // Distinct axes, including all E and Core
  , NUM_AXIS_ENUMS

  // Most of the time we refer only to the single E_AXIS
  #if HAS_EXTRUDERS
    , E_AXIS = E0_AXIS
  #endif

  // A, B, and C are for DELTA, SCARA, etc.
  , A_AXIS = X_AXIS
  #if HAS_Y_AXIS
    , B_AXIS = Y_AXIS
  #endif
  #if HAS_Z_AXIS
    , C_AXIS = Z_AXIS
  #endif

  // To refer to all or none
  , ALL_AXES_ENUM = 0xFE, NO_AXIS_ENUM = 0xFF
};

//
// Loop over axes
//
#define LOOP_NUM_AXES(VAR) LOOP_S_L_N(VAR, X_AXIS, NUM_AXES)
#define LOOP_LOGICAL_AXES(VAR) LOOP_S_L_N(VAR, X_AXIS, LOGICAL_AXES)
#define LOOP_DISTINCT_AXES(VAR) LOOP_S_L_N(VAR, X_AXIS, DISTINCT_AXES)
#define LOOP_DISTINCT_E(VAR) LOOP_L_N(VAR, DISTINCT_E)

//
// feedRate_t is just a humble float
//
using feedRate_t = float;

//
// celsius_t is the native unit of temperature. Signed to handle a disconnected thermistor value (-14).
// For more resolition (e.g., for a chocolate printer) this may later be changed to Celsius x 100
//
using raw_adc_t = uint16_t;
using celsius_t = int16_t;
using celsius_float_t = float;

// Conversion macros
#define MMM_TO_MMS(MM_M) feedRate_t(static_cast<float>(MM_M) / 60.0f)
#define MMS_TO_MMM(MM_S) (static_cast<float>(MM_S) * 60.0f)

//
// Coordinates structures for XY, XYZ, XYZE...
//

/// Tag for logical position strong types.
/// Logical vectors are used only on the G-Code level - those are XYZE coordinates BEFORE hotend offseds (and workspace offset) are applied
struct LogicalPosTag {};

/// Tag for native position strong types.
/// Native positions are AFTER hotend offsets applied, but BEFORE modifiers (MBL, skew, ...) applied.
/// Vast majority of the firmware works with native coordinates
struct NativePosTag {};

/// Tag for machine position strong types.
/// Machine positions are AFTER everything applied (hotend offsets, MBL, skew, ...).
/// This is what planner works with.
/// REMOVEME: For now, make it just an alias for NativePosTag so that we can migrate gradually.
using MachinePosTag = NativePosTag;

template <typename T, typename Tag = NativePosTag>
using XYval = Vector<T, Tag, 2>;

template <typename T, typename Tag = NativePosTag>
using XYZval = Vector<T, Tag, 3>;

static_assert(NUM_AXES == 3);

template <typename T, typename Tag = NativePosTag>
using XYZEval = Vector<T, Tag, 4>;

static_assert(LOGICAL_AXES == 4);

using xy_bool_t = XYval<bool>;
using xyz_bool_t = XYZval<bool>;
using xyze_bool_t = XYZEval<bool>;

using xy_char_t = XYval<char>;
using xyz_char_t = XYZval<char>;
using xyze_char_t = XYZEval<char>;

using xy_uchar_t = XYval<unsigned char>;
using xyz_uchar_t = XYZval<unsigned char>;
using xyze_uchar_t = XYZEval<unsigned char>;

using xy_int8_t = XYval<int8_t>;
using xyz_int8_t = XYZval<int8_t>;
using xyze_int8_t = XYZEval<int8_t>;

using xy_uint8_t = XYval<uint8_t>;
using xyz_uint8_t = XYZval<uint8_t>;
using xyze_uint8_t = XYZEval<uint8_t>;

using xy_int_t = XYval<int16_t>;
using xyz_int_t = XYZval<int16_t>;
using xyze_int_t = XYZEval<int16_t>;

using xy_uint_t = XYval<uint16_t>;
using xyz_uint_t = XYZval<uint16_t>;
using xyze_uint_t = XYZEval<uint16_t>;

using xy_long_t = XYval<int32_t>;
using xyz_long_t = XYZval<int32_t>;
using xyze_long_t = XYZEval<int32_t>;

using xy_ulong_t = XYval<uint32_t>;
using xyz_ulong_t = XYZval<uint32_t>;
using xyze_ulong_t = XYZEval<uint32_t>;

using xyz_vlong_t = XYZval<volatile int32_t>;
using xyze_vlong_t = XYZEval<volatile int32_t>;

using xy_float_t = XYval<float>;
using xyz_float_t = XYZval<float>;
using xyze_float_t = XYZEval<float>;

using xyz_double_t = XYZval<double>;
using xyze_double_t = XYZEval<double>;

using xy_feedrate_t = XYval<feedRate_t>;
using xyz_feedrate_t = XYZval<feedRate_t>;
using xyze_feedrate_t = XYZEval<feedRate_t>;

using xy_byte_t = xy_uint8_t;
using xyz_byte_t = xyz_uint8_t;
using xyze_byte_t = xyze_uint8_t;

using xy_pos_t = xy_float_t;
using xyz_pos_t = xyz_float_t;
using xyze_pos_t = xyze_float_t;

using MachinePosXY = XYval<float, MachinePosTag>;
using MachinePosXYZ = XYZval<float, MachinePosTag>;
using MachinePosXYZE = XYZEval<float, MachinePosTag>;

// TODO: remove or integrate these local/legacy exceptions
#define LOOP_XY(VAR) LOOP_S_LE_N(VAR, X_AXIS, Y_AXIS)
#define LOOP_XYZ(VAR) LOOP_S_LE_N(VAR, X_AXIS, Z_AXIS)
#define LOOP_XYZE(VAR) LOOP_S_LE_N(VAR, X_AXIS, E_AXIS)
#define LOOP_XYZE_N(VAR) LOOP_S_L_N(VAR, X_AXIS, XYZE_N)

#pragma GCC diagnostic pop
