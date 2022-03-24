// thread_measurement.c
#include <stdbool.h>
#include <algorithm>
#include "thread_measurement.h"
#include "print_processor.hpp"
#include "cmsis_os.h" //osDelay
#include "filament_sensor_api.hpp"
#include "marlin_client.h"
#include "trinamic.h"
#include "timing.h"

static inline bool checkTimestampsAscendingOrder(uint32_t a, uint32_t b) {
    uint32_t u = (b - a);
    return !(u & 0x80000000u);
}

void StartMeasurementTask(void const *argument) {
    marlin_client_init();
    marlin_client_wait_for_start_processing();
    marlin_client_set_event_notify(MARLIN_EVT_MSK_FSM, nullptr);
    PrintProcessor::Init(); // this cannot be inside filament sensor ctor, because it can be created in any thread (outside them)

    uint32_t next_fs_cycle = ticks_ms();
    uint32_t next_sg_cycle = ticks_ms();

    for (;;) {
        marlin_client_loop();
        uint32_t now = ticks_ms();

        // sample filament sensor
        if (checkTimestampsAscendingOrder(next_fs_cycle, now)) {
            FSensors_instance().Cycle();
            // call fs_cycle every ~50 ms
            next_fs_cycle = now + 50;
        }

        // sample stallguard
        if (checkTimestampsAscendingOrder(next_sg_cycle, now)) {
            tmc_sample();

            // This represents the lowest samplerate per axis
            uint32_t next_delay = 40;

            auto sg_mask = tmc_get_sg_mask();
            int num_of_enabled_axes = 0;

            for (unsigned axis = 0; axis < 4; axis++) {
                if (sg_mask & (1 << axis))
                    num_of_enabled_axes += 1;
            }

            if (num_of_enabled_axes)
                next_delay /= num_of_enabled_axes;
            next_sg_cycle = now + next_delay;
        }

        osDelay(checkTimestampsAscendingOrder(next_fs_cycle, next_sg_cycle) ? next_fs_cycle - now : next_sg_cycle - now);
    }
}
