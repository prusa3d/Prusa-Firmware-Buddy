#include "nozzle_thermal_compensation.hpp"

#include <metric.h>
#include <module/temperature.h>
#include <tool_index.hpp>

namespace buddy::nozzle_thermal_compensation {

namespace {

    METRIC_DEF(metric_offset, "noz_therm_z", METRIC_VALUE_FLOAT, 0, METRIC_ENABLED);

} // namespace

float current_elongation_vs_reference_mm() {
    const auto tool = PhysicalToolIndex::currently_selected_opt();
    if (!tool) {
        return 0;
    }

    const float offset_mm = elongation_vs_reference_mm(Temperature::degTargetHotend(*tool));

    // Recorded on change rather than on a tick, so the metric shows only the events that moved it
    static float recorded_mm = NAN;
    if (offset_mm != recorded_mm) {
        recorded_mm = offset_mm;
        metric_record_float(&metric_offset, offset_mm);
    }

    return offset_mm;
}

} // namespace buddy::nozzle_thermal_compensation
