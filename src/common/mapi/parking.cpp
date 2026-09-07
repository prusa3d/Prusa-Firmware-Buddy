#include "parking.hpp"

#include <Marlin/src/gcode/gcode.h>
#include <Marlin/src/module/motion.h>
#include <config_store/store_instance.hpp>
#include <utils/overloaded_visitor.hpp>
#include <module/planner.h>
#include <mapi/motion.hpp>

#include <option/has_wastebin.h>
#include <option/has_wastebin_fill_tracking.h>

#include <option/has_indx.h>
#if HAS_INDX()
    #include <nozzle_cleaner.hpp>
#endif

#include <option/has_toolchanger.h>
#include <bsod/bsod.h>
#if HAS_TOOLCHANGER()
    #include <module/prusa/toolchanger.h>
#endif

namespace mapi {

// Make sure our little [[no_unique_address]] trick works
static_assert(sizeof(ParkingPosition::AtLeast) == 8);

const ParkArgs ParkArgs::default_args {};

#if HAS_INDX()
/// Positions that get apply_nozzle_cleaner_offset() applied are relative to the nozzle cleaner,
/// so they must lie in the wastebin area, otherwise applying the offset makes no sense
constexpr bool is_in_wastebin_area(float x, float y) {
    return x > X_WASTEBIN_SAFE_POINT && y > Y_WASTEBIN_SAFE_POINT;
}

/// Also checks that X and Y are absolute coordinates - apply_nozzle_cleaner_offset()
/// silently leaves the Unchanged ones alone
constexpr bool is_in_wastebin_area(const ParkingPosition &position) {
    const auto *x = std::get_if<float>(&position.x);
    const auto *y = std::get_if<float>(&position.y);
    return x && y && is_in_wastebin_area(*x, *y);
}
#endif

ParkingPosition get_parking_position(ParkPosition position, [[maybe_unused]] std::variant<PhysicalToolIndex, NoTool> tool) {
    switch (position) {
    case ParkPosition::park:
#if HAS_INDX()
        static_assert(is_in_wastebin_area(X_NOZZLE_PARK_POINT, Y_NOZZLE_PARK_POINT));
        return apply_nozzle_cleaner_offset({ X_NOZZLE_PARK_POINT, Y_NOZZLE_PARK_POINT, mapi::ParkingPosition::AtLeast { .above_print = Z_NOZZLE_PARK_POINT } });
#else
        return ParkingPosition(XYZ_NOZZLE_PARK_POINT);
#endif
    case ParkPosition::purge: {
#if HAS_WASTEBIN()
    #if HAS_INDX()
        // Wastebin is fixed to the CoreXY gantry, Z does not matter
        static constexpr ParkingPosition base_pos { X_WASTEBIN_POINT, Y_WASTEBIN_POINT, mapi::ParkingPosition::AtLeast { .above_print = 2 } };
        static_assert(is_in_wastebin_area(X_WASTEBIN_POINT, Y_WASTEBIN_POINT));
        return apply_nozzle_cleaner_offset(base_pos);

    #elif PRINTER_IS_PRUSA_iX()
        // Wastebin is fixed to the CoreXY gantry, Z does not matter
        return ParkingPosition { X_WASTEBIN_POINT, Y_WASTEBIN_POINT, mapi::ParkingPosition::AtLeast { .above_print = 2 } };

    #else
        #error Need to define wastebin parking position
    #endif
#else
        return ParkingPosition { X_AXIS_LOAD_POS, Y_AXIS_LOAD_POS, ParkingPosition::AtLeast { .above_print = Z_NOZZLE_PARK_RISE, .absolute = Z_AXIS_LOAD_POS } };
#endif
    }

    case ParkPosition::load: {
        static constexpr ParkingPosition base_pos { X_AXIS_LOAD_POS, Y_AXIS_LOAD_POS, ParkingPosition::AtLeast { .above_print = Z_NOZZLE_PARK_RISE, .absolute = Z_AXIS_LOAD_POS } };
#if HAS_INDX()
        // The load and unload positions coincide with the park and purge points over the wastebin,
        // so they need the calibrated nozzle cleaner offsets applied as well
        static_assert(is_in_wastebin_area(base_pos));
        return apply_nozzle_cleaner_offset(base_pos);
#else
        return base_pos;
#endif
    }

    case ParkPosition::unload: {
        static constexpr ParkingPosition base_pos { X_AXIS_UNLOAD_POS, Y_AXIS_UNLOAD_POS, ParkingPosition::AtLeast { .above_print = Z_NOZZLE_PARK_RISE, .absolute = Z_AXIS_UNLOAD_POS } };
#if HAS_INDX()
        static_assert(is_in_wastebin_area(base_pos));
        return apply_nozzle_cleaner_offset(base_pos);
#else
        return base_pos;
#endif
    }

    case ParkPosition::loadcell_selftest:
        return ParkingPosition(XYZ_LOADCELL_SELFTEST_POINT);

    case ParkPosition::filament_change: {
        static constexpr xyz_pos_t filament_change_point { { XYZ_NOZZLE_PARK_POINT_M600 } };
        static constexpr ParkingPosition base_pos { filament_change_point.x, filament_change_point.y, mapi::ParkingPosition::AtLeast { .above_print = Z_NOZZLE_PARK_RISE_M600, .absolute = filament_change_point.z } };
#if HAS_INDX()
        static_assert(is_in_wastebin_area(filament_change_point.x, filament_change_point.y));
        return apply_nozzle_cleaner_offset(base_pos);
#else
        return base_pos;
#endif
    }

    case ParkPosition::nozzle_cleaning_failed: {
        static constexpr xyz_pos_t cleaning_failed_point { { XYZ_NOZZLE_CLEANINIG_FAILED_POINT } };
        static constexpr ParkingPosition base_pos { cleaning_failed_point.x, cleaning_failed_point.y, cleaning_failed_point.z };
#if HAS_INDX()
        static_assert(is_in_wastebin_area(cleaning_failed_point.x, cleaning_failed_point.y));
        return apply_nozzle_cleaner_offset(base_pos);
#else
        return base_pos;
#endif
    }
    case ParkPosition::print_end: {
        static constexpr xyz_pos_t print_end_point { { XYZ_NOZZLE_PARK_POINT_ON_PRINT_END } };
        static constexpr ParkingPosition base_pos { print_end_point.x, print_end_point.y, print_end_point.z };
#if HAS_INDX()
        // The print end park position is over the wastebin, which needs the calibrated nozzle cleaner offsets applied
        static_assert(is_in_wastebin_area(print_end_point.x, print_end_point.y));
        return apply_nozzle_cleaner_offset(base_pos);
#else
        return base_pos;
#endif
    }

#if HAS_WASTEBIN_FILL_TRACKING()
    case ParkPosition::empty_wastebin:
        // Park at the INDX home corner (X to the min endstop, Y all the way back), clear of the
        // cleaner which sits at high X, and drop the bed near its lowest (Z within 50 of Z_MAX, or
        // 20 above tall prints) so there is room to reach in and empty the cleaner.
        return ParkingPosition { 0.0f, static_cast<float>(Y_MAX_POS), ParkingPosition::AtLeast { .above_print = 20, .absolute = Z_MAX_POS - 50 } };
#endif

#if HAS_NOZZLE_CLEANER() || HAS_NOZZLE_CLEANER_LITE()
    case ParkPosition::nozzle_cleaner_approach:
    #if HAS_INDX()
        return ParkingPosition { .x = X_WASTEBIN_SAFE_POINT, .y = Y_BRUSH_AVOID_POINT, .z = ParkingPosition::AtLeast { .above_print = 3.0f } };
    #else
        // No caller reaches these positions on other variants yet.
        // Fill in the coordinates when one appears.
        return {};
    #endif

    case ParkPosition::nozzle_cleaner_exit:
    #if HAS_INDX()
        return ParkingPosition { .x = X_WASTEBIN_SAFE_POINT, .y = Y_WASTEBIN_SAFE_POINT, .z = ParkingPosition::AtLeast { .above_print = 3.0f } };
    #else
        return {};
    #endif
#endif

#if HAS_TOOLCHANGER()
    case ParkPosition::tool_park:
        if (auto *t = std::get_if<PhysicalToolIndex>(&tool)) {
            const auto xy = prusa_toolchanger.tool_park_position(*t);
            return { .x = xy.x, .y = xy.y, .z = mapi::ParkingPosition::AtLeast { .above_print = 2 } };
        } else {
            // User-reachable, don't do a BSOD
            return {};
        }

#endif

    case ParkPosition::_cnt:
        bsod_unreachable();
    }
    bsod_unreachable();
}

xyz_bool_t ParkingPosition::axes_needing_homing() const {
    // We want XY homing requirements to be joined, mainly bcs of pre_park_move_pattern
    const bool do_axis_xy = std::holds_alternative<float>(x) || std::holds_alternative<float>(y);
    return xyz_bool_t {
        .x = do_axis_xy,
        .y = do_axis_xy,
        .z = std::holds_alternative<float>(z),
    };
}

xyz_pos_t ParkingPosition::to_xyz_pos(const xyz_pos_t &pos) const {
    const auto result = to_nan_xyz_pos(pos);
    if (std::isnan(result.x) || std::isnan(result.y) || std::isnan(result.z)) {
        bsod("Conversion to xyz_pos_t failed.");
    }
    return result;
}

float ParkingPosition::resolve_z(float reference_z) const {
    return match(
        z, //
        [&](Unchanged) { return reference_z; }, //
        [](float val) { return val; }, //
        [&](Relative arg) {
            return std::clamp<float>(reference_z + arg.delta, Z_MIN_POS, Z_MAX_POS);
        }, //
        [&](AtLeast arg) {
            float min_z = arg.absolute;

            if (!std::isnan(arg.above_print)) {
                min_z = std::max(min_z, planner.max_printed_z + arg.above_print);
            }

            // Don't let Z go down
            return std::clamp<float>(reference_z, min_z, Z_MAX_POS);
        } //
    );
}

xyz_pos_t ParkingPosition::to_nan_xyz_pos(const xyz_pos_t &pos) const {
    return {
        .x = std::holds_alternative<Unchanged>(x) ? pos.x : std::get<float>(x),
        .y = std::holds_alternative<Unchanged>(y) ? pos.y : std::get<float>(y),
        .z = resolve_z(pos.z),
    };
}

ParkingPosition ParkingPosition::from_xyz_pos(const xyz_pos_t &pos) {
    return ParkingPosition {
        std::isnan(pos.x) ? (X)unchanged : pos.x,
        std::isnan(pos.y) ? (Y)unchanged : pos.y,
        std::isnan(pos.z) ? (Z)unchanged : pos.z,
    };
}

ParkingPosition ParkingPosition::from_xy_relative_z_pos(const xyz_pos_t &pos) {
    return ParkingPosition {
        std::isnan(pos.x) ? (X)unchanged : pos.x,
        std::isnan(pos.y) ? (Y)unchanged : pos.y,
        std::isnan(pos.z) ? (Z)unchanged : Relative { pos.z },
    };
}

#if HAS_INDX()
/// Applies nozzle cleaner origin offsets (from calibration) to the given parking position's X and Y.
ParkingPosition apply_nozzle_cleaner_offset(const ParkingPosition &position) {
    const float x_offset = config_store().nozzle_cleaner_x_origin_offset.get();
    const float y_offset = config_store().nozzle_cleaner_y_origin_offset.get();

    ParkingPosition result = position;
    if (auto *x = std::get_if<float>(&result.x)) {
        *x += x_offset;
    }
    if (auto *y = std::get_if<float>(&result.y)) {
        *y += y_offset;
    }
    return result;
}
#endif

namespace {

