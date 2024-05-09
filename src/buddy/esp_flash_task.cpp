#include "esp_flash_task.hpp"

#include <esp_flash.hpp>
#include <gui/gui_bootstrap_screen.hpp>
#include <tasks.hpp>

LOG_COMPONENT_REF(Buddy);

static ErrCode convert(const ESPFlash::State state) {
    switch (state) {
    case ESPFlash::State::ReadError:
        return ErrCode::ERR_SYSTEM_ESP_FW_READ;
    case ESPFlash::State::NotConnected:
        return ErrCode::ERR_SYSTEM_ESP_NOT_CONNECTED;
    case ESPFlash::State::WriteError:
    case ESPFlash::State::FlashError:
        return ErrCode::ERR_SYSTEM_ESP_COMMAND_ERR;
    default:
        return ErrCode::ERR_SYSTEM_ESP_UNKNOWN_ERR;
    }
}

class ProgressHook final : public ESPFlash::ProgressHook {
public:
    void update_progress(ESPFlash::State state, size_t current, size_t total) final {
        uint8_t percent = total ? 100 * current / total : 0;
        const char *stage_description;
        switch (state) {
        case ESPFlash::State::Init:
            stage_description = "Connecting ESP";
            break;
        case ESPFlash::State::WriteData:
            stage_description = "Flashing ESP";
            break;
        case ESPFlash::State::Checking:
            stage_description = "Checking ESP";
            break;
        default:
            stage_description = "Unknown ESP state";
        }

        gui_bootstrap_screen_set_state(percent, stage_description);
    }
};

static void flash_esp() {
    ESPFlash esp_flash;
    ProgressHook progress_hook;
    auto esp_result = esp_flash.flash(progress_hook);
    if (esp_result != ESPFlash::State::Done) {
        log_error(Buddy, "ESP flash failed with %u", static_cast<unsigned>(esp_result));
        fatal_error(convert(esp_result));
    }
}

static void flash_esp_task_body(void *) {
    buddy::hw::espPower.write(buddy::hw::Pin::State::high);
    flash_esp();
    TaskDeps::provide(TaskDeps::Dependency::esp_flashed);
    vTaskDelete(nullptr);
}

void start_flash_esp_task() {
    // Note: Error can be ignored, since this can only fail due to memory allocation
    //       and that would call bsod() anyway...
    std::ignore = xTaskCreate(flash_esp_task_body, "flash_esp", 512, nullptr, TASK_PRIORITY_ESP_UPDATE, nullptr);
}
