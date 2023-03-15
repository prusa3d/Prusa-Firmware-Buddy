#include <leds/task.hpp>
#include <leds/side_strip.hpp>
#include <leds/side_strip_control.hpp>
#include <stdint.h>
#include <cmsis_os.h>

constexpr uint32_t refresh_ms = 1000 / 50;

void leds::run_task(void const *arg) {
    osDelay(1);
    while (true) {
        leds::side_strip_control.Tick();
        osDelay(refresh_ms);
    }
}
