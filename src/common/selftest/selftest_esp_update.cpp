/**
 * @file selftest_esp_update.cpp
 */

#include "selftest_esp_update.hpp"
#include "RAII.hpp"
#include "log.h"
#include "marlin_server.hpp"
#include "fsm_base_types.hpp"
#include "client_fsm_types.h"
#include "selftest_esp_type.hpp"

#include "../../lib/Marlin/Marlin/src/Marlin.h"

#include <array>
#include <algorithm>
#include <cstdint>
#include <basename.h>
#include <dirent.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <stm32_port.h>
#include <esp_loader.h>
#include <espif.h>
#include <wui.h>

#include "FreeRTOS.h"
#include "task.h"

#ifdef __cplusplus
}
#endif

#define BOOT_ADDRESS            0x00000ul
#define APPLICATION_ADDRESS     0x10000ul
#define PARTITION_TABLE_ADDRESS 0x08000ul
extern UART_HandleTypeDef huart6;

namespace {

enum class esp_upload_action {
    Initial,
    Initial_wait_user,
    Connect,
    Start_flash,
    Write_data,
    ESP_error,
    USB_error,
    Error_wait_user,
    Finish_and_reset,
    Finish_wait_user,
    Aborted, //currently abort does not wait for user
    // Aborted_wait_user,
    Done
};

struct esp_entry {
    uint32_t address;
    const char *filename;
    uint32_t size;
};

struct status_t {
    PhasesSelftest phase;
    uint8_t progress;

    void Empty() {
        phase = PhasesSelftest::_none;
        progress = 0;
    }

    constexpr bool operator==(const status_t &other) const {
        return ((progress == other.progress) && (phase == other.phase));
    }

    constexpr bool operator!=(const status_t &other) const {
        return !((*this) == other);
    }
};

union status_encoder_union {
    uint32_t u32;
    status_t status;
};

static_assert(sizeof(uint32_t) >= sizeof(status_t), "error esp status is too big");
} // anonymous namespace

class ESPUpdate {
    static constexpr size_t buffer_length = 512;

    std::array<esp_entry, 3> firmware_set;
    FILE *opened_file;
    esp_upload_action progress_state;
    esp_entry *current_file;
    uint32_t readCount;
    loader_stm32_config_t loader_config;
    PhasesSelftest phase;
    const bool from_menu;
    uint8_t progress;

    void updateProgress();
    static std::atomic<uint32_t> status;

public:
    ESPUpdate(bool from_menu);

    void Loop();
    static status_t GetStatus() {
        status_encoder_union u;
        u.u32 = status;
        return u.status;
    }
};

std::atomic<uint32_t> ESPUpdate::status = 0;

ESPUpdate::ESPUpdate(bool from_menu)
    : firmware_set({ { { .address = PARTITION_TABLE_ADDRESS, .filename = "/internal/res/esp/partition-table.bin", .size = 0 },
        { .address = BOOT_ADDRESS, .filename = "/internal/res/esp/bootloader.bin", .size = 0 },
        { .address = APPLICATION_ADDRESS, .filename = "/internal/res/esp/uart_wifi.bin", .size = 0 } } })
    , opened_file(NULL)
    , progress_state(esp_upload_action::Initial)
    , current_file(firmware_set.begin())
    , readCount(0)
    , loader_config({
          .huart = &huart6,
          .port_io0 = GPIOE,
          .pin_num_io0 = GPIO_PIN_6,
          .port_rst = GPIOC,
          .pin_num_rst = GPIO_PIN_13,
      })
    , phase(PhasesSelftest::_none)
    , from_menu(from_menu)
    , progress(0) {
}

void ESPUpdate::updateProgress() {
    int progr = 100 * readCount / current_file->size;
    progr = std::clamp(progr, 0, 100);
    if (progress != progr) {
        progress = progr;
    }
}

