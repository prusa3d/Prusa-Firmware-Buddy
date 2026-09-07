#pragma once

#include <core/types.h>
#include <variant>

#include <option/has_nozzle_cleaner.h>
#include <option/has_nozzle_cleaner_lite.h>
#include <option/has_wastebin.h>
#include <option/has_wastebin_fill_tracking.h>
#include <option/has_indx.h>
#include <bsod/bsod.h>
#include <tool_index.hpp>
#include <utils/compact_optional.hpp>
#include <mapi/feedrates/standard_feedrates.hpp>

namespace mapi {

enum class ParkPosition : uint8_t {
    park,

    /// Position where it's safe to purge
    /// If the printer has a wastebin, it will be over the wastebin
    /// Otherwise some generic location one can poop from
    purge,

    load,
    unload,
    loadcell_selftest,

    /// Default position for filament change (M600, M1601, MMU errors)
    filament_change,

    /// User-accessible position for manual nozzle cleaning after the automatic
    /// nozzle cleaning failed. Uses the M600 position - while we are not changing
    /// filament, the nozzle parks at an accessible place to have it cleaned.
    nozzle_cleaning_failed,
    /// Position to park the head at when the print finishes
    print_end,

#if HAS_WASTEBIN_FILL_TRACKING()
    /// Head clear of the nozzle cleaner so the wastebin can be pulled out, lifted above the print.
    empty_wastebin,
#endif

#if HAS_NOZZLE_CLEANER() || HAS_NOZZLE_CLEANER_LITE()
    /// Just outside the cleaner area, staging the tool for a cleaning sequence.
    /// Does not define Z; set it explicitly if a Z move is needed.
    nozzle_cleaner_approach,

    /// Clear of the cleaner area, for parking after a cleaning sequence.
    /// Does not define Z; set it explicitly if a Z move is needed.
    nozzle_cleaner_exit,
#endif

#if HAS_TOOLCHANGER()
    /// Move the head to a position for tool docking - typically just before the dock
    /// !!! The tool MUST be provided in get_parking_position args
    tool_park,
#endif

    _cnt,
};

/// Describes a position, or rather behavior for parking
/// The behavior might also be for example a relative Z lift, or some axes might not be moved at all
struct ParkingPosition {
    // special marker indicating "leave the synchronized xyz_pos_t on that axis as is"
    struct Unchanged {
        constexpr bool operator==(const Unchanged &) const = default;
    };

    /// Moves relatively to the current position, clamped to the machine limits
    struct Relative {
        float delta;

        constexpr bool operator==(const Relative &) const = default;
    };

    /// Parks the Z axis at the specified minimum distance
    struct AtLeast {
        /// A little trick to prevent users from doing AtLeast { 5.5f }
        /// Encourages using aggregate initializer AtLeast { .above_print = ... }
        [[no_unique_address]] std::monostate _use_aggregatee_initizalizer {};

        /// Don't park lower than this distance above print (planner.max_printed_z) if specified
        float above_print = NAN;

        /// Don't park lower than this absolute position if specified
        float absolute = 0;

        constexpr bool operator==(const AtLeast &) const = default;
    };

    static constexpr Unchanged unchanged {};

    using X = std::variant<Unchanged, float>;
    using Y = std::variant<Unchanged, float>;
    using Z = std::variant<Unchanged, float, Relative, AtLeast>;

    // float = absolute coordinate
    X x = unchanged;
    Y y = unchanged;
    Z z = unchanged;

    constexpr bool operator==(const ParkingPosition &) const = default;

    /// @returns a vector of which axes need to be homed for the parking to the position to be realizable
    xyz_bool_t axes_needing_homing() const;

    /// Resolves the Z component against reference_z (the current Z): Unchanged yields
    /// reference_z, an absolute value yields itself, Relative offsets reference_z and
    /// AtLeast raises it to a floor; Relative/AtLeast are clamped to Z_MAX_POS.
    float resolve_z(float reference_z) const;

    // Synchronizes this provided position and provides appropriate xyz_pos_t
    xyz_pos_t to_xyz_pos(const xyz_pos_t &pos) const;

    // Do not use if not necessary! This method currently works as a
    // bridge between unrefactored parts still using xyz_pos_t
    // Should not be needed upon more refactoring
    [[deprecated("Construct the ParkingPosition properly")]]
    xyz_pos_t to_nan_xyz_pos(const xyz_pos_t &pos = { NAN, NAN, NAN }) const;

    [[deprecated("Construct the ParkingPosition properly")]]
    static ParkingPosition from_xyz_pos(const xyz_pos_t &pos);

    [[deprecated("Construct the ParkingPosition properly")]]
    static ParkingPosition from_xy_relative_z_pos(const xyz_pos_t &pos);

    /// @returns a modified parking position with Z not moving at all
    [[deprecated("Construct the ParkingPosition properly")]]
    ParkingPosition without_z_move() const {
        auto result = *this;
        result.z = unchanged;
        return result;
    }
};

ParkingPosition get_parking_position(ParkPosition position, std::variant<PhysicalToolIndex, NoTool> tool = NoTool {});

#if HAS_NOZZLE_CLEANER()
void move_out_of_nozzle_cleaner_area();

    #if HAS_INDX()
/// Applies nozzle cleaner origin offsets (from calibration) to the given parking position's X and Y.
ParkingPosition apply_nozzle_cleaner_offset(const ParkingPosition &position);
    #endif

#endif

struct ParkArgs {
    static const ParkArgs default_args;

    /// Distance to retract during parking - retraction is done in parallel with the parking moves
    float retract_distance_mm = 0;

    /// Feedrate of the retraction
    CompactOptional<float, NAN> retract_fr_mm_s = {};

    /// If > 0, the Z moves are done in parallel to the XY moves
    /// with an angle `z_ramp_slope = tan(angle)` (1 → 45°) respective to the XY moves
    /// until the target Z position is reached (then only the XY move continues).
    /// If the XY moves with the slope are not enough to reach the destination Z,
    /// a final Z-only move is done at the end.
    /// !!! Warning - this bypasses the Z move prevention when Z is unhomed
    float z_ramp_slope = 0;

    /// @brief Gives retract feedrate, handles if stored feedrate is NAN
    /// @return (mm/s) stored feedrate or standard feedrate adjusted for current filament
    inline float evaluate_feedrate() const {
        return retract_fr_mm_s.has_value() ? retract_fr_mm_s.value() : buddy::standard_feedrates::current_extruder(buddy::standard_feedrates::Extruder::retract);
    }
};

/**
 * @brief Parks the toolhead at the specified position.
 *
 * Moves the toolhead to a parking position, optionally adjusting the Z axis first.
 * On printers with a nozzle cleaner, the function automatically performs intermediate
 * moves to avoid collisions with the brush/v-blade area.
 *
 * Can only execute part of parking move if axes are not homed
 *
 * @returns if the whole intended move was executed
 */
bool park(const ParkingPosition &parking_position = get_parking_position(ParkPosition::park), const ParkArgs &args = ParkArgs::default_args);

/**
 * @brief Homes required axes if needed, then parks the toolhead.
 *
 * Same as park(), but performs homing first on axes that will need it
 */
void home_if_needed_and_park(const ParkingPosition &parking_position = get_parking_position(ParkPosition::park), const ParkArgs &args = ParkArgs::default_args);

} // namespace mapi
