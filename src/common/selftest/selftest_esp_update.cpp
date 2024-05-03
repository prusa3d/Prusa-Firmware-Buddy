/**
 * @file selftest_esp_update.cpp
 */

#include "selftest_esp.hpp"
#include "selftest_esp_update.hpp"
#include "settings_ini.hpp"

#include "RAII.hpp"
#include "log.h"
#include "timing.h"
#include "marlin_server.hpp"
#include <device/peripherals.h>

#include "../../lib/Marlin/Marlin/src/Marlin.h"

#include <array>
#include <algorithm>
#include <filepath_operation.h>
#include <dirent.h>
#include <stdlib.h>
#include "buddy/priorities_config.h"

extern "C" {
#include <wui.h>
#include <netdev.h>
}

LOG_COMPONENT_REF(Network);

EspCredentials::EspCredentials(type_t type)
    : type(type)
    , initial_netdev_id(netdev_get_active_id())
    , progress_state(esp_credential_action::ShowInstructions) {
    switch (type) {
    case type_t::credentials_standalone:
        progress_state = esp_credential_action::ShowInstructions_qr;
        break;
    case type_t::ini_creation:
        progress_state = esp_credential_action::CheckUSB_inserted;
        break;
    case type_t::credentials_user:
        progress_state = esp_credential_action::InsertUSB; // it skips first few screens, but it might return to them in case USB / config error
        break;
    }
}

bool EspCredentials::make_file() {
    file.reset(fopen(settings_ini::file_name, "w"));
    if (file.get() == nullptr) {
        log_error(Network, "ESP credentials: Unable to create file %s", settings_ini::file_name);
        return false;
    }

    bool failed = fputs(file_str, file.get()) < 0;
    if (failed) {
        log_error(Network, "ESP credentials: Unable to write into file %s", settings_ini::file_name);
    }
    file.reset(nullptr);

    log_info(Network, "ESP credentials generated to %s", settings_ini::file_name);
    return !failed;
}

bool EspCredentials::delete_file() {
    return remove(settings_ini::file_name);
}

bool EspCredentials::file_exists() {
    unique_file_ptr fl;

    // no other thread should modify files in file system during upload
    // or this might fail
    fl.reset(fopen(settings_ini::file_name, "r"));
    return fl.get() != nullptr;
}

bool EspCredentials::upload_config() {
    bool ret = netdev_load_esp_credentials_eeprom();

    notify_reconfigure();
    return ret;
}

bool EspCredentials::AlreadySet() {
    ap_entry_t entry = netdev_read_esp_credentials_eeprom();
    return entry.ssid[0] != '\0';
}

void EspCredentials::Loop() {
    while (progress_state != esp_credential_action::Done) {
        if (progress_state != last_state) {
            capture_timestamp();
            last_state = progress_state;
        }
        usb_inserted = marlin_server::get_media_inserted();
        wifi_enabled = netdev_get_active_id() == NETDEV_ESP_ID;
        continue_yes_retry_pressed = false;
        no_pressed = false;

        if (phase) {
            switch (marlin_server::get_response_from_phase(*phase)) {
            case Response::Continue:
            case Response::Retry:
            case Response::Yes:
                continue_yes_retry_pressed = true;
                break;
            case Response::No:
                no_pressed = true;
                break;
            case Response::Abort:
            case Response::Skip:
                progress_state = esp_credential_action::Aborted;
                break;
            default:
                break;
            }
        }

        // handle abort and done outside specific loops
        switch (progress_state) {
        case esp_credential_action::Aborted:
            progress_state = esp_credential_action::Done;
            // we are not changing network interface in ini file creation, thus we dont need to restore it back
            // this solves bad rendering of button after skipping ini file creation
            if (type != type_t::ini_creation) {
                netdev_set_active_id(initial_netdev_id); // restore wifi
            }
            file.reset(nullptr);
            break;
        case esp_credential_action::Done:
            break;
        default: // specific loop
            switch (type) {
            case type_t::credentials_standalone:
            case type_t::credentials_user:
                loop();
                break;
            case type_t::ini_creation:
                loopCreateINI();
                break;
            }
        }

        // update
        if (phase) {
            marlin_server::fsm_change(*phase);
        }

        // call idle loop to prevent watchdog
        idle(true, true);
    }
}

