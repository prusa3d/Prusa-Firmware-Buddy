// thread_measurement.c
#include "thread_measurement.h"
#include "cmsis_os.h" //osDelay
#include "filament_sensor.h"

void StartMeasurementTask(void const *argument) {
    fs_init();
    /* Infinite loop */
    for (;;) {
        fs_cycle();
        osDelay(1);
    }
}
