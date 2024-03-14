// thread_measurement.c
#include <stdbool.h>
#include <algorithm>
#include "thread_measurement.h"
#include "print_processor.hpp"
#include "cmsis_os.h" //osDelay
#include "filament_sensors_handler.hpp"
#include "marlin_client.hpp"
#include "trinamic.h"
#include "metric.h"
#include "timing.h"
#include "printers.h"

static metric_t metrics_tmc_sg[4] = {
    METRIC("tmc_sg_x", METRIC_VALUE_INTEGER, 10, METRIC_HANDLER_DISABLE_ALL),
    METRIC("tmc_sg_y", METRIC_VALUE_INTEGER, 10, METRIC_HANDLER_DISABLE_ALL),
    METRIC("tmc_sg_z", METRIC_VALUE_INTEGER, 10, METRIC_HANDLER_DISABLE_ALL),
    METRIC("tmc_sg_e", METRIC_VALUE_INTEGER, 10, METRIC_HANDLER_DISABLE_ALL),
};

static void register_trinamic_metrics() {
    for (unsigned idx = 0; idx < sizeof(metrics_tmc_sg) / sizeof(metrics_tmc_sg[0]); idx++) {
        metric_register(&metrics_tmc_sg[idx]);
    }
}

static void record_trinamic_metrics(unsigned updated_axes) {
    for (unsigned axis = 0; axis < sizeof(metrics_tmc_sg) / sizeof(metrics_tmc_sg[0]); axis++) {
        if (updated_axes & (1 << axis)) {
            metric_record_integer(&metrics_tmc_sg[axis], tmc_get_last_sg_sample(axis));
        }
    }
}

static inline bool checkTimestampsAscendingOrder(uint32_t a, uint32_t b) {
    uint32_t u = (b - a);
    return !(u & 0x80000000u);
}

void StartMeasurementTask([[maybe_unused]] void const *argument) {
    marlin_client::init();
    marlin_client::wait_for_start_processing();
    marlin_client::set_event_notify(marlin_server::EVENT_MSK_FSM, nullptr);
    register_trinamic_metrics();
    PrintProcessor::Init(); // this cannot be inside filament sensor ctor, because it can be created in any thread (outside them)

    uint32_t next_fs_cycle = ticks_ms();
    uint32_t next_sg_cycle = ticks_ms();

    tmc_set_sg_mask(
#if PRINTER_IS_PRUSA_MINI
        0 // disable sampling on mini
#else
        0x07 // XYZ
#endif
    );

    for (;;) {
        marlin_client::loop();
        uint32_t now = ticks_ms();

        // sample filament sensor
        if (checkTimestampsAscendingOrder(next_fs_cycle, now)) {
            FSensors_instance().Cycle();
            // call fs_cycle every ~50 ms
            next_fs_cycle = now + 50;
        }

        // sample stallguard
        if (checkTimestampsAscendingOrder(next_sg_cycle, now)) {
            uint8_t updated_axes = tmc_sample();

            record_trinamic_metrics(updated_axes);

            // This represents the lowest samplerate per axis
            uint32_t next_delay = 40;

            auto sg_mask = tmc_get_sg_mask();
            int num_of_enabled_axes = 0;

            for (unsigned axis = 0; axis < sizeof(metrics_tmc_sg) / sizeof(metrics_tmc_sg[0]); axis++) {
                if (sg_mask & (1 << axis)) {
                    num_of_enabled_axes += 1;
                }
                if (metrics_tmc_sg[axis].enabled_handlers) {
                    next_delay = std::min(next_delay, metrics_tmc_sg[axis].min_interval_ms);
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
