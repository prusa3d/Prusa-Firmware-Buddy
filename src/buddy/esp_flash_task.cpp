#include "esp_flash_task.hpp"

#include <espif.h>
#include <esp_flash.hpp>
#include <gui/gui_bootstrap_screen.hpp>
#include <tasks.hpp>
#include <option/has_esp_flash_task.h>
#include <option/has_embedded_esp32.h>

static_assert(HAS_ESP_FLASH_TASK());

LOG_COMPONENT_REF(Buddy);

using namespace buddy_esp_serial_flasher;

static void flash_esp() {
    const Result result = flash();
    log_info(Buddy, "esp flash result %u", static_cast<unsigned>(result));
    switch (result) {
    case Result::success:
        return espif_notify_flash_result(FlashResult::success);
    case Result::not_connected:
#if HAS_EMBEDDED_ESP32()
        fatal_error(ErrCode::ERR_SYSTEM_ESP_NOT_CONNECTED);
#endif
        return espif_notify_flash_result(FlashResult::not_connected);
    case Result::protocol_error:
    case Result::filesystem_error:
    case Result::checksum_mismatch:
    case Result::hal_error:
        return espif_notify_flash_result(FlashResult::failure);
    }
}

static void flash_esp_task_body(void *) {
#if HAS_EMBEDDED_ESP32()
    buddy::hw::espPower.write(buddy::hw::Pin::State::high);
#endif
#if PRINTER_IS_PRUSA_MINI
    vTaskDelay(pdMS_TO_TICKS(1000));
#endif
    flash_esp();
    TaskDeps::provide(TaskDeps::Dependency::esp_flashed);
    vTaskDelete(nullptr);
}

void start_flash_esp_task() {
    // Note: Error can be ignored, since this can only fail due to memory allocation
    //       and that would call bsod() anyway...
    std::ignore = xTaskCreate(flash_esp_task_body, "flash_esp", 512, nullptr, TASK_PRIORITY_ESP_UPDATE, nullptr);
}
