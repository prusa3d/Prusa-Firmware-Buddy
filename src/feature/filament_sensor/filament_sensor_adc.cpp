/**
 * @file
 */
#include <cstring> // memset
#include <stdint.h>
#include <climits>
#include <atomic>

#include "filament_sensor_adc.hpp"
#include <logging/log.hpp>
#include "metric.h"
#include "filament_sensor_adc_eval.hpp"

#include <config_store/store_instance.hpp>
#include <option/has_adc_side_fsensor.h>
#include <feature/filament_sensor/calibrator/filament_sensor_calibrator_adc.hpp>

static_assert(std::is_same_v<FSensorADC::Value, FSensorADCEval::Value>);

LOG_COMPONENT_REF(FSensor);

// min_interval_ms is 0, that is intended here.
// Rate limiting is done per-sensor inside FSensorADC through limit_record(_raw)
METRIC_DEF(metric_extruder, "fsensor", METRIC_VALUE_CUSTOM, 0, METRIC_DISABLED);
#if HAS_ADC_SIDE_FSENSOR()
METRIC_DEF(metric_side, "side_fsensor", METRIC_VALUE_CUSTOM, 0, METRIC_DISABLED);
#endif

void FSensorADC::cycle() {
    if (!is_enabled()) {
        return;
    }

    const auto filtered_value { fs_filtered_value.load() }; // store value - so interrupt cannot change it during evaluation

    // disabled FS will not enter cycle, but load_settings can disable it too
    // so better not try to change state when sensor is disabled
    state = FSensorADCEval::evaluate_state(filtered_value, fs_ref_nins_value, fs_ref_ins_value, state);
}

void FSensorADC::set_filtered_value_from_IRQ(Value filtered_value) {
    fs_filtered_value.store(filtered_value);
}

bool FSensorADC::is_calibrated() const {
    return FSensorADCEval::within_limits(fs_ref_ins_value)
        && FSensorADCEval::within_limits(fs_ref_nins_value);
}

FSensorADC::FSensorADC(FilamentSensorID id)
    : IFSensor(id) {
    load_settings();
}

void FSensorADC::load_settings() {
#if HAS_ADC_SIDE_FSENSOR()
    const bool is_side = (id_.position == FilamentSensorID::Position::side);
#endif
    const uint8_t tool_index = id_.index;

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

void FSensorADC::record_state() {
    if (!limit_record.check(ticks_ms())) {
        return;
    }

    const uint8_t tool_index = id_.index;

    metric_t *metric = &metric_extruder;
#if HAS_ADC_SIDE_FSENSOR()
    if (id_.position == FilamentSensorID::Position::side) {
        metric = &metric_side;
    }
#endif

    metric_record_custom(metric, ",n=%u st=%ui,f=%" PRId32 "i,r=%" PRId32 "i,ri=%" PRId32 "i",
        tool_index, static_cast<unsigned>(get_state()), fs_filtered_value.load(), fs_ref_nins_value, fs_ref_ins_value);
}

FilamentSensorCalibrator *FSensorADC::create_calibrator(FilamentSensorCalibrator::Storage &storage) {
    return &storage.emplace<FilamentSensorCalibratorADC>(*this);
}
