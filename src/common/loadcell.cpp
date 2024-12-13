#include "loadcell.hpp"
#include "bsod.h"
#include "error_codes.hpp"
#include "gpio.h"
#include "metric.h"
#include "bsod.h"
#include <cmath> //isnan
#include <algorithm>
#include <numeric>
#include <limits>
#include <common/sensor_data.hpp>
#include "timing.h"
#include <logging/log.hpp>
#include "probe_position_lookback.hpp"
#include "bsod.h"
#include "config_features.h"
#if ENABLED(POWER_PANIC)
    #include "power_panic.hpp"
#endif // POWER_PANIC
#include "../Marlin/src/module/planner.h"
#include "../Marlin/src/module/endstops.h"
#include "feature/prusa/e-stall_detector.h"

LOG_COMPONENT_DEF(Loadcell, logging::Severity::info);

Loadcell loadcell;
METRIC_DEF(metric_loadcell, "loadcell", METRIC_VALUE_CUSTOM, 0, METRIC_ENABLED); // BFW-6463 - necessary for data acquisition and analysis
METRIC_DEF(metric_loadcell_hp, "loadcell_hp", METRIC_VALUE_FLOAT, 0, METRIC_DISABLED);
METRIC_DEF(metric_loadcell_xy, "loadcell_xy", METRIC_VALUE_FLOAT, 0, METRIC_DISABLED);
METRIC_DEF(metric_loadcell_age, "loadcell_age", METRIC_VALUE_INTEGER, 0, METRIC_DISABLED);

// To be used by sensor info screen so we don't have to parse the CUSTOM_VALUE from the loadcell metric
METRIC_DEF(metric_loadcell_value, "loadcell_value", METRIC_VALUE_FLOAT, 0, METRIC_DISABLED);

Loadcell::Loadcell()
    : scale(1)
    , thresholdStatic(NAN)
    , thresholdContinuous(NAN)
    , hysteresis(0)
    , failsOnLoadAbove(INFINITY)
    , failsOnLoadBelow(-INFINITY)
    , highPrecision(false)
    , tareMode(TareMode::Static)
    , z_filter()
    , xy_filter() {
    Clear();
}

void Loadcell::WaitBarrier(uint32_t ticks_us) {
    // the first sample we're waiting for needs to be valid
    while (!planner.draining() && undefinedCnt) {
        idle(true, true);
    }

    // now wait until the requested timestamp
    while (!planner.draining() && ticks_diff(loadcell.GetLastSampleTimeUs(), ticks_us) < 0) {
        idle(true, true);
    }
}

float Loadcell::Tare(TareMode mode) {
    // ensure high-precision mode is enabled when taring
    if (!highPrecision) {
        bsod("high precision not enabled during tare");
    }

    if (tareCount != 0) {
        bsod("loadcell tare already requested");
    }

    if (endstops.is_z_probe_enabled() && (endstop || xy_endstop)) {
        fatal_error("LOADCELL", "Tare under load");
    }

    tareMode = mode;

    // request tare from ISR routine
    int requestedTareCount = tareMode == TareMode::Continuous
        ? std::max(z_filter.SETTLING_TIME, xy_filter.SETTLING_TIME)
        : STATIC_TARE_SAMPLE_CNT;
    tareSum = 0;
    tareCount = requestedTareCount;

    // wait until we have all the samples that were requested
    while (!planner.draining() && tareCount != 0) {
        idle(true, true);
    }

    if (!planner.draining()) {
        if (tareMode == TareMode::Continuous) {
            // double-check filters are ready after the tare
            assert(z_filter.settled());
            assert(xy_filter.settled());
        }

        offset = tareSum / requestedTareCount;
    }

    endstop = false;
    xy_endstop = false;

    return offset * scale; // Return offset scaled to output grams
}

void Loadcell::Clear() {
    tareCount = 0;
    loadcellRaw = undefined_value;
    undefinedCnt = -UNDEFINED_INIT_MAX_CNT;
    offset = 0;
    reset_filters();
    endstop = false;
    xy_endstop = false;
}

void Loadcell::reset_filters() {
    z_filter.reset();
    xy_filter.reset();
    loadcell.analysis.Reset();
}

bool Loadcell::GetMinZEndstop() const {
    return endstop && endstops.is_z_probe_enabled();
}

bool Loadcell::GetXYEndstop() const {
    return xy_endstop;
}