    class ParkingExecutor {

    public:
        static constexpr feedRate_t fr_xy = NOZZLE_PARK_XY_FEEDRATE;
        static constexpr feedRate_t fr_z = NOZZLE_PARK_Z_FEEDRATE;

        ParkingExecutor(const ParkArgs &args, const float destination_z)
            : args_(args)
            , destination_z_(destination_z) {
            remaining_distance_to_retract_mm_ = args.retract_distance_mm;
            debug_assert(remaining_distance_to_retract_mm_ >= 0);
        }

    public:
        /// Can apply retraction as well, depending on the PrintArgs
        void move(const xyz_pos_t &target, feedRate_t fr_mm_s) {
            static constexpr PrepareMoveHints hints {
                .scale_feedrate = false,
                .move {
                    .ignore_e_factor = true,
                }
            };

            const xyz_pos_t delta_xyz = target - current_position.xyz();
            const float delta_distance_mm = delta_xyz.magnitude();

            xyze_pos_t target_xyze = current_position;

            // Note: This is disregarding accelerations. Considering that would be too complicated.
            if (remaining_distance_to_retract_mm_ > 0 && delta_distance_mm > 0) {

                /// Distance we would be able to retract during the full move considering the feedrates
                const float maximum_move_retraction_mm = delta_distance_mm / fr_mm_s * args_.evaluate_feedrate();

                const float distance_to_retract_mm = std::min(maximum_move_retraction_mm, remaining_distance_to_retract_mm_);
                remaining_distance_to_retract_mm_ -= distance_to_retract_mm;

                // If the retraction distance would be reached before reaching target XYZ, split the move into two segments:
                // - First segment with parallel retraction
                // - Second, move-only segment after we've got remaining_distance_to_retract_mm_ to 0, implemented after this if

                target_xyze.set(current_position.xyz() + delta_xyz * (distance_to_retract_mm / maximum_move_retraction_mm));
                target_xyze.e -= distance_to_retract_mm;
                prepare_move_to(target_xyze, fr_mm_s, hints);
            }

            // This will be a nop if the move with retraction reached the target XYZ already
            target_xyze.set(target);
            prepare_move_to(target_xyze, fr_mm_s, hints);
        }

