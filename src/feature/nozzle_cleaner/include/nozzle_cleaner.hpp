#pragma once

#include <gcode/inject_queue_actions.hpp>
#include <optional>
#include <string_view>
#include <str_utils.hpp>

#include <option/has_indx.h>
#include <option/has_nozzle_cleaner.h>

static_assert(HAS_NOZZLE_CLEANER(), "nozzle_cleaner.hpp included on a build without HAS_NOZZLE_CLEANER()");

#if HAS_INDX()
    // Nozzle cleaner geometry [mm], shared by the INDX variants. Anchored to the per-variant
    // X/Y_NOZZLE_CLEANER_ORIGIN from the printer configuration; expands at the point of use,
    // which must see the Marlin config.

    // Y calibration indent positions [mm] for the manual fallback (the two wastebin variants). Both bins
    // share the cleaner coordinate system; only where the manual Y indent sits differs. The standard
    // (longer) bin's indent is at the origin; the extended (shorter) bin's is 40 mm closer (+Y). The
    // matched point also selects the bin's capacity.
    #define Y_NOZZLE_CLEANER_CALIB_POINT_STANDARD Y_NOZZLE_CLEANER_ORIGIN
    #define Y_NOZZLE_CLEANER_CALIB_POINT_EXTENDED (Y_NOZZLE_CLEANER_ORIGIN + 40.f)

    // Anchor for the cleaner tray Y geometry; the wastebin point, tray back edge and entry derive from
    // it. INDX_TODO: tune.
    #define Y_NOZZLE_CLEANER_PURGE_CENTER_NOMINAL (Y_NOZZLE_CLEANER_ORIGIN + 90.f)

    #define X_WASTEBIN_SAFE_POINT (X_NOZZLE_CLEANER_ORIGIN - 10.35f)
    #define Y_WASTEBIN_SAFE_POINT (Y_NOZZLE_CLEANER_ORIGIN - 8.f)
    #define Y_BRUSH_AVOID_POINT   (Y_NOZZLE_CLEANER_ORIGIN + 101.f)

    #define X_WASTEBIN_POINT X_NOZZLE_CLEANER_ORIGIN
    #define Y_WASTEBIN_POINT (Y_NOZZLE_CLEANER_PURGE_CENTER_NOMINAL - 4.f) // derived from the tray anchor

    // Loadcell Y calibration touches the tray back edge (drive to the measured wall middle at
    // PURGE_ENTRY, move -Y); stored offset = contact - effective nozzle radius - BACK_NOMINAL.
    // BACK_NOMINAL is the physical edge face, radius-free.
    #define Y_NOZZLE_CLEANER_PURGE_BACK_NOMINAL (Y_NOZZLE_CLEANER_PURGE_CENTER_NOMINAL + 5.f)
    #define Y_NOZZLE_CLEANER_PURGE_PROBE_MIN    (Y_NOZZLE_CLEANER_PURGE_BACK_NOMINAL - 3.f) // probe ceiling past the edge
    // Entry sits clear of the edge by more than the offset tolerance so the X align move never bumps the
    // tray even on a max-tolerance +Y misaligned bin.
    #define Y_NOZZLE_CLEANER_PURGE_ENTRY (Y_NOZZLE_CLEANER_PURGE_BACK_NOMINAL + 4.f)

    // Loadcell X calibration touches the outer wall face (from WALL_ENTRY, move +X) and the inner face
    // (move -X from the V-groove lane at X_NOZZLE_CLEANER_ORIGIN, reached around the wall's +Y end via
    // the purge-entry lane). Wall middle = mean of the two contacts (the nozzle radius cancels out);
    // effective nozzle radius = (contact distance - THICKNESS) / 2. Stored offset = middle -
    // MIDDLE_NOMINAL; the face nominal is radius-free part geometry.
    #define X_NOZZLE_CLEANER_WALL_TOUCH_Y   (Y_NOZZLE_CLEANER_ORIGIN + 77.f)
    #define X_NOZZLE_CLEANER_WALL_ENTRY     (X_NOZZLE_CLEANER_ORIGIN - 12.f)
    #define X_NOZZLE_CLEANER_WALL_PROBE_MAX (X_NOZZLE_CLEANER_ORIGIN - 2.f)
    #define X_NOZZLE_CLEANER_WALL_THICKNESS 1.69f
    // V-groove center to wall middle; the stored offset's zero point. 6.03 = two-sided middle anchored
    // to a manual (V-groove homing) measurement, 1 C1L unit (shared part). INDX_TODO: refine on more
    // units.
    #define X_NOZZLE_CLEANER_WALL_MIDDLE_NOMINAL (X_NOZZLE_CLEANER_ORIGIN - 6.03f)
    // Derived; only estimates the outer contact before the two-sided measurement completes.
    #define X_NOZZLE_CLEANER_WALL_OUTER_FACE_NOMINAL (X_NOZZLE_CLEANER_WALL_MIDDLE_NOMINAL - X_NOZZLE_CLEANER_WALL_THICKNESS / 2.f)
#endif

namespace nozzle_cleaner {

enum class Sequence : uint16_t {
    clean,
#if HAS_INDX()
    quick_clean,
    deep_clean,
#endif
    purge_clean,
#if HAS_INDX()
    power_panic_purge,
    eject_blob,
    enter_cleaner,
    exit_cleaner,

    // Internal-only sequences below; not invocable via G12.
    _cnt_external,
    enter_cleaner_from_inside = _cnt_external,
#endif
    _cnt,
};

/// Count of sequences exposed via G12. Excludes internal-only sequences.
#if HAS_INDX()
inline constexpr size_t externally_invocable_count = static_cast<size_t>(Sequence::_cnt_external);
#else
inline constexpr size_t externally_invocable_count = static_cast<size_t>(Sequence::_cnt);
#endif

std::optional<Sequence> parse_sequence(std::string_view name);
const GCodeFile &get_sequence(Sequence seq);
void load_sequence(Sequence seq);

/// Load a sequence, wait for it to be ready, and execute it.
/// @return true on success, false if aborted (planner draining)
bool load_and_execute(Sequence seq);

bool is_loader_idle();
bool is_loader_buffering();

/**
 * @brief Executes the loaded nozzle cleaner gcode.
 * The load_sequence() function must be called before this function, and gcode loaded must be in ready state for this to work correctly.
 *
 * @return true if the gcode was executed successfully
 * @return false if still buffering, failed loading or not even loaded.
 */
bool execute();

void reset();

} // namespace nozzle_cleaner
