/// @file
#pragma once

#include <atomic>
#include <stdint.h>

/// HT (PT1000) hotend nozzle-temperature ceiling. A compile-time constant (not the runtime
/// Hotend::max_nozzle_temp()) so it can gate a static_assert on the HT filament presets.
inline constexpr int16_t ht_hotend_max_nozzle_temp = 415;

/// Boot-time hotend detection may request a user confirmation dialog.
///
/// Thread sync: written from the main thread by PhysicalTool::for_index() during early
/// boot (before the FreeRTOS scheduler starts) and read from the GUI task by ScreenSplash
/// afterwards. The scheduler-start barrier already orders the write before the read;
/// std::atomic makes the cross-thread contract visible at the declaration per the project's
/// 'shared variables are visibly atomic or guarded' rule.
/// Only used on printers with HT hotend support (COREONE, COREONEL).
inline std::atomic<bool> hotend_detect_dialog_pending { false };

namespace hotend_detect {

/// 10-bit ADC thresholds derived from the PT1000 temptable.
/// Outside the [lower, upper] band → the reading is unambiguously NTC.
/// Inside the band → ambiguous (could be a hot NTC or a cold-to-hot PT1000).
struct Thresholds {
    uint16_t clear_ntc_upper; ///< ADC reading above this is unambiguously NTC (cold)
    uint16_t clear_ntc_lower; ///< ADC reading below this is unambiguously NTC (hot)
};

struct Inputs {
    uint16_t adc_10bit;
    bool stored_is_high_temp; ///< whether the persisted hotend is the HT (PT1000) one
};

struct Outputs {
    bool is_high_temp; ///< the detected/selected hotend is the HT (PT1000) one
    bool dialog_pending; ///< boot must ask the user to confirm the hotend
};

/// Pure hotend-sensor classification — testable without firmware dependencies.
/// A clear-NTC reading is authoritative (select standard, no dialog); an ambiguous one (a hot
/// NTC and a cold-to-hot PT1000 read alike) trusts the persisted hotend and asks only when it
/// isn't yet HT. The clear-NTC branch must NOT raise the dialog even against a stored HT, or boot
/// loops: the caller overwrites the stored hotend, the dialog flips it back, the next boot re-asks.
constexpr Outputs detect_impl(Inputs in, Thresholds t) {
    const bool clear_ntc = (in.adc_10bit > t.clear_ntc_upper) || (in.adc_10bit < t.clear_ntc_lower);
    if (clear_ntc) {
        return { false, false };
    }
    return { in.stored_is_high_temp, !in.stored_is_high_temp };
}

} // namespace hotend_detect