void Loadcell::SetScale(float scale) {
    this->scale = scale;
}

void Loadcell::SetHysteresis(float hysteresis) {
    this->hysteresis = hysteresis;
}

float Loadcell::GetScale() const {
    return scale;
}

float Loadcell::GetHysteresis() const {
    return hysteresis;
}

int32_t Loadcell::get_raw_value() const {
    return loadcellRaw;
}

void Loadcell::SetFailsOnLoadAbove(float failsOnLoadAbove) {
    this->failsOnLoadAbove = failsOnLoadAbove;
}

float Loadcell::GetFailsOnLoadAbove() const {
    return failsOnLoadAbove;
}

void Loadcell::set_xy_endstop(const bool enabled) {
    xy_endstop_enabled = enabled;
}

void Loadcell::ProcessSample(int32_t loadcellRaw, uint32_t time_us) {
    if (loadcellRaw != undefined_value) {
        this->loadcellRaw = loadcellRaw;
        this->undefinedCnt = 0;
    } else {
        if (!HAS_LOADCELL_HX717() || (!DBGMCU->CR || (TERN0(DEBUG_LEVELING_FEATURE, DEBUGGING(LEVELING)) || DEBUGGING(ERRORS)))) {
            // see comment in hx717mux: only enable additional safety checks if HX717 is multiplexed
            // and directly attached without an active debugging session or LEVELING/ERROR flags, to
            // avoid triggering inside other breakpoints.

            // undefined value, use forward-fill only for short bursts
            if (++this->undefinedCnt > UNDEFINED_SAMPLE_MAX_CNT) {
                fatal_error(ErrCode::ERR_SYSTEM_LOADCELL_TIMEOUT);
            }
        }
    }

    // handle filters only in high precision mode
    if (highPrecision) {
        z_filter.filter(this->loadcellRaw);
        xy_filter.filter(this->loadcellRaw);
    }

    // sample timestamp
    int32_t ticks_us_from_now = ticks_diff(time_us, ticks_us());
    uint32_t timestamp_us = ticks_us() + ticks_us_from_now;
    last_sample_time_us = timestamp_us;

    metric_record_custom_at_time(&metric_loadcell, timestamp_us, " r=%" PRId32 "i,o=%" PRId32 "i,s=%0.4f", loadcellRaw, offset, (double)scale);
    metric_record_integer_at_time(&metric_loadcell_age, timestamp_us, ticks_us_from_now);

    // filtered loads
    const float tared_z_load = get_tared_z_load();
    metric_record_float(&metric_loadcell_value, tared_z_load);
    sensor_data().loadCell = tared_z_load;
    if (!std::isfinite(tared_z_load)) {
        fatal_error(ErrCode::ERR_SYSTEM_LOADCELL_INFINITE_LOAD);
    }

    const float filtered_z_load = get_filtered_z_load();
    metric_record_float_at_time(&metric_loadcell_hp, timestamp_us, filtered_z_load);

    const float filtered_xy_load = get_filtered_xy();
    metric_record_float_at_time(&metric_loadcell_xy, timestamp_us, filtered_xy_load);

    if (tareCount != 0) {
        // Undergoing tare process, only use valid samples
        if (loadcellRaw != undefined_value) {
            tareSum += loadcellRaw;
            tareCount -= 1;
        }
    } else {
        // Trigger Z endstop/probe
        float loadForEndstops, threshold;
        if (tareMode == TareMode::Static) {
            loadForEndstops = tared_z_load;
            threshold = thresholdStatic;
        } else {
            assert(!Endstops::is_z_probe_enabled() || z_filter.settled());
            loadForEndstops = filtered_z_load;
            threshold = thresholdContinuous;
        }

        if (endstop) {
            if (loadForEndstops >= (threshold + hysteresis)) {
                endstop = false;
            }
            buddy::hw::zMin.isr();
        } else {
            if (loadForEndstops <= threshold) {
                endstop = true;
                buddy::hw::zMin.isr();
            }
        }

        // Trigger XY endstop/probe
        if (xy_endstop_enabled) {
            assert(xy_filter.settled());

            // Everything as absolute values, watch for changes.
            // Load perpendicular to the sensor sense vector is not guaranteed to have defined sign.
            if (abs(filtered_xy_load) > abs(XY_PROBE_THRESHOLD)) {
                xy_endstop = true;
                buddy::hw::zMin.isr();
            }
            if (abs(filtered_xy_load) < abs(XY_PROBE_THRESHOLD) - abs(XY_PROBE_HYSTERESIS)) {
                xy_endstop = false;
                buddy::hw::zMin.isr();
            }
        }
    }

    if (Endstops::is_z_probe_enabled()) {
#if 0
      // TODO: temporarily disabled for release until true overloads are resolved
      // load is negative, so flip the signs accordingly just below
        if ((isfinite(failsOnLoadAbove) && loadForEndstops < -failsOnLoadAbove)
            || (isfinite(failsOnLoadBelow) && loadForEndstops > -failsOnLoadBelow))
            fatal_error("LOADCELL", "Loadcell overload");
#endif
    }

    // push sample for analysis
    float z_pos = buddy::probePositionLookback.get_position_at(time_us, []() { return planner.get_axis_position_mm(AxisEnum::Z_AXIS); });
    if (!std::isnan(z_pos)) {
        analysis.StoreSample(z_pos, tared_z_load);
    } else {
        // Temporary disabled as this causes positive feedback loop by blocking the calling thread if the logs are
        // being uploaded to a remote server. This does not solve the problem entirely. There are other logs that
        // can block. Still this should fix most of the issues and allow us to test the rest of the functionality
        // until a proper solution is found.
        // log_warning(Loadcell, "Got NaN z-coordinate; skipping (age=%dus)", ticks_us_from_now);
    }

    // Perform E motor stall detection
    EMotorStallDetector::Instance().ProcessSample(this->loadcellRaw);
}

