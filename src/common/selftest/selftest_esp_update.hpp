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

enum class esp_credential_action {
    // ini creation
    CheckUSB_inserted,
    USB_not_inserted,
    USB_not_inserted_wait, // wait for USB or user can force next step or abort
    AskMakeFile,
    AskMakeFile_wait_user,
    MakeFile_failed,
    MakeFile_failed_wait_user,
    EjectUSB,
    WaitUSB_ejected,
    // credentials upload
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
    ShowEnableWIFI,
    EnableWIFI,
    AskCredentialsDelete,
    WaitWIFI_enabled, // pressing abort will just restore connection interface (Eth / WiFi / none)
    Aborted, // currently abort does not wait for user
    // Aborted_wait_user,
    Done
};

class EspCredentials {
public:
    enum class type_t {
        credentials_standalone,
        ini_creation,
        credentials_user // file generated from slicer
    };

private:
    static constexpr const char *file_str = "[wifi]\n"
                                            "ssid=\n"
                                            "psk=\n";

    bool wait_in_progress(uint32_t ms);
    void capture_timestamp();

    unique_file_ptr file;
    uint32_t time_stamp;
    type_t type;
    const uint8_t initial_netdev_id; // it is not enum because of stupid C api
    esp_credential_action progress_state = esp_credential_action::ShowInstructions;
    esp_credential_action last_state = esp_credential_action::Done; // needed to invalidate time stamp at change of state, done ensures progress_state != last_state
    std::optional<PhasesESP> phase;
    bool usb_inserted = false;
    bool wifi_enabled = false;
    bool continue_yes_retry_pressed = false;
    bool no_pressed = false;

    bool make_file();
    static bool file_exists();
    bool upload_config();
    bool delete_file();

    void loop();
    void loopCreateINI();

public:
    explicit EspCredentials(type_t type);

    void Loop();
    static bool AlreadySet();
};
