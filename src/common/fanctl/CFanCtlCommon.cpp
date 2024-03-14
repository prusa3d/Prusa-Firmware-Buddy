
#include "CFanCtlCommon.hpp"
#include <fanctl.hpp>
#include "metric.h"
#include "Marlin/src/module/motion.h" // for active_extruder
#include <utils/utility_extensions.hpp>

void record_fanctl_metrics() {
    static metric_t metric = METRIC("fan", METRIC_VALUE_CUSTOM, 0, METRIC_HANDLER_ENABLE_ALL);
    static metric_t fan_print = METRIC("print_fan_act", METRIC_VALUE_INTEGER, 1000, METRIC_HANDLER_DISABLE_ALL);
    static metric_t fan_hbr = METRIC("hbr_fan_act", METRIC_VALUE_INTEGER, 1000, METRIC_HANDLER_DISABLE_ALL);
    static uint32_t last_update = 0;
    static constexpr uint32_t UPDATE_PERIOD = 987;

    auto record = [](CFanCtl &fanctl, const char *fan_name) {
        int8_t state = ftrstd::to_underlying(fanctl.getState());
        float pwm = static_cast<float>(fanctl.getPWM()) / static_cast<float>(fanctl.getMaxPWM());
        float measured = static_cast<float>(fanctl.getActualRPM()) / static_cast<float>(fanctl.getMaxRPM());

        metric_record_custom(&metric, ",fan=%s state=%i,pwm=%i,measured=%i",
            fan_name, state, (int)(pwm * 100.0f), (int)(measured * 100.0f));
    };

    if (HAL_GetTick() - last_update > UPDATE_PERIOD) {
        record(Fans::print(active_extruder), "print");
        metric_record_integer(&fan_print, Fans::print(active_extruder).getActualRPM());
        record(Fans::heat_break(active_extruder), "heatbreak");
        metric_record_integer(&fan_hbr, Fans::heat_break(active_extruder).getActualRPM());
        last_update = HAL_GetTick();
    }
}
