/// @file
#include "thermal_model_protection.hpp"

#include <algorithm>
#include <module/temperature/temp_defines.hpp>
#include <metric.h>
#include <module/temperature.h>

#if ENABLED(MODEL_DETECT_STUCK_THERMISTOR)

static constexpr int_least8_t self_healing_cycles = 10;
static_assert((THERMAL_PROTECTION_MODEL_PERIOD + self_healing_cycles) < INT_LEAST8_MAX, "THERMAL_PROTECTION_MODEL_PERIOD doesn't fit int_least8_t.");

/**
 * @brief Detect discrepancy between expected heating based on model and actual heating
 *
 * PWM output is checked once per 1 second. Each time it is THERMAL_PROTECTION_MODEL_DISCREPANCY
 * over feed_forward value failed_cycles are incremented. Each time it is under it is decremented.
 * Once failed_cycles reaches over THERMAL_PROTECTION_MODEL_PERIOD, temperature error is announced.
 *
 * @param pid_output heater PWM output
 * @param feed_forward part of the heater PWM output not affected by temperature readings
 */
void ThermalModelProtection::step(float pid_output, float feed_forward) {
    // Start the timer if already not started. In case millis() == 0 it will not start the timer.
    // But it will do no harm, as it will be started in the next call to this function.
    if (!timer) {
        timer = millis();
    }

    // Each 1 second
    if (!ELAPSED(millis(), timer)) {
        return;
    }

    timer = millis() + 1000UL;

    // Ignore extreme model forecasts caused by extrusion
    // scaling during un/retractions.
    const float model_discrepancy = pid_output - std::clamp<float>(feed_forward, 0, PID_MAX);

    // Expected interval is 1000 ms. min_interval_ms set to 100 ms, so it will be visible in samples collected if
    // expected interval doesn't hold.
    METRIC_DEF(heating_model_discrepancy, "heating_model_discrepancy", METRIC_VALUE_INTEGER, 100, METRIC_DISABLED);

    metric_record_integer(&heating_model_discrepancy, static_cast<int>(model_discrepancy));

    if (model_discrepancy > THERMAL_PROTECTION_MODEL_DISCREPANCY) {
        failed_cycles = std::min(failed_cycles + 1, THERMAL_PROTECTION_MODEL_PERIOD + self_healing_cycles);
    } else {
        failed_cycles = std::max(failed_cycles - 1, 0);
    }
}

bool ThermalModelProtection::is_ok() const {
    return failed_cycles <= THERMAL_PROTECTION_MODEL_PERIOD;
}

#endif