        /// Can apply retraction and Z ramp as well, depending on the PrintArgs
        void move_x(float target) {
            move_xy({ target, current_position.y });
        }

        /// Can apply retraction and Z ramp as well, depending on the PrintArgs
        void move_y(float target) {
            move_xy({ current_position.x, target });
        }

        /// Can apply retraction and Z ramp as well, depending on the PrintArgs
        void move_xy(const xy_pos_t &target) {
            if (args_.z_ramp_slope > 0) {
                // Do a parallel Z move with the XY move with a CONSTANT SLOPE.
                //
                // The Z ramp is intended to be SPREAD across all XY moves during the park,
                // we don't have to reach the `destination_z_` at the end of this function.
                //
                // If the destination_z is not reached even after all XY moves,
                // things will get settled by the final Z park move.

                const float dest_delta_z = destination_z_ - current_position.z;
                const xy_pos_t full_delta_xy = target - current_position.xy();

                /// Delta Z we would be able to do according to the z_ramp_slope, disregarding destination Z overshoot
                const float full_delta_z = std::copysignf(full_delta_xy.magnitude() * args_.z_ramp_slope, dest_delta_z);

                if (full_delta_z != 0) {
                    // If we would reach target Z before reaching target xy, the move will get split into two segments:
                    // - Segment with the parallel Z move to reach the dest Z
                    // - Second XY-only segment, implemented after this if

                    /// Delta XYZ we would be able to do according to the z_ramp_slope, disregarding destination Z overshoot
                    const xyz_pos_t full_delta_xyz { full_delta_xy.x, full_delta_xy.y, full_delta_z };

                    // Compute how much of full_delta_xyz we can do before overshooting target Z
                    const float segment_coef = std::min<float>(std::abs(dest_delta_z / full_delta_z), 1);

                    move(current_position.xyz() + full_delta_xyz * segment_coef, fr_xy);
                }
            }

            // This will be a nop if the Z ramp move reached the target XY
            move(xyz_pos_t { target.x, target.y, current_position.z }, fr_xy);
        }

