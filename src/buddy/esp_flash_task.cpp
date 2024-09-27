#include "esp_flash_task.hpp"
#include "esp_uart_dma_buffer_rx.hpp"

#include <device/peripherals_uart.hpp>
#include <espif.h>
#include <esp_flash.hpp>
#include <gui/gui_bootstrap_screen.hpp>
#include <tasks.hpp>
#include <option/has_esp_flash_task.h>
#include <option/has_embedded_esp32.h>
#include <logging/log.hpp>

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
    flash_esp();
    TaskDeps::provide(TaskDeps::Dependency::esp_flashed);
    vTaskDelete(nullptr);
}

void start_flash_esp_task() {
    // Note: Error can be ignored, since this can only fail due to memory allocation
    //       and that would call bsod() anyway...
    std::ignore = xTaskCreate(flash_esp_task_body, "flash_esp", 512, nullptr, TASK_PRIORITY_ESP_UPDATE, nullptr);
}

void skip_esp_flashing() {
    espif_notify_flash_result(FlashResult::success);
    TaskDeps::provide(TaskDeps::Dependency::esp_flashed);

    // To make esp work we need to initialize the uart DMA to receive messages from esp.
    // Normally this is done at the end of the flashing when we change the baudrate.
    HAL_UART_Receive_DMA(&uart_handle_for_esp, (uint8_t *)dma_buffer_rx, RX_BUFFER_LEN);
}
