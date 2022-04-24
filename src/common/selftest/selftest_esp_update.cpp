/**
 * @file selftest_esp_update.cpp
 */

#include "selftest_esp_update.hpp"
#include "RAII.hpp"
#include "log.h"
#include "marlin_server.hpp"
#include "fsm_base_types.hpp"
#include "client_fsm_types.h"

#include "../../lib/Marlin/Marlin/src/Marlin.h"

#include <array>
#include <algorithm>
#include <cstdint>
#include <basename.h>
#include <dirent.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <stm32_port.h>
#include <esp_loader.h>
#include <espif.h>
#include <wui.h>

#ifdef __cplusplus
}
#endif

#define BOOT_ADDRESS            0x00000ul
#define APPLICATION_ADDRESS     0x10000ul
#define PARTITION_TABLE_ADDRESS 0x08000ul
extern UART_HandleTypeDef huart6;

enum class esp_upload_action {
    Initial,
    Initial_wait_user,
    Connect,
    Start_flash,
    Write_data,
    ESP_error,
    USB_error,
    Error_wait_user,
    Reset,
    Aborted,
    Done
};

struct esp_entry {
    uint32_t address;
    const char *filename;
    uint32_t size;
};

class ESPUpdate {
    static constexpr size_t buffer_length = 512;

    std::array<esp_entry, 3> firmware_set;
    FILE *opened_file;
    esp_upload_action progress_state;
    esp_entry *current_file;
    uint32_t readCount;
    loader_stm32_config_t loader_config;

    FSM_Holder fsm_holder;
    PhasesSelftest phase;
    const bool from_menu;
    uint8_t progress;

    void updateProgress();

public:
    ESPUpdate(bool from_menu);

    void Loop();
};

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
    , fsm_holder(ClientFSM::Selftest, 0)
    , phase(PhasesSelftest::_none)
    , from_menu(from_menu)
    , progress(0) {
}

void ESPUpdate::updateProgress() {
    int progr = 100 * readCount / current_file->size;
    progr = std::clamp(progr, 0, 100);
    if (progress != progr) {
        progress = progr;
        fsm_holder.Change(phase, progr);
    }
}

void ESPUpdate::Loop() {
    while (progress_state != esp_upload_action::Done) {
        bool continue_pressed = false;

        //we use only 2 responses here
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
            fsm_holder.Change(phase, 0);
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
                fsm_holder.Change(phase, 0);
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
                    : esp_upload_action::Reset;
            }
            break;
        }
        case esp_upload_action::Reset:
            log_info(Network, "ESP finished flashing");
            esp_loader_flash_finish(true);
            espif_flash_deinitialize();
            notify_reconfigure();
            progress_state = esp_upload_action::Done;
            current_file = firmware_set.begin();
            readCount = 0;
            break;
        case esp_upload_action::ESP_error:
        case esp_upload_action::USB_error: {
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

        //call idle loop to prevent watchdog
        idle(true, true);
    }
}

void update_esp(bool force) {
    ESPUpdate update(force);
    update.Loop();
}
