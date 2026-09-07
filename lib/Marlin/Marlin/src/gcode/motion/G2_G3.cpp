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

#include "../../inc/MarlinConfig.h"
#include <option/has_crash_detection.h>

#if ENABLED(ARC_SUPPORT)

#include "../gcode.h"
#include "../../module/motion.h"
#include "../../module/planner.h"
#include "../../module/temperature.h"
#include "feature/precise_stepping/internal.hpp"

#include <option/has_cancel_object.h>
#if HAS_CANCEL_OBJECT()
  #include <feature/cancel_object/cancel_object.hpp>
#endif

#if HAS_CRASH_DETECTION()
  #include <feature/prusa/crash_recovery.hpp>
#endif

#if N_ARC_CORRECTION < 1
  // #error dead code found by automatic analyses (see BFW-5461)
  #undef N_ARC_CORRECTION
  #define N_ARC_CORRECTION 1
#endif
#if !defined(MAX_ARC_SEGMENT_MM) && defined(MIN_ARC_SEGMENT_MM)
  // #error dead code found by automatic analyses (see BFW-5461)
  #define MAX_ARC_SEGMENT_MM MIN_ARC_SEGMENT_MM
#elif !defined(MIN_ARC_SEGMENT_MM) && defined(MAX_ARC_SEGMENT_MM)
  // #error dead code found by automatic analyses (see BFW-5461)
  #define MIN_ARC_SEGMENT_MM MAX_ARC_SEGMENT_MM
#endif

#define ARC_LIJKUVW_CODE(L,I,J,K,U,V,W)    CODE_N(SUB2(NUM_AXES),L,I,J,K,U,V,W)
#define ARC_LIJKUVWE_CODE(L,I,J,K,U,V,W,E) ARC_LIJKUVW_CODE(L,I,J,K,U,V,W); CODE_ITEM_E(E)

