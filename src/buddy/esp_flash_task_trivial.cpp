#include "esp_flash_task.hpp"

#include <tasks.hpp>

void start_flash_esp_task() {
    // Note: If there is no ESP to flash, this dependency is trivially provided
    //       in order to unlock dependant tasks. This is better than polluting
    //       everything with #ifdefs and doesn't cost much...
    TaskDeps::provide(TaskDeps::Dependency::esp_flashed);
}
