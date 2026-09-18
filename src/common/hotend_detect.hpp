/// @file
#pragma once

#include <atomic>
#include <stdint.h>

/// HT (PT1000) hotend nozzle-temperature ceiling. A compile-time constant (not the runtime
/// Hotend::max_nozzle_temp()) so it can gate a static_assert on the HT filament presets.
inline constexpr int16_t ht_hotend_max_nozzle_temp = 415;

/// Whether boot may still have to ask the user which hotend is installed.
///
/// Set from the early reading, cleared once the type is settled. The GUI queues the dialog on the
/// early value, so it must never be clear while a later step could still set it.
///
/// Thread sync: set during early boot, cleared when the type is settled, read on the GUI task.
/// std::atomic makes the cross-thread contract visible at the declaration per the project's
/// 'shared variables are visibly atomic or guarded' rule.
/// Only used on printers with HT hotend support (COREONE, COREONEL).
inline std::atomic<bool> hotend_detect_dialog_pending { false };

namespace hotend_detect {

/// Classify the nozzle ADC and publish hotend_detect_dialog_pending, without settling the type.
///
/// Call before the GUI builds its screen stack. Blocks for ~0.5 s while the reading has to hold
/// still.
void classify_early();

/// Settle the hotend type from the early classification alone, without probing.
///
/// For boot paths that stop at an error screen: they never heat, and probing there could turn a
/// hard fault into a boot loop.
void init_skipped();

/// Settle the hotend type, probing the nozzle first when the early reading was ambiguous.
///
/// Call from Marlin setup before the first Hotend consumer.
void resolve();

/// Release the heat-pulse boot-loop guard. Call from Marlin setup right after watchdog_init().
///
/// The pulse arms the IWDG, so everything until Marlin feeds it (settings.first_load() included)
/// must fit in one IWDG period. A reset in that stretch leaves the guard set and later boots
/// skip the pulse until a power cycle. Only the boot that started a pulse releases it.
void release_heat_pulse_guard();

/// What the print-fan phase concluded.
enum class FanProbeVerdict : uint8_t {
    stock_ntc, ///< the reading rose sharply: a hot NTC shedding heat
    warm_pt1000, ///< the nozzle is already warm, so a PT1000 -- do not heat it
    inconclusive, ///< nothing moved: a cold PT1000 or a dead sensor
};

struct FanProbeInputs {
    int32_t delta_adc_12bit; ///< end minus start, from averaged 12-bit samples
    uint16_t end_adc_10bit; ///< the end reading, for the absolute warm-nozzle interlock
};

struct FanProbeThresholds {
    int32_t rise_adc_12bit; ///< at or above this rise: a hot NTC
    uint16_t warm_nozzle_adc_10bit; ///< at or above this: warm PT1000, unsafe to add heat to
};

/// Classify the print-fan phase.
///
/// One-sided: cooling moves a hot NTC by tens of counts, a PT1000 by a few in either direction.
/// The warm-nozzle check is an absolute safety interlock (never heat a warm nozzle) and comes
/// second, so a hot NTC is caught by the rise first.
constexpr FanProbeVerdict classify_fan_probe(FanProbeInputs in, FanProbeThresholds t) {
    if (in.delta_adc_12bit >= t.rise_adc_12bit) {
        return FanProbeVerdict::stock_ntc;
    }
    if (in.end_adc_10bit >= t.warm_nozzle_adc_10bit) {
        return FanProbeVerdict::warm_pt1000;
    }
    return FanProbeVerdict::inconclusive;
}

/// What the heat-pulse phase concluded.
enum class HeatPulseVerdict : uint8_t {
    stock_ntc, ///< the reading fell hard: an NTC being heated
    high_temp_pt1000, ///< the reading rose: a PT1000 being heated
    sensor_fault, ///< neither: the probe is not reading the block at all
};

struct HeatPulseThresholds {
    int32_t move_adc_12bit; ///< a move of at least this, in either direction, is a real response
};

/// Classify the heat-pulse phase.
///
/// Two-sided: heating drives an NTC down and a PT1000 up. A dead probe does not move, which is
/// what separates it from a cold PT1000.
constexpr HeatPulseVerdict classify_heat_pulse(int32_t delta_adc_12bit, HeatPulseThresholds t) {
    if (delta_adc_12bit <= -t.move_adc_12bit) {
        return HeatPulseVerdict::stock_ntc;
    }
    if (delta_adc_12bit >= t.move_adc_12bit) {
        return HeatPulseVerdict::high_temp_pt1000;
    }
    return HeatPulseVerdict::sensor_fault;
}

/// 10-bit ADC thresholds derived from the PT1000 temptable.
/// Outside the [lower, upper] band → the reading is unambiguously NTC.
/// Inside the band → ambiguous (could be a hot NTC or a cold-to-hot PT1000).
struct Thresholds {
    uint16_t clear_ntc_upper; ///< ADC reading above this is unambiguously NTC (cold)
    uint16_t clear_ntc_lower; ///< ADC reading below this is unambiguously NTC (hot)

    /// Beyond the NTC temptable's span: a sensor fault, not a hotend class.
    uint16_t shorted_max; ///< ADC reading at or below this means a shorted probe
    uint16_t open_min; ///< ADC reading at or above this means an open probe
};

struct Inputs {
    uint16_t adc_10bit;
    bool stored_is_high_temp; ///< whether the persisted hotend is the HT (PT1000) one
};

struct Outputs {
    bool is_high_temp; ///< the detected/selected hotend is the HT (PT1000) one
    bool dialog_pending; ///< boot must ask the user to confirm the hotend

    bool operator==(const Outputs &) const = default;
};

/// Result for a reading that decides nothing (ambiguous, or unstable): keep the persisted type
/// and ask unless it is already HT.
constexpr Outputs unresolved(bool stored_is_high_temp) {
    return { stored_is_high_temp, !stored_is_high_temp };
}

/// Pure hotend-sensor classification — testable without firmware dependencies.
/// A clear-NTC reading is authoritative (select standard, no dialog); an ambiguous one (a hot
/// NTC and a cold-to-hot PT1000 read alike) trusts the persisted hotend and asks only when it
/// isn't yet HT. The clear-NTC branch must NOT raise the dialog even against a stored HT, or boot
/// loops: the caller overwrites the stored hotend, the dialog flips it back, the next boot re-asks.
/// An open or shorted probe keeps the stored type without a dialog; MINTEMP/MAXTEMP report it.
constexpr Outputs detect_impl(Inputs in, Thresholds t) {
    // Must not fall through to clear-NTC: that would silently put an HT hotend on the NTC table.
    if (in.adc_10bit <= t.shorted_max || in.adc_10bit >= t.open_min) {
        return { .is_high_temp = in.stored_is_high_temp, .dialog_pending = false };
    }

    const bool clear_ntc = (in.adc_10bit > t.clear_ntc_upper) || (in.adc_10bit < t.clear_ntc_lower);
    if (clear_ntc) {
        return { false, false };
    }
    return unresolved(in.stored_is_high_temp);
}

/// Length of the run of agreeing decisive readings ending at `current`, so that a single
/// threshold crossing cannot conclude the coast.
constexpr uint32_t confirm_heat_pulse_read(uint32_t run, HeatPulseVerdict previous, HeatPulseVerdict current) {
    if (current == HeatPulseVerdict::sensor_fault) {
        return 0;
    }
    return (run > 0 && current == previous) ? run + 1 : 1;
}

} // namespace hotend_detect
