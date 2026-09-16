#include "common.hpp"

#include <cstring>
#include "esp_attr.h"
#include "esp_log.h"
#include "esp_wifi.h"

uint32_t IRAM_ATTR now_seconds() {
    return xTaskGetTickCount() / configTICK_RATE_HZ;
}

void IRAM_ATTR do_wifi_scan([[maybe_unused]] void *arg) {
    wifi_scan_config_t config {};

    config.scan_type = WIFI_SCAN_TYPE_ACTIVE;
    switch (scan.scan_type) {
    case ScanType::probe:
        config.show_hidden = true;
        config.scan_time.active.min = 120;
        config.scan_time.active.max = 300;
        break;

    case ScanType::incremental:
        // The given timeouts are in ms, but per channel
        // On 2.4GHz wifi it would be 12-14 channels
        if (scan.incremental_scan_time == 0) {
            scan.incremental_scan_time = 42; // 42*12 = 504ms of total scan time
        } else {
            scan.incremental_scan_time *= 2;
            // Cap scan segment at ~30s (12*2500 = 30 000ms).
            if (scan.incremental_scan_time > 2500) {
                scan.incremental_scan_time = 2500;
            }
        }
        config.scan_time.active.max = config.scan_time.active.min = scan.incremental_scan_time;
        break;

    case ScanType::unknown:
    default:
        break;
    }

    ESP_ERROR_CHECK(esp_wifi_scan_start(&config, false));

    vTaskDelete(nullptr);
}

esp_err_t IRAM_ATTR start_wifi_scan(WifiScanCallback callback, ScanType scan_type) {
    if (scan_type == ScanType::unknown) {
        return ESP_FAIL;
    }

    if (scan.in_progress) {
        return ESP_FAIL;
    }

    scan.callback = callback;
    scan.scan_type = scan_type;

    if (!wifi_running) {
        esp_err_t err = esp_wifi_start();
        if (err == ESP_OK) {
            wifi_running = true;
        } else {
            ESP_LOGE(TAG, "Unable start the wifi for scan");
            return err;
        }
    }

    scan.in_progress = true;

    if (associated && scan_type != ScanType::probe) {
        // The wifi scan behaves differently when the esp is connected to the AP.
        // Intentionally disconnect when starting a scan.
        // The connection will be automatically reestablished after the scan.
        //
        // But we don't want to disconnect the wifi while probing.
        // That can cause "wifi speed decrease" since the connection procedure can take a long time
        esp_err_t err = esp_wifi_disconnect();
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Unable to disconnect from current wifi AP: %s", esp_err_to_name(err));
        }
        associated = false;
    }

    if (connecting) {
        esp_err_t err = esp_wifi_disconnect();
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Unable to disconnect from current wifi AP: %s", esp_err_to_name(err));
        }
        err = esp_wifi_deauth_sta(0); // 0 -> all
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Unable to deauthorize from wifi: %s", esp_err_to_name(err));
        }
    }

    xTaskCreate(&do_wifi_scan, "wifi_scan", 2048, nullptr, tskIDLE_PRIORITY + 1, nullptr);
    return ESP_OK;
}

esp_err_t IRAM_ATTR force_stop_wifi_scan() {
    scan.in_progress = false;
    scan.scan_type = ScanType::unknown;
    scan.incremental_scan_time = 0;
    return esp_wifi_scan_stop();
}

void IRAM_ATTR cache_current_ap_info() {
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        memcpy(latest_bssid, ap_info.bssid, BSSID_MAX_LEN);
        memcpy(latest_ssid, ap_info.ssid, SSID_MAX_LEN);
    }
}

void IRAM_ATTR wifi_re_connect_task([[maybe_unused]] void *arg) {
    if (!scan.in_progress) {
        ESP_LOGI(TAG, "Connecting to AP");
        esp_wifi_connect();
    } else {
        ESP_LOGW(TAG, "Unable to connect, scan is in progress. Will try to reconnect after the scan");
        scan.should_reconnect = true;
    }

    vTaskDelete(nullptr);
}

void IRAM_ATTR start_wifi_connect_task() {
    connecting = true;
    xTaskCreate(wifi_re_connect_task, "wifi_connect_task", 2048, nullptr, tskIDLE_PRIORITY, nullptr);
}

void send_link_status(int8_t status);

void IRAM_ATTR probe_handler(wifi_ap_record_t *aps, int ap_count) {
    bool found = false;

    if (associated) {
        cache_current_ap_info();
    }

    // Try to match BSSID first and if that fails go on and try SSID match . The BSSD check
    // should be sufficient, but there are APs that advertise mismatching BSSID in their
    // beacons and/or probe rensonse. That's the real culprit of the beacon timeout
    // disconnects and the primary motivation of this whole excercise.
    for (int i = 0; i < ap_count; ++i) {
        if (0 == memcmp(latest_bssid, aps[i].bssid, BSSID_MAX_LEN)) {
            found = true;
            beacon_quirk = false;
            break;
        }
    }
    if (beacon_quirk && !found) {
        for (int i = 0; i < ap_count; ++i) {
            if (latest_ssid[0] && aps[i].ssid[0]) {
                if (0 == strncmp((char *)(latest_ssid), (char *)(aps[i].ssid), SSID_MAX_LEN)) {
                    found = true;
                    break;
                }
            }
        }
    }

    if (!found) {
        if (probe_retry_count++ < probe_max_reties) {
            probe_run();
        } else {
            send_link_status(0);
            probe_in_progress = false;
        }
    } else {
        probe_in_progress = false;
        last_inbound_seen = now_seconds();
    }
}

void IRAM_ATTR probe_run() {
    start_wifi_scan(&probe_handler, ScanType::probe);
}