void plan_arc(const xyze_pos_t &cart, const xy_float_t &offset,
  const bool clockwise, const uint8_t circles, bool last_segment) {

  #if ENABLED(CNC_WORKSPACE_PLANES)
    // #error dead code found by automatic analyses (see BFW-5461)
    AxisEnum axis_p, axis_q, axis_l;
    switch (gcode.workspace_plane) {
      default:
      case GcodeSuite::PLANE_XY: axis_p = X_AXIS; axis_q = Y_AXIS; axis_l = Z_AXIS; break;
      case GcodeSuite::PLANE_YZ: axis_p = Y_AXIS; axis_q = Z_AXIS; axis_l = X_AXIS; break;
      case GcodeSuite::PLANE_ZX: axis_p = Z_AXIS; axis_q = X_AXIS; axis_l = Y_AXIS; break;
    }
  #else
    constexpr AxisEnum axis_p = X_AXIS, axis_q = Y_AXIS OPTARG(HAS_Z_AXIS, axis_l = Z_AXIS);
  #endif

  // Radius vector from center to current location
  xy_float_t rvec = -offset;

  const float radius = HYPOT(rvec.x, rvec.y),
              center_P = current_position[axis_p] - rvec.x,
              center_Q = current_position[axis_q] - rvec.y,
              rt_X = cart[axis_p] - center_P,
              rt_Y = cart[axis_q] - center_Q;

  ARC_LIJKUVWE_CODE(
    float start_L = current_position[axis_l],
    float start_I = current_position.i,
    float start_J = current_position.j,
    float start_K = current_position.k,
    float start_U = current_position.u,
    float start_V = current_position.v,
    float start_W = current_position.w,
    float start_E = current_position.e
  );

  // Angle of rotation between position and target from the circle center.
  float angular_travel, abs_angular_travel;

  // Do a full circle if starting and ending positions are "identical"
  if (NEAR(current_position[axis_p], cart[axis_p]) && NEAR(current_position[axis_q], cart[axis_q])) {
    // Preserve direction for circles
    angular_travel = clockwise ? -RADIANS(360) : RADIANS(360);
    abs_angular_travel = RADIANS(360);
  }
  else {
    // Calculate the angle
    angular_travel = ATAN2(rvec.x * rt_Y - rvec.y * rt_X, rvec.x * rt_X + rvec.y * rt_Y);

    // Angular travel too small to detect? Just return.
    if (!angular_travel) return;

    // Make sure angular travel over 180 degrees goes the other way around.
    switch (((angular_travel < 0) << 1) | clockwise) {
      case 1: angular_travel -= RADIANS(360); break; // Positive but CW? Reverse direction.
      case 2: angular_travel += RADIANS(360); break; // Negative but CCW? Reverse direction.
    }

    abs_angular_travel = ABS(angular_travel);
  }

  ARC_LIJKUVWE_CODE(
    float travel_L = cart[axis_l] - start_L,
    float travel_I = cart.i       - start_I,
    float travel_J = cart.j       - start_J,
    float travel_K = cart.k       - start_K,
    float travel_U = cart.u       - start_U,
    float travel_V = cart.v       - start_V,
    float travel_W = cart.w       - start_W,
    float travel_E = cart.e       - start_E
  );

  // If "P" specified circles, call plan_arc recursively then continue with the rest of the arc
  if (TERN0(ARC_P_CIRCLES, circles)) {
    const float total_angular = abs_angular_travel + circles * RADIANS(360),    // Total rotation with all circles and remainder
              part_per_circle = RADIANS(360) / total_angular;                   // Each circle's part of the total

    ARC_LIJKUVWE_CODE(
      const float per_circle_L = travel_L * part_per_circle,    // X, Y, or Z movement per circle
      const float per_circle_I = travel_I * part_per_circle,    // The rest are also non-arc
      const float per_circle_J = travel_J * part_per_circle,
      const float per_circle_K = travel_K * part_per_circle,
      const float per_circle_U = travel_U * part_per_circle,
      const float per_circle_V = travel_V * part_per_circle,
      const float per_circle_W = travel_W * part_per_circle,
      const float per_circle_E = travel_E * part_per_circle     // E movement per circle
    );

    xyze_pos_t temp_position = current_position;
    for (uint16_t n = circles; n--;) {
      ARC_LIJKUVWE_CODE(                                        // Destination Linear Axes
        temp_position[axis_l] += per_circle_L,                  // Linear X, Y, or Z
        temp_position.i       += per_circle_I,                  // The rest are also non-circular
        temp_position.j       += per_circle_J,
        temp_position.k       += per_circle_K,
        temp_position.u       += per_circle_U,
        temp_position.v       += per_circle_V,
        temp_position.w       += per_circle_W,
        temp_position.e       += per_circle_E                   // Destination E axis
      );
      plan_arc(temp_position, offset, clockwise, 0);            // Plan a single whole circle
    }

    // Update starting coordinates for the remaining part
    ARC_LIJKUVWE_CODE(
      start_L = current_position[axis_l],
      start_I = current_position.i,
      start_J = current_position.j,
      start_K = current_position.k,
      start_U = current_position.u,
      start_V = current_position.v,
      start_W = current_position.w,
      start_E = current_position.e
    );
    ARC_LIJKUVWE_CODE(
      travel_L = cart[axis_l] - start_L,                        // Linear X, Y, or Z
      travel_I = cart.i       - start_I,                        // The rest are also non-arc
      travel_J = cart.j       - start_J,
      travel_K = cart.k       - start_K,
      travel_U = cart.u       - start_U,
      travel_V = cart.v       - start_V,
      travel_W = cart.w       - start_W,
      travel_E = cart.e       - start_E
    );
  }

  // Millimeters in the arc, assuming it's flat
  const float flat_mm = radius * abs_angular_travel;

  // Return if the move is near zero
  if (flat_mm < 0.0001f
    GANG_N(SUB2(NUM_AXES),                                      // Two axes for the arc
      && NEAR_ZERO(travel_L),                                   // Linear X, Y, or Z
      && NEAR_ZERO(travel_I),
      && NEAR_ZERO(travel_J),
      && NEAR_ZERO(travel_K),
      && NEAR_ZERO(travel_U),
      && NEAR_ZERO(travel_V),
      && NEAR_ZERO(travel_W)
    )
  ) {
    #if HAS_EXTRUDERS
      if (!NEAR_ZERO(travel_E)) gcode.G0_G1();                  // Handle retract/recover as G1
      return;
    #endif
  }

  // Feedrate for the move, scaled by the feedrate multiplier
  const feedRate_t scaled_fr_mm_s = MMS_SCALED(feedrate_mm_s);

  // Compute how many segments to use adapted from RepRapFirmware.
  // For the arc to deviate up to MAX_ARC_DEVIATION from the ideal, the segment length should be sqrtf(8 * radius * MAX_ARC_DEVIATION + fast_sqrt(MAX_ARC_DEVIATION)).
  // We leave out the square term because it is very small.
  // In CNC applications even very small deviations can be visible, so we use a smaller segment length at low speeds.
  const float segment_mm = std::clamp(std::min(fast_sqrt(8 * radius * float(MAX_ARC_DEVIATION)), feedrate_mm_s * (1.f / float(MIN_ARC_SEGMENTS_PER_SEC))), float(MIN_ARC_SEGMENT_MM), float(MAX_ARC_SEGMENT_MM));
  const uint32_t segments = std::max(uint32_t(flat_mm / segment_mm + 0.8f), 1ul);

  // Add hints to help optimize the move
  PlannerHints hints {
    .move = {
      .is_printing_move = true,
    }
  };

  #if ENABLED(FEEDRATE_SCALING)
    // #error dead code found by automatic analyses (see BFW-5461)
    hints.inv_duration = (scaled_fr_mm_s / flat_mm) * segments;
  #endif

  /**
   * Vector rotation by transformation matrix: r is the original vector, r_T is the rotated vector,
   * and phi is the angle of rotation. Based on the solution approach by Jens Geisler.
   *     r_T = [cos(phi) -sin(phi);
   *            sin(phi)  cos(phi)] * r ;
   *
   * For arc generation, the center of the circle is the axis of rotation and the radius vector is
   * defined from the circle center to the initial position. Each line segment is formed by successive
   * vector rotations. This requires only two cos() and sin() computations to form the rotation
   * matrix for the duration of the entire arc. Error may accumulate from numerical round-off, since
   * all double numbers are single precision on the Arduino. (True double precision will not have
   * round off issues for CNC applications.) Single precision error can accumulate to be greater than
   * tool precision in some cases. Therefore, arc path correction is implemented.
   *
   * Small angle approximation may be used to reduce computation overhead further. This approximation
   * holds for everything, but very small circles and large MAX_ARC_SEGMENT_MM values. In other words,
   * theta_per_segment would need to be greater than 0.1 rad and N_ARC_CORRECTION would need to be large
   * to cause an appreciable drift error. N_ARC_CORRECTION~=25 is more than small enough to correct for
   * numerical drift error. N_ARC_CORRECTION may be on the order a hundred(s) before error becomes an
   * issue for CNC machines with the single precision Arduino calculations.
   *
   * This approximation also allows plan_arc to immediately insert a line segment into the planner
   * without the initial overhead of computing cos() or sin(). By the time the arc needs to be applied
   * a correction, the planner should have caught up to the lag caused by the initial plan_arc overhead.
   * This is important when there are successive arc motions.
   */

  xyze_pos_t segment_target;

  // do not calculate rotation parameters for trivial single-segment arcs
  if (segments > 1) {
    // Vector rotation matrix values
    const float theta_per_segment = angular_travel / segments,
                cos_T = cos(theta_per_segment),
                sin_T = sin(theta_per_segment);

    ARC_LIJKUVWE_CODE(
      const float per_segment_L = travel_L / segments,
      const float per_segment_I = travel_I / segments,
      const float per_segment_J = travel_J / segments,
      const float per_segment_K = travel_K / segments,
      const float per_segment_U = travel_U / segments,
      const float per_segment_V = travel_V / segments,
      const float per_segment_W = travel_W / segments,
      const float per_segment_E = travel_E / segments
    );

    // Initialize all linear axes and E
    ARC_LIJKUVWE_CODE(
      segment_target[axis_l] = start_L,
      segment_target.i       = start_I,
      segment_target.j       = start_J,
      segment_target.k       = start_K,
      segment_target.u       = start_U,
      segment_target.v       = start_V,
      segment_target.w       = start_W,
      segment_target.e       = start_E
    );

    millis_t next_idle_ms = millis() + 200UL;

    #if N_ARC_CORRECTION > 1
      int8_t arc_recalc_count = N_ARC_CORRECTION;
    #endif

    for (uint16_t i = 1; i < segments; i++) { // Iterate (segments-1) times

      thermalManager.manage_heater();
      const millis_t ms = millis();
      if (ELAPSED(ms, next_idle_ms)) {
        next_idle_ms = ms + 200UL;
        idle(false);
      }

      #if N_ARC_CORRECTION > 1
        if (--arc_recalc_count) {
          // Apply vector rotation matrix to previous rvec.a / 1
          const float r_new_Y = rvec.a * sin_T + rvec.b * cos_T;
          rvec.a = rvec.a * cos_T - rvec.b * sin_T;
          rvec.b = r_new_Y;
        }
        else
      #endif
      {
        #if N_ARC_CORRECTION > 1
          arc_recalc_count = N_ARC_CORRECTION;
        #endif

        // Arc correction to radius vector. Computed only every N_ARC_CORRECTION increments.
        // Compute exact location by applying transformation matrix from initial radius vector(=-offset).
        // To reduce stuttering, the sin and cos could be computed at different times.
        // For now, compute both at the same time.
        const float Ti = i * theta_per_segment, cos_Ti = cos(Ti), sin_Ti = sin(Ti);
        rvec.a = -offset[0] * cos_Ti + offset[1] * sin_Ti;
        rvec.b = -offset[0] * sin_Ti - offset[1] * cos_Ti;
      }

      // Update segment_target location
      segment_target[axis_p] = center_P + rvec.a;
      segment_target[axis_q] = center_Q + rvec.b;
      ARC_LIJKUVWE_CODE(
        segment_target[axis_l] = start_L + per_segment_L * i,
        segment_target.i       = start_I + per_segment_I * i,
        segment_target.j       = start_J + per_segment_J * i,
        segment_target.k       = start_K + per_segment_K * i,
        segment_target.u       = start_U + per_segment_U * i,
        segment_target.v       = start_V + per_segment_V * i,
        segment_target.w       = start_W + per_segment_W * i,
        segment_target.e       = start_E + per_segment_E * i
      );

      apply_motion_limits(segment_target);

      if (!planner.buffer_line(to_machine_pos(segment_target), scaled_fr_mm_s, PhysicalToolIndex::currently_selected(), hints))
        break;

      set_current_position(segment_target);
    }
  }

  // Ensure last segment arrives at target location.
  segment_target = cart;
  apply_motion_limits(segment_target);

  planner.buffer_line(to_machine_pos(segment_target), scaled_fr_mm_s, PhysicalToolIndex::currently_selected(), hints);
  set_current_position(segment_target);

} // plan_arc