void EspCredentials::loopCreateINI() {
    switch (progress_state) {
    case esp_credential_action::CheckUSB_inserted:
        progress_state = usb_inserted ? esp_credential_action::AskMakeFile : esp_credential_action::USB_not_inserted;
        break;
    case esp_credential_action::USB_not_inserted:
        progress_state = esp_credential_action::USB_not_inserted_wait;
        phase = PhasesESP::ESP_USB_not_inserted;
        break;
    case esp_credential_action::USB_not_inserted_wait:
        if (continue_yes_retry_pressed || usb_inserted) {
            progress_state = esp_credential_action::AskMakeFile;
        }
        break;
    case esp_credential_action::AskMakeFile:
        phase = file_exists() ? PhasesESP::ESP_ask_gen_overwrite : PhasesESP::ESP_ask_gen;
        progress_state = esp_credential_action::AskMakeFile_wait_user;
        break;
    case esp_credential_action::AskMakeFile_wait_user:
        if (continue_yes_retry_pressed) {
            progress_state = make_file() ? esp_credential_action::EjectUSB : esp_credential_action::MakeFile_failed;
        }
        break;
    case esp_credential_action::MakeFile_failed:
        progress_state = esp_credential_action::MakeFile_failed_wait_user;
        phase = PhasesESP::ESP_makefile_failed;
        break;
    case esp_credential_action::MakeFile_failed_wait_user:
        if (continue_yes_retry_pressed) {
            progress_state = esp_credential_action::CheckUSB_inserted;
        }
        break;
    case esp_credential_action::EjectUSB:
        if (usb_inserted) {
            progress_state = esp_credential_action::WaitUSB_ejected;
            phase = PhasesESP::ESP_eject_USB;
        } else {
            // this should not happen
            progress_state = esp_credential_action::Done;
        }
        break;
    case esp_credential_action::WaitUSB_ejected:
        if (continue_yes_retry_pressed || !usb_inserted) {
            progress_state = esp_credential_action::Done;
        }
        break;
    default:
        break;
    }
}

