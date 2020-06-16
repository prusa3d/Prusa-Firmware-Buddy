// thread_measurement.c
#include "thread_measurement.h"
#include "cmsis_os.h" //osDelay
#include "filament_sensor.h"
#include "marlin_client.h"
#include "trinamic.h"

void StartMeasurementTask(void const *argument) {
    marlin_client_init();
    marlin_client_wait_for_start_processing();
    uint8_t fs_counter = 0; // counter for fs_cycle timing
    fs_init_on_edge();
    marlin_client_set_event_notify(MARLIN_EVT_MSK_FSM);
    /* Infinite loop */
    // there is no delay in this loop because tmc_sample is blocking and take always 5ms
    // waiting in tmc_sample is done using osSignalWait and osDelay (implementation of tmc hardware uart in arduino library)
    for (;;) {
        marlin_client_loop();
        if (++fs_counter >= 5) {
            fs_cycle(); // called every 5th loop (~50ms)
            fs_counter = 0;
        }
        if (tmc_sample()) // non zero value means "any axis sampled", then it take 5ms
            osDelay(5);   // so we will wait 5ms to ensure 10ms loop period
        else              //
            osDelay(10);  // otherwise we will wait 10ms (full period)
        //TODO: maybe improve timing to for more accurate and equidistant samples
    }
}