/** \addtogroup G-Codes
 * @{
 */

/**
 * ### G2: Clockwise Arc, G3: Counterclockwise Arc <a href="https://reprap.org/wiki/G-code#G2_.26_G3:_Controlled_Arc_Move">G2 & G3: Controlled Arc Move</a>
 *
 * This command has two forms: IJ-form (JK, KI) and R-form.
 *
 *  - Depending on the current Workspace Plane orientation,
 *    use parameters IJ/JK/KI to specify the XY/YZ/ZX offsets.
 *    At least one of the IJ/JK/KI parameters is required.
 *    XY/YZ/ZX can be omitted to do a complete circle.
 *    The given XY/YZ/ZX is not error-checked. The arc ends
 *    based on the angle of the destination.
 *    Mixing IJ/JK/KI with R will throw an error.
 *
 *  - R specifies the radius. X or Y (Y or Z / Z or X) is required.
 *      Omitting both XY/YZ/ZX will throw an error.
 *      XY/YZ/ZX must differ from the current XY/YZ/ZX.
 *      Mixing R with IJ/JK/KI will throw an error.
 *
 *  - P specifies the number of full circles to do
 *      before the specified arc move.
 *
 * #### Usage
 *
 *     G2 [ X | Y | Z | E | F | I | J | R ] (Clockwise Arc)
 *     G3 [ X | Y | Z | E | F | I | J | R ] (Counter-Clockwise Arc)
 *
 * #### Parameters
 *
 *  - `X` - The position to move to on the X axis
 *  - `Y` - The position to move to on the Y axis
 *  - `Z` - The position to move to on the Z axis
 *  - `E` - The amount to extrude between the starting point and ending point
 *  - `F` - The feedrate per minute of the move between the starting point and ending point (if supplied)
 *  - `I` - The point in X space from the current X position to maintain a constant distance from
 *  - `J` - The point in Y space from the current Y position to maintain a constant distance from
 *  - `R` - Constant radius from the current XY (YZ or ZX) position
 *
 *  #### Examples:
 *
 *      G2 I10           ; CW circle centered at X+10
 *      G3 X20 Y12 R14   ; CCW circle with r=14 ending at X20 Y12
 */
