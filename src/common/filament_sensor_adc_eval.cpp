#include "filament_sensor_adc_eval.hpp"
#include "algorithm_range.hpp"

namespace FSensorADCEval {

fsensor_t evaluate_state(int32_t filtered_value, int32_t fs_ref_nins_value, int32_t fs_ref_ins_value, int32_t fs_value_span) {
    if (filtered_value == filtered_value_not_ready) {
        return fsensor_t::NotInitialized;
    }
    // reference value not calibrated or out of sensible bounds
    if (fs_ref_nins_value == ref_value_not_calibrated || fs_ref_nins_value < lower_limit || fs_ref_nins_value > upper_limit || (fs_ref_ins_value != ref_value_not_calibrated && (fs_ref_ins_value < lower_limit || fs_ref_ins_value > upper_limit))) {
        return fsensor_t::NotCalibrated;
    }

    // filtered value is out of sensible bounds
    if (filtered_value < lower_limit || filtered_value > upper_limit) {
        return fsensor_t::NotConnected;
    }

    if (fs_ref_ins_value == ref_value_not_calibrated) {
        // inserted filament value is not calibrated (older versions of FW didn't save inserted value during calibration).
        // That means polarity of sensor is unknown, so we'll consider inserted as anything that is outside of inserted span (both polarities)
        return IsInClosedRange(filtered_value, fs_ref_nins_value - fs_value_span, fs_ref_nins_value + fs_value_span) ? fsensor_t::NoFilament : fsensor_t::HasFilament;
    } else if (fs_ref_nins_value < fs_ref_ins_value) {
        // Inserted value is higher then not inserted
        return IsInClosedRange(filtered_value, lower_limit, fs_ref_nins_value + fs_value_span) ? fsensor_t::NoFilament : fsensor_t::HasFilament;
    } else if (fs_ref_nins_value > fs_ref_ins_value) {
        // Not inserted value is higher then inserted
        return IsInClosedRange(filtered_value, fs_ref_nins_value - fs_value_span, upper_limit) ? fsensor_t::NoFilament : fsensor_t::HasFilament;
    }
    return fsensor_t::NotCalibrated;
}

} // namespace FSensorADCEval
