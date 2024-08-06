
#include "CFanCtlCommon.hpp"
#include <fanctl.hpp>
#include "metric.h"
#include "Marlin/src/module/motion.h" // for active_extruder
#include <common/sensor_data.hpp>
#include <utils/utility_extensions.hpp>
#include <cmath>

void record_fanctl_metrics() {
    METRIC_DEF(metric, "fan", METRIC_VALUE_CUSTOM, 0, METRIC_HANDLER_ENABLE_ALL);
    METRIC_DEF(fan_print, "print_fan_act", METRIC_VALUE_INTEGER, 1000, METRIC_HANDLER_DISABLE_ALL);
    METRIC_DEF(fan_hbr, "hbr_fan_act", METRIC_VALUE_INTEGER, 1000, METRIC_HANDLER_DISABLE_ALL);
#if XL_ENCLOSURE_SUPPORT() // XLBOARD has additional enclosure fan
    METRIC_DEF(fan_enclosure, "hbr_fan_enc", METRIC_VALUE_INTEGER, 1000, METRIC_HANDLER_DISABLE_ALL);
#endif
    static uint32_t last_update = 0;
    static constexpr uint32_t UPDATE_PERIOD = 987;

    auto record = [](const auto &fanctl, const char *fan_name) {
        const int8_t state = ftrstd::to_underlying(fanctl.getState());
        const float pwm = fanctl.getPWM() * 100.f / fanctl.getMaxPWM();
        const float measured = fanctl.getActualRPM() * 100.f / fanctl.getMaxRPM();

        metric_record_custom(&metric, ",fan=%s state=%i,pwm=%i,measured=%i",
            fan_name, state, static_cast<int>(std::lround(pwm)), static_cast<int>(std::lround(measured)));
    };

    if (HAL_GetTick() - last_update > UPDATE_PERIOD) {
        record(Fans::print(active_extruder), "print");
        {
            const float value = Fans::print(active_extruder).getActualRPM();
            metric_record_integer(&fan_print, value);
            sensor_data().printFanAct = value;
        }
        record(Fans::heat_break(active_extruder), "heatbreak");
        {
            const float value = Fans::heat_break(active_extruder).getActualRPM();
            metric_record_integer(&fan_hbr, value);
            sensor_data().hbrFanAct = value;
        }
#if XL_ENCLOSURE_SUPPORT() // XLBOARD has additional enclosure fan
        record(Fans::enclosure(), "enclosure");
        metric_record_integer(&fan_enclosure, Fans::enclosure().getActualRPM());
#endif
        last_update = HAL_GetTick();
    }
}
