/**
 * @file selftest_esp_update.hpp
 * @brief ESP flash
 */

#pragma once
#include <cstdint>
#include <memory> // std::unique_ptr

#include "client_response.hpp"
#include "marlin_server.hpp"
#include "unique_file_ptr.hpp"

extern "C" {
#include <stm32_port.h>
}

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
    uint8_t current_file : 4;
    uint8_t count_of_files : 4;

    void Empty() {
        phase = PhasesSelftest::_none;
        progress = 0;
    }

    constexpr bool operator==(const status_t &other) const {
        return ((progress == other.progress) && (phase == other.phase) && (current_file == other.current_file) && (count_of_files == other.count_of_files));
    }

    constexpr bool operator!=(const status_t &other) const {
        return !((*this) == other);
    }
};

static_assert(sizeof(uint32_t) >= sizeof(status_t), "error esp status is too big");

union status_encoder_union {
    uint32_t u32;
    status_t status;
};

class ESPUpdate {
    static constexpr size_t buffer_length = 512;
    static constexpr size_t files_to_upload = 3;

    unique_file_ptr file;

    std::array<esp_entry, files_to_upload> firmware_set;
    esp_upload_action progress_state;
    esp_entry *current_file;
    uint32_t readCount;
    loader_stm32_config_t loader_config;
    PhasesSelftest phase;
    const bool from_menu;
    uint8_t progress;
    uint8_t current_file_no;

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

enum class esp_credential_action {
    CheckNeeded,
    ShowInstructions,
    ShowInstructions_wait_user,
    AskMakeFile,
    AskMakeFile_wait_user,
    MakeFile_failed,
    MakeFile_failed_wait_user,
    AskLoadConfig,
    AskLoadConfig_wait_user,
    VerifyConfig,
    ConfigNOk,
    ConfigNOk_wait_user,
    ConfigUploaded, // config OK
    ConfigUploaded_wait_user,
    Aborted, //currently abort does not wait for user
    // Aborted_wait_user,
    Done
};

class EspCredentials {
    static constexpr const char *file_str = "[wifi]\n"
                                            "ssid=\n"
                                            "key_mgmt=WPA\n"
                                            "psk=\n";

    static constexpr const char *file_name = "/usb/prusa_printer_settings.ini";

    unique_file_ptr file;
    FSM_Holder &rfsm;
    bool standalone;
    esp_credential_action progress_state;
    std::optional<PhasesSelftest> phase;

    bool make_file();
    bool already_set() const;
    static bool file_exists();
    bool upload_config();

public:
    EspCredentials(FSM_Holder &fsm, bool standalone);

    void Loop();
};