void handle_disconnect_and_try_reconnect();

void IRAM_ATTR event_handler(void *, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        uint8_t current_protocol;
        ESP_ERROR_CHECK(esp_wifi_get_protocol(WIFI_IF_STA, &current_protocol));
        if (current_protocol != UART_NIC_PROTOCOL) {
            ESP_ERROR_CHECK(esp_wifi_set_protocol(WIFI_IF_STA, UART_NIC_PROTOCOL));
            return;
        }
        start_wifi_connect_task();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (scan.in_progress) {
            // We have intentionally disconnected from the wifi to make sure that the scan is not interupted by anything.
            // Lets handle the disconnection at the end of the scan.
            scan.should_reconnect = true;
            connecting = false;
        } else {
            handle_disconnect_and_try_reconnect();
        }
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        connecting = false;
        last_inbound_seen = now_seconds();
        associated = true;
        beacon_quirk = true;
        send_link_status(1);
        s_retry_num = 0;
        ESP_ERROR_CHECK(esp_wifi_set_inactive_time(WIFI_IF_STA, INACTIVE_BEACON_SECONDS));
        cache_current_ap_info();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_SCAN_DONE) {
        scan.in_progress = false;
        const wifi_event_sta_scan_done_t *scan_data = (const wifi_event_sta_scan_done_t *)event_data;
        uint16_t ap_count = scan_data->number;

        if (!scan_data->status && ap_count) {
            wifi_ap_record_t *aps = (wifi_ap_record_t *)malloc(sizeof(wifi_ap_record_t) * ap_count);
            ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_count, aps));
            if (scan.callback != nullptr) {
                scan.callback(aps, ap_count);
            }
            free(aps);
        } else if (scan.callback != nullptr) {
            // if the scan failed, still call callback with no data to allow logic to redo the scan
            scan.callback(nullptr, 0);
        }

        if (!scan.in_progress) {
            // If callback didn't start a new scan check the scan type, change it and rerun scan if possible
            switch (scan.scan_type) {
            case ScanType::incremental:
                ESP_LOGI(TAG, "Restarting incremental scan");
                start_wifi_scan(scan.callback, ScanType::incremental);
                break;
            default:
                scan.scan_type = ScanType::unknown;
                if (scan.should_reconnect) {
                    scan.should_reconnect = false;
                    handle_disconnect_and_try_reconnect();
                }
                break;
            }
        }
    }
}

void IRAM_ATTR check_online_status() {
    if (!associated || probe_in_progress) {
        // Nothing to check, we are not online and we know it.
        return;
    }
    const uint32_t last = last_inbound_seen; // Atomic load
    const uint32_t now = now_seconds();
    // The time may overflow from time to time and due to the conversion to
    // seconds, we don't know when exactly. But if it overflows, the now would
    // get smaller than the last time (assuming we check often enough). In that
    // case we ignore the part up to the overflow and take just the part in the
    // „new round“.
    const uint32_t elapsed = now >= last ? now - last : now;

    if (elapsed > INACTIVE_PACKET_SECONDS) {
        probe_in_progress = true;
        probe_retry_count = 0;
        probe_run();
    }
}

void IRAM_ATTR clear_stored_ssids() {
    scan.stored_ssids_count = 0;
}

void IRAM_ATTR handle_rx_msg_scan_start([[maybe_unused]] uint8_t *data, [[maybe_unused]] Header header) {
    clear_stored_ssids();
    ESP_LOGI(TAG, "Starting scan...");

    start_wifi_scan(&store_scanned_ssids, ScanType::incremental);
}

void IRAM_ATTR handle_rx_msg_scan_stop([[maybe_unused]] uint8_t *data, [[maybe_unused]] Header header) {
    // Response is send automatically after scan is stopped
    // Intentionally don't fail if the scan is no longer running
    // (aka we connected to AP during scan)
    force_stop_wifi_scan();
}

void send_scanned_ssid(uint8_t index, const ScanResult *ap_info);

void IRAM_ATTR handle_rx_msg_scan_get([[maybe_unused]] uint8_t *data, Header header) {
    if (header.ap_index < scan.stored_ssids_count) {
        send_scanned_ssid(header.ap_index, &scan.stored_ssids[header.ap_index]);
    } else {
        send_scanned_ssid(UINT8_MAX, &EMPTY_RESULT);
    }
}

void send_number_of_ssids();

void IRAM_ATTR store_scanned_ssids(wifi_ap_record_t *aps, int ap_count) {
    for (uint8_t i = 0; i < ap_count; ++i) {
        bool found = false;
        if (scan.stored_ssids_count == SCAN_MAX_STORED_SSIDS) {
            ESP_LOGW(TAG, "Scan result storage is full");
            break;
        }
        for (uint8_t j = 0; j < scan.stored_ssids_count; ++j) {
            ESP_LOGI(TAG, "Comparing >%s< and %s", (char *)scan.stored_ssids[j].ssid, (char *)aps[i].ssid);
            if (memcmp(scan.stored_ssids[j].ssid, aps[i].ssid, SSID_MAX_LEN) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            memcpy(scan.stored_ssids[scan.stored_ssids_count].ssid, aps[i].ssid, SSID_MAX_LEN);
            scan.stored_ssids[scan.stored_ssids_count].needs_password = aps[i].authmode != WIFI_AUTH_OPEN || aps[i].pairwise_cipher != WIFI_CIPHER_TYPE_NONE;
            scan.stored_ssids_count++;
            ESP_LOGI(TAG, "Found SSID: %s", aps[i].ssid);
        }
    }

    send_number_of_ssids();
    ESP_LOGI(TAG, "Scan done. Found: %d", scan.stored_ssids_count);
}
