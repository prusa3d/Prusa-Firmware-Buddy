#pragma once

#include <algorithm>

#include <option/has_nextruder.h>

/// Compensation of the nozzle thermal expansion between bed leveling and printing.
///
/// The nozzle stack grows with temperature, moving the tip closer to the bed. The bed is
/// levelled well below printing temperature, so at printing temperature the tip sits lower
/// than the mesh says and the first layer comes out squished.
///
/// What matters is the elongation *difference* between probing and printing, never either
/// absolute length. The same quantity therefore serves both halves of the correction: it is
/// subtracted from each probe result as it is taken, and added to g-code Z in the
/// logical->native transform. The reference the two share cancels out, so what reaches the
/// carriage is e(printing) - e(that mesh point's own tap), whatever temperature either
/// happened at.
///
/// A mesh point therefore carries the temperature it was probed at implicitly, and each point
/// carries its own - a temperature drift across a long probing sequence corrects itself.
///
/// The correction is derived from the nozzle *target*, not from a measurement, so it only
/// changes when something commands a new target - between those points it is bit-identical
/// and cannot move Z.
///
/// The g-code half is never scaled by the leveling fade height, but the probe half rides inside
/// the mesh and does fade with it, so above the fade height the two stop cancelling and a uniform
/// residue of up to ~17 um remains, depending on how far that mesh was probed from the reference.
///
/// Known gap: toolchanger crash recovery zeroes the hotend targets before converting the stored
/// return position back to native, and restores them only afterwards, so the recovery return
/// lands uncompensated and the rest of that layer prints at the pre-feature height. Fixing it
/// means carrying the applied offset alongside the return position rather than re-deriving it
/// from a live target.
namespace buddy::nozzle_thermal_compensation {

#if HAS_NEXTRUDER()

/// Length of the nozzle stack that thermally elongates, at room temperature [mm]. Which end of
/// the working range it is specified at shifts the result by the expansion of that length itself,
/// well under a micron, so it is not worth modelling.
///
/// Here rather than in the per-printer configs because the figure is the same for every
/// HAS_NEXTRUDER() printer, and duplicating it per printer would invite them to drift.
constexpr float nozzle_expanding_length_mm = 21.0f;

/// Linear thermal expansion coefficient of the nozzle material (brass) [1/K].
constexpr float nozzle_thermal_expansion_coef = 1.95e-5f;

#else
    #error "nozzle geometry unknown for this hotend"
#endif

/// The temperature probe results are normalized to, and that g-code Z is corrected against
/// [degC].
///
/// Only an origin - it cancels out of the applied correction - so the value is chosen purely to
/// keep what gets baked into the mesh small, which bounds the above-fade-height residue. Probing
/// happens well below printing temperature and lands roughly between 140 and 210 degC across the
/// filament presets, so the middle of that range is about as close as one constant can get.
constexpr float reference_temperature_c = 170.0f;

/// Elongation of the nozzle stack at the given temperature, measured from 0 degC [mm].
///
/// Sub-zero inputs clamp to zero - a nozzle does not get shorter than its own cold length.
[[nodiscard]] constexpr float elongation_vs_0c_mm(float temperature_c) {
    return nozzle_expanding_length_mm * nozzle_thermal_expansion_coef * std::max(temperature_c, 0.0f);
}

/// How much longer the nozzle is at `temperature_c` than at reference_temperature_c [mm].
///
/// Negative below the reference, which is correct in both directions: a colder nozzle is
/// genuinely shorter, so its tip needs the carriage lower to reach the same height.
[[nodiscard]] constexpr float elongation_vs_reference_mm(float temperature_c) {
    return elongation_vs_0c_mm(temperature_c) - elongation_vs_0c_mm(reference_temperature_c);
}

/// elongation_vs_reference_mm() of the active tool's target temperature. Zero when no tool is
/// selected.
///
/// Tracks the target through sequences that temporarily overwrite it - filament change, nozzle
/// cleaning, spool join and auto retract all set a different target and restore it - so the term
/// shifts for their duration and returns with it.
///
/// Marlin server thread only.
[[nodiscard]] float current_elongation_vs_reference_mm();

} // namespace buddy::nozzle_thermal_compensation