void GcodeSuite::G2_G3(const bool clockwise) {
  TERN_(FULL_REPORT_TO_HOST_FEATURE, set_and_report_grblstate(M_RUNNING));

  #if HAS_CRASH_DETECTION()
    // allow full instruction recovery
    crash_s.set_gcode_replay_flags(Crash_s::RECOVER_FULL);
  #endif

  #if ENABLED(SF_ARC_FIX)
    // #error dead code found by automatic analyses (see BFW-5461)
    const bool relative_mode_backup = relative_mode;
    relative_mode = true;
  #endif

  get_destination_from_command();   // Get X Y [Z[I[J[K...]]]] [E] F (and set cutter power)

  TERN_(SF_ARC_FIX, relative_mode = relative_mode_backup);

  xy_float_t arc_offset = { 0, 0 };
  if (parser.seenval('R')) {
    const float r = parser.value_linear_units();
    if (r) {
      const xy_pos_t p1 = current_position.xy(), p2 = destination.xy();
      if (p1 != p2) {
        const xy_pos_t d2 = (p2 - p1) * 0.5f;          // XY vector to midpoint of move from current
        const float e = clockwise ^ (r < 0) ? -1 : 1,  // clockwise -1/1, counterclockwise 1/-1
                    len = d2.magnitude(),              // Distance to mid-point of move from current
                    h2 = (r - len) * (r + len),        // factored to reduce rounding error
                    h = (h2 >= 0) ? SQRT(h2) : 0.0f;   // Distance to the arc pivot-point from midpoint
        const xy_pos_t s = { -d2.y, d2.x };            // Perpendicular bisector. (Divide by len for unit vector.)
        arc_offset = d2 + s / len * e * h;             // The calculated offset (mid-point if |r| <= len)
      }
    }
  }
  else {
    #if ENABLED(CNC_WORKSPACE_PLANES)
      // #error dead code found by automatic analyses (see BFW-5461)
      char achar, bchar;
      switch (workspace_plane) {
        default:
        case GcodeSuite::PLANE_XY: achar = 'I'; bchar = 'J'; break;
        case GcodeSuite::PLANE_YZ: achar = 'J'; bchar = 'K'; break;
        case GcodeSuite::PLANE_ZX: achar = 'K'; bchar = 'I'; break;
      }
    #else
      constexpr char achar = 'I', bchar = 'J';
    #endif
    if (parser.seenval(achar)) arc_offset.a = parser.value_linear_units();
    if (parser.seenval(bchar)) arc_offset.b = parser.value_linear_units();
  }

  if (TERN0(HAS_CANCEL_OBJECT(), buddy::cancel_object().is_current_object_cancelled())) {
    // Canceling an object, skip arc move
  } else if (arc_offset != xy_pos_t{0, 0}) {

    #if ENABLED(ARC_P_CIRCLES)
      // #error dead code found by automatic analyses (see BFW-5461)
      // P indicates number of circles to do
      const int8_t circles_to_do = parser.byteval('P');
      if (!WITHIN(circles_to_do, 0, 100))
        SERIAL_ERROR_MSG(STR_ERR_ARC_ARGS);
    #else
      constexpr uint8_t circles_to_do = 0;
    #endif

    // Send the arc to the planner
    plan_arc(destination, arc_offset, clockwise, circles_to_do, true);
  }
  else
    SERIAL_ERROR_MSG(STR_ERR_ARC_ARGS);

  TERN_(FULL_REPORT_TO_HOST_FEATURE, set_and_report_grblstate(M_IDLE));
}

/** @}*/

#endif // ARC_SUPPORT
