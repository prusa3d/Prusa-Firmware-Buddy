// thread_measurement.c
#include "thread_measurement.h"
#include "cmsis_os.h" //osDelay
#include "filament_sensor.h"
#include "marlin_client.h"

void StartMeasurementTask(void const *argument) {
    marlin_client_init();
    fs_init_on_edge();
    /* Infinite loop */
    for (;;) {
        marlin_client_loop();
        fs_cycle();
        osDelay(50);//have to wait at least few us, 1ms is very safe
    }
}