void Loadcell::HomingSafetyCheck() const {
    static constexpr uint32_t MAX_LOADCELL_DATA_AGE_WHEN_HOMING_US = 100000;
    if (ticks_us() - last_sample_time_us > MAX_LOADCELL_DATA_AGE_WHEN_HOMING_US) {
        fatal_error(ErrCode::ERR_ELECTRO_HOMING_ERROR_Z);
    }
}

/**
 * @brief Create object enforcing error when positive load value is too big
 *
 * Sets grams threshold when created, restores to original when destroyed.
 * @param grams
 * @param enable Enable condition. Useful if you want to create enforcer based on condition.
 *              You can not put object simply inside if block, because you unintentionally also
 *              limit its scope.
 *   @arg @c true Normal operation
 *   @arg @c false Do not set grams threshold when created. (Doesn't affect destruction.)
 */

Loadcell::FailureOnLoadAboveEnforcer Loadcell::CreateLoadAboveErrEnforcer(bool enable, float grams) {
    return Loadcell::FailureOnLoadAboveEnforcer(*this, enable, grams);
}

/*****************************************************************************/
// IFailureEnforcer
Loadcell::IFailureEnforcer::IFailureEnforcer(Loadcell &lcell, float oldErrThreshold)
    : lcell(lcell)
    , oldErrThreshold(oldErrThreshold) {
}

/**
 *
 * @param lcell
 * @param grams
 * @param enable
 */
Loadcell::FailureOnLoadAboveEnforcer::FailureOnLoadAboveEnforcer(Loadcell &lcell, bool enable, float grams)
    : IFailureEnforcer(lcell, lcell.GetFailsOnLoadAbove()) {
    if (enable) {
        lcell.SetFailsOnLoadAbove(grams);
    }
}
Loadcell::FailureOnLoadAboveEnforcer::~FailureOnLoadAboveEnforcer() {
    lcell.SetFailsOnLoadAbove(oldErrThreshold);
}

/**
 * @brief Create object enabling high precision mode
 *
 * Keep high precision enabled when created, then restore when destroyed
 * @param enable Enable condition. Useful if you want to create enforcer based on condition.
 *              You can not put object simply inside if block, because you unintentionally also
 *              limit its scope.
 *   @arg @c true Normal operation
 *   @arg @c false Do not enable high precision mode and tare when created.
 */
Loadcell::HighPrecisionEnabler::HighPrecisionEnabler(Loadcell &lcell,
    bool enable)
    : m_lcell(lcell)
    , m_enable(enable) {
    if (m_enable) {
        m_lcell.EnableHighPrecision();
    }
}

Loadcell::HighPrecisionEnabler::~HighPrecisionEnabler() {
    if (m_enable) {
        m_lcell.DisableHighPrecision();
    }
}
