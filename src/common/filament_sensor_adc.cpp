/**
 * @file
 */
#include <cstring> // memset
#include <stdint.h>
#include <assert.h>
#include <climits>
#include <atomic>

#include "filament_sensor_adc.hpp"
#include "filament_sensors_handler.hpp"
#include "metric.h"
#include "eeprom.h"
#include "config_buddy_2209_02.h"
#include "algorithm_range.hpp"
#include "rtos_api.hpp"

namespace {

constexpr int32_t fs_disconnect_threshold = //value for detecting disconnected sensor
#if (BOARD_IS_XBUDDY && LOVEBOARD_HAS_PT100)
    10;
#elif (BOARD_IS_XLBUDDY)
    20;
#else
    2000;
#endif

} // unnamed namespace

void FSensorADC::enable() {
    state = fsensor_t::NotInitialized;
}

void FSensorADC::disable() {
    state = fsensor_t::Disabled;
}

void FSensorADC::cycle() {
    const auto filtered_value { fs_filtered_value.load() }; //store value - so interrupt cannot change it during evaluation

    if (flg_load_settings) {
        load_settings();
        init(); // will enable or disable, depends on eeprom disable flag
    }
    if (flg_invalid_calib) {
        invalidate_calibration();
        Disable();
    }
    if (flg_calibrate) {
        save_calibration(filtered_value);
        Enable();
    }
    // disabled FS will not enter cycle, but load_settings can disable it too
    // so better not try to change state when sensor is disabled
    if (state != fsensor_t::Disabled) {
        state = evaluate_state(filtered_value);
    }
}

void FSensorADC::set_filtered_value_from_IRQ(int32_t filtered_value) {
    fs_filtered_value.store(filtered_value);
    if (filtered_value != fs_filtered_value_not_ready) {
        record_filtered();
    }
}

void FSensorADC::set_state(fsensor_t st) {
    CriticalSection C;
    state = st;
}

eevar_id FSensorADC::get_eeprom_span_id() const {
    return eeprom_span_id;
}
eevar_id FSensorADC::get_eeprom_ref_id() const {
    return eeprom_ref_id;
}

FSensorADC::FSensorADC(eevar_id span_id, eevar_id ref_id, uint8_t tool_index)
    : eeprom_span_id(span_id)
    , eeprom_ref_id(ref_id)
    , tool_index(tool_index) {
    load_settings();
    init();
}

void FSensorADC::SetCalibrateFlag() {
    flg_calibrate = true;
}

void FSensorADC::SetLoadSettingsFlag() {
    flg_load_settings = true;
}

void FSensorADC::SetInvalidateCalibrationFlag() {
    flg_invalid_calib = true;
}

int32_t FSensorADC::load_settings() {
    fs_value_span = variant8_get_ui32(eeprom_get_var(eeprom_span_id));
    fs_ref_value = variant8_get_i32(eeprom_get_var(eeprom_ref_id));
    flg_load_settings = false;
    return fs_ref_value;
}

fsensor_t FSensorADC::evaluate_state(int32_t filtered_value) {
    if (filtered_value == fs_filtered_value_not_ready) {
        return fsensor_t::NotInitialized;
    }
    if (fs_ref_value == fs_ref_value_not_calibrated) {
        return fsensor_t::NotCalibrated;
    }
    if (filtered_value < fs_disconnect_threshold) {
        return fsensor_t::NotConnected;
    }
    if (IsInClosedRange(filtered_value, fs_ref_value - fs_value_span, fs_ref_value + fs_value_span)) {
        return fsensor_t::NoFilament;
    }
    return fsensor_t::HasFilament;
}

void FSensorADC::save_calibration(int32_t value) {
    if (value == fs_filtered_value_not_ready) {
        return;
    }
    eeprom_set_var(get_eeprom_ref_id(), variant8_i32(value));
    flg_calibrate = false;
    load_settings();
}

void FSensorADC::invalidate_calibration() {
    eeprom_set_var(get_eeprom_ref_id(), variant8_i32(fs_ref_value_not_calibrated));
    flg_invalid_calib = false;
    load_settings();
}

void FSensorAdcExtruder::record_state() {
    if (limit_record_state()) {
        metric_record_custom(&get_metric_state__static(), ",n=%d v=%di", tool_index, static_cast<int>(Get()));
    }
}

void FSensorAdcSide::record_state() {
    if (limit_record_state()) {
        metric_record_custom(&get_metric_state__static(), ",n=%d v=%di", tool_index, static_cast<int>(Get()));
    }
}

void FSensorAdcExtruder::record_raw(int32_t val) {
    if (limit_record_raw()) {
        metric_record_custom(&get_metric_raw__static(), ",n=%d v=%di", tool_index, val);
    }
}

void FSensorAdcSide::record_raw(int32_t val) {
    if (limit_record_raw()) {
        metric_record_custom(&get_metric_raw__static(), ",n=%d v=%di", tool_index, val);
    }
}

void FSensorAdcExtruder::record_filtered() {
    if (limit_record_filtered()) {
        metric_record_custom(&get_metric_filtered__static(), ",n=%d v=%di", tool_index, fs_filtered_value.load());
    }
}

void FSensorAdcSide::record_filtered() {
    if (limit_record_filtered()) {
        metric_record_custom(&get_metric_filtered__static(), ",n=%d v=%di", tool_index, fs_filtered_value.load());
    }
}

#define METRIC_HANDLER METRIC_HANDLER_DISABLE_ALL

metric_s &FSensorAdcExtruder::get_metric_raw__static() {
    static metric_t ret = METRIC("fsensor_raw", METRIC_VALUE_CUSTOM, 0, METRIC_HANDLER); // No min interval, is handled by limit_record_raw
    return ret;
}

metric_s &FSensorAdcExtruder::get_metric_state__static() {
    static metric_t ret = METRIC("fsensor", METRIC_VALUE_CUSTOM, 0, METRIC_HANDLER); // No min interval, is handled by limit_record_state
    return ret;
}

metric_s &FSensorAdcExtruder::get_metric_filtered__static() {
    static metric_t ret = METRIC("fsensor_filtered", METRIC_VALUE_CUSTOM, 0, METRIC_HANDLER); // No min interval, is handled by limit_record_filtered
    return ret;
}

metric_s &FSensorAdcSide::get_metric_raw__static() {
    static metric_t ret = METRIC("side_fsensor_raw", METRIC_VALUE_CUSTOM, 0, METRIC_HANDLER); // No min interval, is handled by limit_record_raw
    return ret;
}

metric_s &FSensorAdcSide::get_metric_state__static() {
    static metric_t ret = METRIC("side_fsensor", METRIC_VALUE_CUSTOM, 0, METRIC_HANDLER); // No min interval, is handled by limit_record_state
    return ret;
}

metric_s &FSensorAdcSide::get_metric_filtered__static() {
    static metric_t ret = METRIC("side_fsensor_filtered", METRIC_VALUE_CUSTOM, 0, METRIC_HANDLER); // No min interval, is handled by limit_record_filtered
    return ret;
}

FSensorAdcExtruder::FSensorAdcExtruder(eevar_id span_value, eevar_id ref_value, uint8_t tool_index)
    : FSensorADC(span_value, ref_value, tool_index)
    , limit_record_state(49)
    , limit_record_filtered(60)
    , limit_record_raw(60) {}

FSensorAdcSide::FSensorAdcSide(eevar_id span_value, eevar_id ref_value, uint8_t tool_index)
    : FSensorADC(span_value, ref_value, tool_index)
    , limit_record_state(49)
    , limit_record_filtered(60)
    , limit_record_raw(60) {}
