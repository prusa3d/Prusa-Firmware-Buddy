#include <leds/task.hpp>
#include <leds/side_strip.hpp>
#include <leds/side_strip_control.hpp>
#include <stdint.h>
#include <cmsis_os.h>
#include <configuration_store.hpp>

constexpr uint32_t refresh_ms = 1000 / 50;

void leds::run_task([[maybe_unused]] void const *arg) {
    osDelay(1);
    bool ena = config_store().side_leds_enabled.get();
    leds::side_strip_control.SetEnable(ena);
    while (true) {
        leds::side_strip_control.Tick();
        osDelay(refresh_ms);
    }
}