void ESPUpdate::Loop() {
    while (progress_state != esp_upload_action::Done) {
        bool continue_pressed = false;

        //we use only 2 responses here
        //it is safe to use it from different thread as long as no other thread reads it
        switch (ClientResponseHandler::GetResponseFromPhase(phase)) {
        case Response::Continue:
            continue_pressed = true;
            break;
        case Response::Abort:
            progress_state = esp_upload_action::Aborted;
            break;
        default:
            break;
        }

        switch (progress_state) {
        case esp_upload_action::Initial:
            phase = from_menu ? PhasesSelftest::ESP_ask_from_menu : PhasesSelftest::ESP_ask_auto;
            progress_state = esp_upload_action::Initial_wait_user;
            break;
        case esp_upload_action::Initial_wait_user:
            if (continue_pressed) {
                espif_flash_initialize();
                struct stat fs;
                for (esp_entry *chunk = firmware_set.begin();
                     chunk != firmware_set.end(); ++chunk) {
                    if (stat(chunk->filename, &fs) == 0) {
                        chunk->size = fs.st_size;
                    }
                }
                progress_state = esp_upload_action::Connect;
                phase = PhasesSelftest::ESP_upload;
            }
            break;
        case esp_upload_action::Connect: {
            esp_loader_connect_args_t config = ESP_LOADER_CONNECT_DEFAULT();
            if (ESP_LOADER_SUCCESS == esp_loader_connect(&config)) {
                log_info(Network, "ESP boot connect OK");
                progress_state = esp_upload_action::Start_flash;
            } else {
                log_debug(Network, "ESP boot failedK");
                progress_state = esp_upload_action::ESP_error;
            }
            break;
        }
        case esp_upload_action::Start_flash:
            if ((opened_file = fopen(current_file->filename, "rb")) == nullptr) {
                log_error(Network, "ESP flash: Unable to open file %s", current_file->filename);
                progress_state = esp_upload_action::USB_error;
                break;
            } else {
                log_info(Network, "ESP Start flash %s", current_file->filename);
                updateProgress();
            }

            if (esp_loader_flash_start(
                    current_file->address,
                    current_file->size,
                    buffer_length)
                != ESP_LOADER_SUCCESS) {
                log_error(Network, "ESP flash: Unable to start flash on address %0xld", current_file->address);
                progress_state = esp_upload_action::ESP_error;
                fclose(opened_file);
                break;
            } else {
                progress_state = esp_upload_action::Write_data;
                readCount = 0;
            }
            break;
        case esp_upload_action::Write_data: {
            size_t readBytes = 0;
            uint8_t buffer[buffer_length];

            readBytes = fread(buffer, 1, sizeof(buffer), opened_file);
            readCount += readBytes;
            log_debug(Network, "ESP read data %ld", readCount);
            if (ferror(opened_file)) {
                log_error(Network, "ESP flash: Unable to read file %s", current_file->filename);
                readBytes = 0;
                progress_state = esp_upload_action::USB_error;
                break;
            }
            if (readBytes > 0) {
                if (esp_loader_flash_write(buffer, readBytes) != ESP_LOADER_SUCCESS) {
                    log_error(Network, "ESP flash write FAIL");
                    progress_state = esp_upload_action::ESP_error;
                    break;
                } else {
                    updateProgress();
                }
            } else {
                fclose(opened_file);
                ++current_file;
                progress_state = current_file != firmware_set.end()
                    ? esp_upload_action::Start_flash
                    : esp_upload_action::Finish_and_reset;
            }
            break;
        }
        case esp_upload_action::Finish_and_reset:
            log_info(Network, "ESP finished flashing");
            phase = PhasesSelftest::ESP_passed;
            esp_loader_flash_finish(true);
            espif_flash_deinitialize();
            notify_reconfigure();
            progress_state = esp_upload_action::Finish_wait_user;
            current_file = firmware_set.begin();
            readCount = 0;
            break;
        case esp_upload_action::Finish_wait_user:
            if (continue_pressed) {
                progress_state = esp_upload_action::Done;
            }
            break;
        case esp_upload_action::ESP_error:
        case esp_upload_action::USB_error: {
            phase = PhasesSelftest::ESP_failed;
            esp_loader_flash_finish(false);
            progress_state = esp_upload_action::Error_wait_user;
            current_file = firmware_set.begin();
            readCount = 0;
            fclose(opened_file);
            break;
        }
        case esp_upload_action::Error_wait_user:
            if (continue_pressed) {
                progress_state = esp_upload_action::Done;
            }
            break;
        case esp_upload_action::Aborted:
            progress_state = esp_upload_action::Done;
            break;
        case esp_upload_action::Done:
            break;
        }

        //actualize status
        status_encoder_union u;
        u.status.phase = phase;
        u.status.progress = progress;
        status = u.u32;
    }
}

static void EspTask(void *pvParameters) {
    ESPUpdate update((int)pvParameters);

    update.Loop();

    vTaskDelete(NULL); // kill itself
}

void update_esp(bool force) {

    uint tasks_running_at_start = uxTaskGetNumberOfTasks();

    TaskHandle_t xHandle = nullptr;

    xTaskCreate(
        EspTask,              // Function that implements the task.
        "ESP UPDATE",         // Text name for the task.
        512,                  // Stack size in words, not bytes.
        (void *)((int)force), // Parameter passed into the task.
        osPriorityNormal,     // Priority at which the task is created.
        &xHandle);            // Used to pass out the created task's handle.

    if (xHandle) {
        FSM_Holder fsm_holder(ClientFSM::Selftest, 0);
        status_t status;
        status.Empty();

        // wait until task kills itself
        while (uxTaskGetNumberOfTasks() > tasks_running_at_start) {
            status_t current = ESPUpdate::GetStatus();

            if (current != status) {
                status = current;
                SelftestESP_t data(status.progress);
                fsm_holder.Change(status.phase, data);
            }

            //call idle loop to prevent watchdog
            idle(true, true);
        }
    }
}
