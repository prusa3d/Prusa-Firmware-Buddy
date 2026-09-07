#pragma once

#include <cmath>
#include <optional>

namespace nozzle_cleaner_lite {

/// The cleaner pad's usable contact patch, in the pad's own coordinates: 'along'
/// runs down its length, 'across' spans its width. Which machine axis each of
/// them maps to differs per printer, so only the caller needs to care.
///
/// The nozzle wipes back and forth between along_near and along_far while
/// drifting between across_min and across_max, tracing a shallow zigzag instead
/// of wearing one groove:
///
///   across_max  +----/\------/\------/\----+
///               |   /  \    /  \    /  \   |   <- the nozzle's path
///   across_min  +--/----\--/----\--/----\--+
///               ^                          ^
///          along_near                  along_far
///          (the nozzle arrives from the touchpoint on this side)
struct Pad {
    /// End of the length the nozzle arrives at. May be greater than \p along_far -
    /// the pad does not have to run towards growing machine coordinates.
    float along_near = 0.0f;
    /// The opposite end of the length
    float along_far = 0.0f;

    float across_min = 0.0f;
    float across_max = 0.0f;

    constexpr float length() const { return std::abs(along_far - along_near); }
    constexpr float width() const { return across_max - across_min; }
};

/// Generates the zigzag path over a Pad. The drift turns around only at the
/// pad's edges, so reversing a wipe does not disturb it.
///
/// Produces the path as waypoints and leaves the motion to the caller, so it
/// knows nothing about machine axes, feedrates or the planner.
class ZigZag {
public:
    struct Waypoint {
        /// Position between the pad's along_near and along_far
        float along = 0.0f;
        /// Position within the pad's across band
        float across = 0.0f;

        constexpr bool operator==(const Waypoint &) const = default;
    };

    /// \param crossings_per_stroke across_min <-> across_max crossings per
    ///        full-length stroke. A whole number keeps every stroke on the same
    ///        diagonals; a fractional one makes consecutive strokes differ. Must
    ///        be greater than zero.
    /// \param start_phase where in the pattern to start, as a fraction of a
    ///        crossing there and back: 0 starts at across_min heading towards
    ///        across_max, 0.5 starts at across_max. Wrapped into [0, 1).
    ZigZag(const Pad &pad, float crossings_per_stroke, float start_phase);

    /// Where the nozzle has to be before the first stroke.
    Waypoint start() const;

    /// Begin a stroke ending at \p along_to, from wherever the previous one ended
    /// (\ref start() for the first). Discards any unfinished stroke.
    void begin_stroke(float along_to);

    /// \returns the next waypoint of the stroke begun by \ref begin_stroke, the
    /// last one landing exactly on its \p along_to; nullopt once it is complete.
    std::optional<Waypoint> next();

private:
    /// Across coordinate after \p wiped of wiping along the pad
    float across_at(float wiped) const;

    Pad pad_;
    /// Wiped distance of one across_min <-> across_max crossing
    float half_period_;

    /// Along coordinate the current stroke starts from
    float along_;
    /// Distance wiped before the current stroke
    float wiped_;

    /// Along target of the stroke in progress, if any
    std::optional<float> stroke_to_;
    /// Value of wiped_ once the stroke in progress completes
    float stroke_end_ = 0.0f;
    /// Wiped distance at which the drift turns next
    float next_turn_ = 0.0f;
};

} // namespace nozzle_cleaner_lite
