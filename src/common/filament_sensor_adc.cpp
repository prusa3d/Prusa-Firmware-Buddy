/**
 * @file
 */
#include <cstring> // memset
#include <stdint.h>
#include <assert.h>
#include <climits>
#include <atomic>

#include "filament_sensor_adc.hpp"
#include <logging/log.hpp>
#include "metric.h"
#include "algorithm_range.hpp"
#include "filament_sensor_adc_eval.hpp"

#include <config_store/store_instance.hpp>
#include <option/has_adc_side_fsensor.h>

LOG_COMPONENT_REF(FSensor);

// min_interval_ms is 0, that is intended here.
// Rate limiting is done per-sensor inside FSensorADC through limit_record(_raw)
METRIC_DEF(metric_extruder_raw, "fsensor_raw", METRIC_VALUE_CUSTOM, 0, METRIC_DISABLED);
METRIC_DEF(metric_extruder, "fsensor", METRIC_VALUE_CUSTOM, 0, METRIC_DISABLED);
METRIC_DEF(metric_side_raw, "side_fsensor_raw", METRIC_VALUE_CUSTOM, 0, METRIC_DISABLED);
METRIC_DEF(metric_side, "side_fsensor", METRIC_VALUE_CUSTOM, 0, METRIC_DISABLED);

void FSensorADC::cycle() {
    const auto filtered_value { fs_filtered_value.load() }; // store value - so interrupt cannot change it during evaluation

    if (flg_invalid_calib) {
        invalidate_calibration();
        force_set_enabled(false);
    }
    if (req_calibrate == CalibrateRequest::CalibrateNoFilament) {
        CalibrateNotInserted(filtered_value);
        force_set_enabled(true);
    }

    if (req_calibrate == CalibrateRequest::CalibrateHasFilament) {
        CalibrateInserted(filtered_value);
    }

    // disabled FS will not enter cycle, but load_settings can disable it too
    // so better not try to change state when sensor is disabled
    if (is_enabled()) {
        state = FSensorADCEval::evaluate_state(filtered_value, fs_ref_nins_value, fs_ref_ins_value, fs_value_span);
    }
}

void FSensorADC::set_filtered_value_from_IRQ(int32_t filtered_value) {
    fs_filtered_value.store(filtered_value);
}

FSensorADC::FSensorADC(uint8_t tool_index, bool is_side_sensor)
    : tool_index(tool_index)
    , is_side(is_side_sensor) {
    load_settings();
}

void FSensorADC::SetCalibrateRequest(CalibrateRequest req) {
    req_calibrate = req;
}

bool FSensorADC::IsCalibrationFinished() const {
    return req_calibrate == CalibrateRequest::NoCalibration;
}

void FSensorADC::SetInvalidateCalibrationFlag() {
    flg_invalid_calib = true;
}

void FSensorADC::CalibrateInserted(int32_t filtered_value) {
    if (filtered_value == FSensorADCEval::filtered_value_not_ready) {
        return;
    }

    // value should be outside of extended span, because if its close to span that is used to evaluate filament sensor, it will not be reliable and trigger randomly
    int32_t extended_span = fs_value_span * fs_selftest_span_multipler;
    if (IsInClosedRange(filtered_value, fs_ref_nins_value - extended_span, fs_ref_nins_value + extended_span)) {
        log_info(FSensor, "Calibrating HasFilament: FAIL value: %d", static_cast<int>(filtered_value));
        invalidate_calibration();
    } else {
        log_info(FSensor, "Calibrating HasFilament: PASS value: %d", static_cast<int>(filtered_value));
#if HAS_ADC_SIDE_FSENSOR()
        if (is_side) {
            config_store().set_side_fs_ref_ins_value(tool_index, filtered_value);
        } else
#endif
        {
            config_store().set_extruder_fs_ref_ins_value(tool_index, filtered_value);
        }
        load_settings();
    }

    // mark calibration as done
    req_calibrate = CalibrateRequest::NoCalibration;
}

void FSensorADC::load_settings() {
    fs_value_span =
#if HAS_ADC_SIDE_FSENSOR()
        is_side ? config_store().get_side_fs_value_span(tool_index) :
#endif
                config_store().get_extruder_fs_value_span(tool_index);
    fs_ref_ins_value =
#if HAS_ADC_SIDE_FSENSOR()
        is_side ? config_store().get_side_fs_ref_ins_value(tool_index) :
#endif
                config_store().get_extruder_fs_ref_ins_value(tool_index);
    fs_ref_nins_value =
#if HAS_ADC_SIDE_FSENSOR()
        is_side ? config_store().get_side_fs_ref_nins_value(tool_index) :
#endif
                config_store().get_extruder_fs_ref_nins_value(tool_index);
}

void FSensorADC::CalibrateNotInserted(int32_t value) {
    if (value == FSensorADCEval::filtered_value_not_ready) {
        return;
    }
#if HAS_ADC_SIDE_FSENSOR()
    if (is_side) {
        config_store().set_side_fs_ref_nins_value(tool_index, value);
    } else
#endif
    {
        config_store().set_extruder_fs_ref_nins_value(tool_index, value);
    }
    req_calibrate = CalibrateRequest::NoCalibration;
    load_settings();

    log_info(FSensor, "Calibrating NoFilament value: %d", static_cast<int>(value));
}

void FSensorADC::invalidate_calibration() {
#if HAS_ADC_SIDE_FSENSOR()
    if (is_side) {
        config_store().set_side_fs_ref_ins_value(tool_index, FSensorADCEval::ref_value_not_calibrated);
        config_store().set_side_fs_ref_nins_value(tool_index, FSensorADCEval::ref_value_not_calibrated);
        config_store().set_side_fs_value_span(tool_index, config_store_ns::defaults::side_fs_value_span);
    } else
#endif
    {
        config_store().set_extruder_fs_ref_ins_value(tool_index, FSensorADCEval::ref_value_not_calibrated);
        config_store().set_extruder_fs_ref_nins_value(tool_index, FSensorADCEval::ref_value_not_calibrated);
        config_store().set_extruder_fs_value_span(tool_index, config_store_ns::defaults::extruder_fs_value_span);
    }
    flg_invalid_calib = false;
    load_settings();
}

void FSensorADC::record_state() {
    if (!limit_record()) {
        return;
    }

    metric_record_custom(is_side ? &metric_side : &metric_extruder, ",n=%u st=%ui,f=%" PRId32 "i,r=%" PRId32 "i,ri=%" PRId32 "i,sp=%" PRId32 "i",
        tool_index, static_cast<unsigned>(get_state()), fs_filtered_value.load(), fs_ref_nins_value, fs_ref_ins_value, fs_value_span);
}

void FSensorADC::record_raw(int32_t val) {
    if (!limit_record_raw()) {
        return;
    }

    metric_record_custom(is_side ? &metric_side_raw : &metric_extruder_raw, ",n=%u v=%" PRId32 "i", tool_index, val);
}
