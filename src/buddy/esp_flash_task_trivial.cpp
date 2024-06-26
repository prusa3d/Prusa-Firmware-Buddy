#include "esp_flash_task.hpp"

#include <espif.h>
#include <option/has_esp_flash_task.h>
#include <tasks.hpp>

static_assert(!HAS_ESP_FLASH_TASK());

void start_flash_esp_task() {
    // Note: If there is no ESP to flash, this dependency is trivially provided
    //       in order to unlock dependant tasks. This is better than polluting
    //       everything with #ifdefs and doesn't cost much...
    skip_esp_flashing();
}

void skip_esp_flashing() {
    TaskDeps::provide(TaskDeps::Dependency::esp_flashed);
    espif_notify_flash_result(FlashResult::not_connected);
}
