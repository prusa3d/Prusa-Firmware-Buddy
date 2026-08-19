/// @file
#pragma once

#include <option/has_wastebin_fill_tracking.h>

#include <atomic>
#include <cstdint>
#include <optional>

// Compiled into the build only where the feature is enabled (added via CMake). A static_assert
// (instead of #if-ing the whole header) turns a stray include on a non-feature build into a hard
// error rather than silently nothing.
static_assert(HAS_WASTEBIN_FILL_TRACKING(), "wastebin_watcher.hpp included on a build without HAS_WASTEBIN_FILL_TRACKING()");

/**
 * Centralizes the INDX nozzle-cleaner wastebin fill policy: pellet accounting, mid-print full
 * detection (+ optional auto-pause), pre-print overfill projection and the per-print monitoring
 * state. The persistent pellet count lives in Odometer_s; this is the policy on top of it. Singleton.
 *
 * Thread-safety is marked per method below ("[Marlin thread only]" / "[Thread-safe]").
 */
class WastebinWatcher {
public:
    /// [Thread-safe] Singleton accessor.
    static WastebinWatcher &instance();

    /// [Marlin thread only] G12: account for one ejected blob (== one pellet). The moment the bin
    /// fills mid-print, either pauses to have it emptied or just warns, per the auto-pause setting.
    void account_ejected_pellet();

    /// [Marlin thread only] Park clear of the cleaner, block the gcode stream until the user
    /// confirms the bin is empty, then (mid-print) reheat over the cleaner and return.
    /// \p full also offers Ignore on the prompt (stop checking the bin for the rest of this print).
    void pause_to_empty(bool full);

    /// [Thread-safe] True if this print is likely to overfill the bin (current fill + this print's
    /// toolchanges exceed capacity). False when the gcode carries no toolchange count (older slicer).
    bool print_will_overfill() const;

    /// [Thread-safe] Reset the per-print toolchange progress. Called when a print starts.
    void reset_print_progress();

    /// [Thread-safe] Suppress (true) / re-enable (false) the mid-print full check for the current print.
    void set_ignored_for_print(bool ignored);

    /// [Thread-safe] The bin was emptied: reset the persistent pellet counter to zero. Called from
    /// the warning dispatch (marlin thread) when the user confirms.
    void mark_emptied();

    /// [Thread-safe] Pellet capacity of the installed cleaner (standard / extended), per the persisted setting.
    uint32_t capacity() const;
    /// [Thread-safe] Pellets in the bin since it was last emptied (current fill, not an action).
    uint32_t fill_level() const;
    /// [Thread-safe] Pellets still expected to be ejected in the rest of this print (one per
    /// remaining toolchange). std::nullopt if the gcode carries no toolchange count
    std::optional<uint32_t> expected_remaining_pellets() const;

private:
    WastebinWatcher() = default;

    /// Set by the pre-print / mid-print "Ignore": stop checking the bin for the rest of this print.
    std::atomic<bool> ignored_for_print_ = false;
    /// Pellets ejected during the current print (== toolchange progress). Reset at print start.
    std::atomic<uint32_t> pellets_this_print_ = 0;
};