void EspCredentials::loop() {
    switch (progress_state) {
    case esp_credential_action::ShowInstructions_qr:
        progress_state = esp_credential_action::ShowInstructions_qr_wait_user;
        phase = PhasesESP::ESP_qr_instructions;
        break;
    case esp_credential_action::ShowInstructions_qr_wait_user:
        if (continue_yes_retry_pressed) {
            progress_state = esp_credential_action::ShowInstructions;
        }
        break;
    case esp_credential_action::ShowInstructions:
        progress_state = esp_credential_action::ShowInstructions_wait_user;
        phase = PhasesESP::ESP_instructions;
        break;
    case esp_credential_action::ShowInstructions_wait_user:
        if (continue_yes_retry_pressed) {
            progress_state = esp_credential_action::InsertUSB;
        }
        break;
    case esp_credential_action::InsertUSB:
        if (!usb_inserted) {
            progress_state = esp_credential_action::WaitUSB_inserted;
            phase = PhasesESP::ESP_insert_USB;
        } else {
            // this should not happen
            progress_state = esp_credential_action::VerifyConfig_init;
        }
        break;
    case esp_credential_action::WaitUSB_inserted:
        if (continue_yes_retry_pressed || usb_inserted) {
            progress_state = esp_credential_action::VerifyConfig_init;
        }
        break;
    case esp_credential_action::VerifyConfig_init:
        phase = PhasesESP::ESP_uploading_config;
        progress_state = esp_credential_action::DisableWIFI_if_needed;
        break;
    case esp_credential_action::DisableWIFI_if_needed:
        if (wifi_enabled) {
            // give GUI some time to draw, netdev_set_active_id will consume all cpu power
            if (wait_in_progress(1024)) {
                break;
            }
            netdev_set_active_id(NETDEV_NODEV_ID);
            progress_state = esp_credential_action::WaitWIFI_disabled;
        } else {
            progress_state = esp_credential_action::VerifyConfig;
        }
        break;
    case esp_credential_action::WaitWIFI_disabled:
        if (!wifi_enabled) {
            progress_state = esp_credential_action::VerifyConfig;
        }
        break;
    case esp_credential_action::VerifyConfig:
        // at this point cpu load is to high to draw screen nicely
        // so we wait a bit
        if (wait_in_progress(2048)) {
            break;
        }
        progress_state = upload_config() ? esp_credential_action::AskCredentialsDelete : esp_credential_action::ConfigNOk;
        break;
    case esp_credential_action::ConfigNOk:
        // at this point cpu load is to high to draw screen nicely
        // so we wait a bit
        if (wait_in_progress(2048)) {
            break;
        }
        progress_state = esp_credential_action::ConfigNOk_wait_user;
        phase = PhasesESP::ESP_invalid;
        break;
    case esp_credential_action::ConfigNOk_wait_user:
        if (continue_yes_retry_pressed) {
            progress_state = esp_credential_action::VerifyConfig_init;
        }
        break;
    case esp_credential_action::AskCredentialsDelete:
        // at this point cpu load is to high to draw screen nicely
        // so we wait a bit
        if (wait_in_progress(1024)) {
            break;
        }

        // Start enabling wi-fi on the background
        phase = PhasesESP::ESP_asking_credentials_delete;
        if (continue_yes_retry_pressed) {
            delete_file();
            progress_state = esp_credential_action::ShowEnableWIFI;
        } else if (no_pressed) {
            progress_state = esp_credential_action::ShowEnableWIFI;
        }
        break;
    case esp_credential_action::ShowEnableWIFI:
        progress_state = esp_credential_action::EnableWIFI;
        phase = PhasesESP::ESP_enabling_WIFI;
        break;
    case esp_credential_action::EnableWIFI:
        // give GUI some time to draw before call of netdev_set_active_id
        if (wait_in_progress(1024)) {
            break;
        }
        netdev_set_active_id(NETDEV_ESP_ID);
        progress_state = esp_credential_action::WaitWIFI_enabled;
        break;
    case esp_credential_action::WaitWIFI_enabled:
        // Wi-fi connected -> we can finish automatically
        if (wifi_enabled && netdev_get_status(NETDEV_ESP_ID) == NETDEV_NETIF_UP) {
            progress_state = esp_credential_action::Done;
        }

        // User pressed continue -> continue connecting on the background
        else if (continue_yes_retry_pressed) {
            progress_state = esp_credential_action::Done;
        }
        break;
    default:
        break;
    }
}

bool EspCredentials::wait_in_progress(uint32_t ms) {
    return (ticks_ms() - time_stamp) < ms;
}

void EspCredentials::capture_timestamp() {
    time_stamp = ticks_ms();
}

/*****************************************************************************/
// public functions

void update_esp_credentials() {
    bool credentials_on_usb = false;
    {
        unique_file_ptr fl;
        // if other thread modifies files during this action, detection might fail
        fl.reset(fopen(settings_ini::file_name, "r"));
        credentials_on_usb = fl.get() != nullptr;
    }

    marlin_server::FSM_Holder holder { PhasesESP::_none };
    EspCredentials credentials(credentials_on_usb ? EspCredentials::type_t::credentials_user : EspCredentials::type_t::credentials_standalone);
    credentials.Loop();
}

void credentials_generate_ini() {
    marlin_server::FSM_Holder holder { PhasesESP::_none };
    EspCredentials credentials(EspCredentials::type_t::ini_creation);
    credentials.Loop();
}
