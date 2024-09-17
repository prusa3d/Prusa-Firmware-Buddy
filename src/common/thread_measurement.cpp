// thread_measurement.c
#include <stdbool.h>
#include <algorithm>
#include "thread_measurement.h"
#include "cmsis_os.h" //osDelay
#include "filament_sensors_handler.hpp"
#include "marlin_client.hpp"
#include "trinamic.h"
#include "metric.h"
#include "timing.h"
#include "printers.h"
#include <inc/MarlinConfig.h>
#include <option/has_phase_stepping.h>
#include <option/has_burst_stepping.h>

#if HAS_PHASE_STEPPING()
    #include <feature/phase_stepping/phase_stepping.hpp>
#endif

METRIC_DEF(metric_tmc_sg_x, "tmc_sg_x", METRIC_VALUE_INTEGER, 10, METRIC_HANDLER_DISABLE_ALL);
METRIC_DEF(metric_tmc_sg_y, "tmc_sg_y", METRIC_VALUE_INTEGER, 10, METRIC_HANDLER_DISABLE_ALL);
METRIC_DEF(metric_tmc_sg_z, "tmc_sg_z", METRIC_VALUE_INTEGER, 10, METRIC_HANDLER_DISABLE_ALL);
METRIC_DEF(metric_tmc_sg_e, "tmc_sg_e", METRIC_VALUE_INTEGER, 10, METRIC_HANDLER_DISABLE_ALL);

static metric_t *metrics_tmc_sg[4] = {
    &metric_tmc_sg_x,
    &metric_tmc_sg_y,
    &metric_tmc_sg_z,
    &metric_tmc_sg_e,
};

static void record_trinamic_metrics(unsigned updated_axes) {
    for (unsigned axis = 0; axis < sizeof(metrics_tmc_sg) / sizeof(metrics_tmc_sg[0]); axis++) {
        if (updated_axes & (1 << axis)) {
            metric_record_integer(metrics_tmc_sg[axis], tmc_get_last_sg_sample(axis));
        }
    }
}

static inline bool checkTimestampsAscendingOrder(uint32_t a, uint32_t b) {
    uint32_t u = (b - a);
    return !(u & 0x80000000u);
}

void StartMeasurementTask([[maybe_unused]] void const *argument) {
    FSensors_instance().task_init();

    uint32_t next_fs_cycle = ticks_ms();
    uint32_t next_sg_cycle = ticks_ms();

    tmc_set_sg_mask(
#if PRINTER_IS_PRUSA_MINI()
        0 // disable sampling on mini
#else
        0x07 // XYZ
#endif
    );

    for (;;) {
        uint32_t now = ticks_ms();

        // sample filament sensor
        if (checkTimestampsAscendingOrder(next_fs_cycle, now)) {
            FSensors_instance().task_cycle();
            // call fs_cycle every ~50 ms
            next_fs_cycle = now + 50;
        }

        // sample stallguard
        if (checkTimestampsAscendingOrder(next_sg_cycle, now)) {
#if HAS_PHASE_STEPPING() && !HAS_BURST_STEPPING()
            if (!phase_stepping::any_axis_enabled()) {
#endif
                uint8_t updated_axes = tmc_sample();
                record_trinamic_metrics(updated_axes);
#if HAS_PHASE_STEPPING() && !HAS_BURST_STEPPING()
            }
#endif

            // This represents the lowest samplerate per axis
            uint32_t next_delay = 40;

            auto sg_mask = tmc_get_sg_mask();
            int num_of_enabled_axes = 0;

            for (unsigned axis = 0; axis < sizeof(metrics_tmc_sg) / sizeof(metrics_tmc_sg[0]); axis++) {
                if (sg_mask & (1 << axis)) {
                    num_of_enabled_axes += 1;
                }
                if (metrics_tmc_sg[axis]->enabled) {
                    next_delay = std::min<uint32_t>(next_delay, metrics_tmc_sg[axis]->min_interval_ms);
                }
            }

            if (num_of_enabled_axes) {
                next_delay /= num_of_enabled_axes;
            }
            next_sg_cycle = now + next_delay;
        }

        osDelay(checkTimestampsAscendingOrder(next_fs_cycle, next_sg_cycle) ? next_fs_cycle - now : next_sg_cycle - now);
    }
}