        /// Can apply retraction as well, depending on the PrintArgs
        void move_z(float target) {
            auto target_pos = current_position.xyz();
            target_pos.z = target;
            move(target_pos, fr_z);
        }

        /// Retracts the remaining distance, in case the retraction was not fully done during the standard moves
        void finalize_retraction() {
            if (remaining_distance_to_retract_mm_ > 0) {
                mapi::extruder_move(-remaining_distance_to_retract_mm_, args_.evaluate_feedrate());
            }
        }

    public:
#if HAS_NOZZLE_CLEANER()
        void pre_park_move_pattern(const xy_pos_t &destination);
#endif

    private:
        const ParkArgs &args_;
        const float destination_z_;

        /// Distance to retract over the whole parking procedure
        float remaining_distance_to_retract_mm_;
    };

} // namespace

#if HAS_NOZZLE_CLEANER()
/**
 * Does the extra parking moves except the last one to move in the correct
 * directions and avoid the nozzle cleaner.
 *
 *  RearArea = WasteArea + NoWasteLand         │ X_NOZZLE_PARK_POINT
 *                                             ▼
 * ┌────────────────NO─WASTE─LAND────────────────┬─────────WASTE─AREA─────────┐
 * │                                         .  .│                            │
 * │                                         .  .│                        * ◄─┼────X,Y_WASTEBIN_POINT
 * │                                         .  .├ ─ ─ ─ ─ ─ ─ ┐              │
 * │                                         .  .│    ╲‾‾╲     |   ╱‾‾╱=>     │
 * │                                         ....│      ╲  ╲   |  |  |        │
 * │                                             │        ╲__╲ │   ╲ˍˍ╲=>     │
 * │                                             │    BRUSH    │              │
 * ├─────────────────────────────────────────────┴─────────────┴──────────────┤ ◄── Y_WASTEBIN_SAFE_POINT
 * │                                             X_BRUSH_POINT ▲              |
 * │   _____  _____  _____ _   _ _______            _____  ______             │
 * │  |  __ \|  __ \|_   _| \ | |__   __|     /\   |  __ \|  ____|   /\       │
 * │  | |__) | |__) | | | |  \| |  | |       /  \  | |__) | |__     /  \      │
 * │  |  ___/|  _  /  | | | . ` |  | |      / /\ \ |  _  /|  __|   / /\ \     │
 * │  | |    | | \ \ _| |_| |\  |  | |     / ____ \| | \ \| |____ / ____ \    │        ▲
 * │  |_|    |_|  \_\_____|_| \_|  |_|    /_/    \_\_|  \_\______/_/    \_\   │        | Y
 * │                                                                          │        |
 * │                                                                          │        |      X
 * └──────────────────────────────────────────────────────────────────────────┘        O ──────►
 */
void ParkingExecutor::pre_park_move_pattern(const xy_pos_t &destination) {
    #if PRINTER_IS_PRUSA_iX()
    static constexpr float x_border_point = X_NOZZLE_PARK_POINT + 1;
    static constexpr float X_BRUSH_POINT = 242.f; // X point before which we can go directly from (no need to avoid v-blade)

    const bool start_in_rear_area = current_position.y > Y_WASTEBIN_SAFE_POINT;
    const bool destination_in_rear_area = destination.y > Y_WASTEBIN_SAFE_POINT;

    const bool start_in_wastebin_area = start_in_rear_area && current_position.x > x_border_point;
    const bool destination_in_wastebin_area = destination_in_rear_area && destination.x > x_border_point;

    const bool start_in_brush_area = start_in_wastebin_area && current_position.x < X_BRUSH_POINT;
    const bool destination_in_brush_area = destination_in_wastebin_area && destination.x < X_BRUSH_POINT;

    if (start_in_rear_area != destination_in_rear_area) { // One in the rear, other in print area
        if (start_in_wastebin_area || destination_in_wastebin_area) { // One in waste area, other in print area
            if (!start_in_brush_area && !destination_in_brush_area) { // If neither is in brush area, go around vblade
                move_x(X_WASTEBIN_POINT);
            } // If we are in brush area, we can go directly
            move_y(destination.y);
        } else { // One in no-waste land, other in print area
            if (start_in_rear_area) { // Start in no-waste land, end in print area
                move_y(destination.y);
            } else { // Start in print area, end in no-waste land
                move_x(destination.x);
            }
        }
    } else if (start_in_rear_area) { // Both in the rear area
        if (start_in_wastebin_area != destination_in_wastebin_area) { // One in waste area, other in no-waste land
            if (start_in_wastebin_area) { // Start in waste area, end in no-waste land
                if (!start_in_brush_area) { // If we don't start in brush, avoid v-blade
                    move_x(X_WASTEBIN_POINT);
                }
                move_y(Y_WASTEBIN_SAFE_POINT);
                move_x(destination.x);
            } else { // Start in no-waste land, end in waste area
                if (!destination_in_brush_area) { // If we don't end in brush, avoid v-blade
                    move_x(X_WASTEBIN_POINT);
                }
                move_x(X_WASTEBIN_POINT);
                move_y(destination.y);
            }
        } else if (start_in_wastebin_area) { // Both in waste area (here no need to worry about brush)
            move_x(destination.x);
        } else { // Both in no-waste land
            move_y(Y_WASTEBIN_SAFE_POINT);
            move_x(destination.x);
        }
    } // Both in print area, no need to pre-park move
    #elif HAS_INDX()
    const bool start_in_side_area = current_position.x > X_WASTEBIN_SAFE_POINT;
    const bool destination_in_side_area = destination.x > X_WASTEBIN_SAFE_POINT;

    const bool start_in_wastebin_area = start_in_side_area && current_position.y > Y_WASTEBIN_SAFE_POINT;
    const bool destination_in_wastebin_area = destination_in_side_area && destination.y > Y_WASTEBIN_SAFE_POINT;

    if (start_in_side_area != destination_in_side_area) { // One in the side, other in print area
        if (start_in_wastebin_area || destination_in_wastebin_area) { // One in waste area, other in print area
            move_y(Y_BRUSH_AVOID_POINT);
            move_x(destination.x);
        } else { // One in no-waste land, other in print area
            if (start_in_side_area) { // Start in no-waste land, end in print area
                move_x(X_WASTEBIN_SAFE_POINT);
            } else { // Start in print area, end in no-waste land
                move_xy(xy_pos_t { .x = X_WASTEBIN_SAFE_POINT, .y = destination.y });
            }
        }
    } else if (start_in_side_area) { // Both in the side area
        if (start_in_wastebin_area != destination_in_wastebin_area) { // One in waste area, other in no-waste land
            if (start_in_wastebin_area) { // Start in waste area, end in no-waste land
                move_y(Y_BRUSH_AVOID_POINT);
                move_x(X_WASTEBIN_SAFE_POINT);
                move_y(destination.y);
            } else { // Start in no-waste land, end in waste area
                move_x(X_WASTEBIN_SAFE_POINT);
                move_y(Y_BRUSH_AVOID_POINT);
                move_x(destination.x);
            }
        }
    } // Both in print area, no need to pre-park move
    #else
        #error "Implement"
    #endif
}

void move_out_of_nozzle_cleaner_area() {
    static constexpr ParkArgs args {};
    ParkingExecutor p(args, current_position.z);

    #if PRINTER_IS_PRUSA_iX()
    p.pre_park_move_pattern(xy_pos_t { X_WASTEBIN_POINT, Y_WASTEBIN_SAFE_POINT });
    #else
    p.pre_park_move_pattern(xy_pos_t { X_WASTEBIN_SAFE_POINT, Y_BRUSH_AVOID_POINT });
    #endif
}
#endif

bool park(const ParkingPosition &parking_position, const ParkArgs &args) {
    const auto park_destination = parking_position.to_xyz_pos(current_position.xyz());
    const auto needs_homed = parking_position.axes_needing_homing();

    ParkingExecutor p(args, park_destination.z);

    // @returns false if the move cannot be performed safely (the caller aborts)
    const auto move_z = [&]() -> bool {
        if (park_destination.z == current_position.z) {
            return true;
        }
        if (axes_home_level.is_homed(Z_AXIS, AxisHomeLevel::imprecise)) {
            p.move_z(park_destination.z);

        } else if (needs_homed.z) {
            // The move requires homed Z axis, which we don't have
            // Don't allow XY moves without lifting
            return false;

        } else {
            do_homing_move(Z_AXIS, park_destination.z - current_position.z, HOMING_FEEDRATE_INVERTED_Z);
        }
        return true;
    };

    const auto move_xy = [&]() -> bool {
        if (park_destination.xy() == current_position.xy()) {
            return true;
        }
        if (!axes_home_level.is_homed({ X_AXIS, Y_AXIS }, AxisHomeLevel::imprecise)) {
            // We need to always be homed to do XY moves
            return false;
        }

#if HAS_INDX()
        // move out of dock area perpendicularly before parking
        if (current_position.y < Y_DOCK_PARKING_MIN_SAFE_POS) {
            p.move_y(Y_DOCK_PARKING_MIN_SAFE_POS);
        }
#endif

#if HAS_NOZZLE_CLEANER()
        p.pre_park_move_pattern(park_destination.xy());
#endif
        p.move_xy(park_destination.xy());
        return true;
    };

    const bool move_xy_first =
        // Order the two moves so the nozzle is never dragged across the print: the XY traverse always
        // happens at the higher of the current/destination Z. Going up: lift first, then traverse.
        // Going down: traverse first (at the current, higher Z), then descend. Each move handles its own
        // homing requirements internally (and bails out if unsatisfied).
        (park_destination.z < current_position.z)

        // Specifying parallel_z_move_angle_tan makes the Z moves done in parallel to the XY moves - handled in the move machinery
        || (args.z_ramp_slope > 0);

    if (move_xy_first && !move_xy()) {
        return false;
    }

    if (!move_z()) {
        return false;
    }

    if (!move_xy_first && !move_xy()) {
        return false;
    }

    p.finalize_retraction();

    planner.synchronize();
    report_current_position();
    return !planner.draining();
}

void home_if_needed_and_park(const ParkingPosition &parking_position, const ParkArgs &args) {
    // We only need homing if we move to absolute coordinates
    // For relative moves we can use do_homing_move
    const xyz_bool_t do_axis = parking_position.axes_needing_homing();
    GcodeSuite::G28_no_parser(do_axis.x, do_axis.y, do_axis.z,
        {
            .only_if_needed = true,
            .z_raise = 3,
            .precise = false, // We don't need precise position for parking
        });

    park(parking_position, args);
}

} // namespace mapi
