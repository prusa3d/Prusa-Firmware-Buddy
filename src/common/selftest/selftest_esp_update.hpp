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
    Info,
    Info_wait_user,
    DisableWIFI_if_needed,
    WaitWIFI_disabled,
    Connect_show,
    Connect,
    Start_flash,
    Write_data,
    ESP_error,
    USB_error,
    Error_wait_user,
    EnableWIFI,
    WaitWIFI_enabled, // pressing abort will just restore connection interface (Eth / WiFi / none)
    Finish,
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
    const bool credentials_already_set;
    uint8_t progress;
    uint8_t current_file_no;
    const uint8_t initial_netdev_id; // it is not enum because of stupid C api
    bool aborted;

    void updateProgress();
    static std::atomic<uint32_t> status;

public:
    enum init_mask {
        msk_from_menu = 0b01,
        msk_credentials_already_set = msk_from_menu << 1
    };
    enum class state {
        did_not_finished,
        finished,
        aborted
    };

    ESPUpdate(uintptr_t mask);

    void Loop();
    bool Aborted() const { return aborted; }
    static status_t GetStatus() {
        status_encoder_union u;
        u.u32 = status;
        return u.status;
    }
};

enum class esp_credential_action {
    //ini creation
    AskMakeFile,
    CheckUSB_inserted,
    USB_not_inserted,
    USB_not_inserted_wait, // wait for USB or user can force next step or abort
    AskMakeFile_wait_user,
    MakeFile_failed,
    MakeFile_failed_wait_user,
    EjectUSB,
    WaitUSB_ejected,
    //credentials upload
    ShowInstructions_qr,
    ShowInstructions_qr_wait_user,
    ShowInstructions,
    ShowInstructions_wait_user,
    InsertUSB,
    WaitUSB_inserted,
    VerifyConfig_init,
    DisableWIFI_if_needed,
    WaitWIFI_disabled,
    VerifyConfig,
    ConfigNOk,
    ConfigNOk_wait_user,
    ConfigUploaded, // config OK
    ConfigUploaded_wait_user,
    ShowEnableWIFI,
    EnableWIFI,
    WaitWIFI_enabled, // pressing abort will just restore connection interface (Eth / WiFi / none)
    Aborted,          //currently abort does not wait for user
    // Aborted_wait_user,
    Done
};

class EspCredentials {
public:
    enum class type_t {
        credentials_standalone,
        credentials_sequence,
        ini_creation
    };

private:
    static constexpr const char *file_str = "[wifi]\n"
                                            "ssid=\n"
                                            "key_mgmt=WPA\n"
                                            "psk=\n";

    static constexpr const char *file_name = "/usb/prusa_printer_settings.ini";

    bool wait_in_progress(uint32_t ms);
    void capture_timestamp();

    unique_file_ptr file;
    FSM_Holder &rfsm;
    uint32_t time_stamp;
    type_t type;
    const uint8_t initial_netdev_id; // it is not enum because of stupid C api
    esp_credential_action progress_state;
    esp_credential_action last_state; //needed to invalidate time stamp at change of state
    std::optional<PhasesSelftest> phase;
    bool usb_inserted;
    bool wifi_enabled;
    bool continue_pressed;

    bool make_file();
    static bool file_exists();
    bool upload_config();

    void loop();
    void loopCreateINI();

public:
    EspCredentials(FSM_Holder &fsm, type_t type);

    void Loop();
    static bool AlreadySet();
};
