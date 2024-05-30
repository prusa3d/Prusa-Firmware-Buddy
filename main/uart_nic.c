/* UART NIC

  This code makes ESP WiFi device accessible to external system using UART. 
  This is implemented using a simple protocol that enables sending and
  receiveing network packets and some confuration using messages.

  In general the application works as follows:
  - Read incomming messages on UART
  - Read incomming packets on WiFi
  - Resend incomming packets from Wifi as UART messages
  - Resend incomming packet messages from UART using WiFi
  - Configure WiFi interface acoring to client message
  - Report link status on Wifi event or explicit request using UART message

  This aims to be used from MCUs. A Python script that exposes UART nic as
  Linux tap device is attached for testing.


  Copyright (C) 2022 Prusa Research a.s - www.prusa3d.com
  SPDX-License-Identifier: GPL-3.0-or-later 
*/

#define UART_FULL_THRESH_DEFAULT (60)

#include <string.h>
#include <stdatomic.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "driver/gpio.h"
#include "driver/uart.h"
#include "soc/uart_reg.h"

#include "esp_private/wifi.h"
#include "esp_wpa.h"
#include <arpa/inet.h>


// Externals with no header
esp_err_t mac_init(void);

#define FW_VERSION 12

#define SCAN_MAX_STORED_SSIDS 64
#define SSID_LEN 32
#define BSSID_LEN 6

// Hack: because we don't see the beacon on some networks (and it's quite
// common), but don't want to be "flapping", we set the timeout for beacon
// inactivity to a ridiculously long time and handle the disconnect ourselves.
//
// It's not longer for the only reason the uint16_t doesn't hold as big numbers.
static const uint16_t INACTIVE_BEACON_SECONDS = 3600 * 18;
// This is the effective timeout. If we don't receive any packet for this long,
// we consider the signal lost.
//
// TODO: Shall we generate something to provoke getting some packets? Like ARP
// pings to the AP?
static const uint32_t INACTIVE_PACKET_SECONDS = 5;

#define MSG_DEVINFO_V2 0
#define MSG_CLIENTCONFIG_V2 6
#define MSG_PACKET_V2 7
#define MSG_SCAN_START 8
#define MSG_SCAN_STOP 9
#define MSG_SCAN_AP_CNT 10
#define MSG_SCAN_AP_GET 11

struct __attribute__((packed)) header {
    uint8_t type;
    union {
        uint8_t version;  // when type == MSG_DEVINFO_V2
        uint8_t unused;   // when type == MSG_CLIENTCONFIG_V2 || type == MSG_SCAN_START || type == MSG_SCAN_STOP
        uint8_t up;       // when type == MSG_PACKET_V2
        uint8_t ap_count; // when type == MSG_SCAN_AP_GET
        uint8_t ap_index; // when type == MSG_SCAN_AP_GET
    };
    uint16_t size;
};

static const uint8_t uart_nic_protocol = WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N;

static const char *TAG = "uart_nic";

SemaphoreHandle_t uart_mtx = NULL;
static int s_retry_num = 0;
QueueHandle_t uart_tx_queue = 0;
QueueHandle_t wifi_egress_queue = 0;

static char intron[8] = {'U', 'N', '\x00', '\x01', '\x02', '\x03', '\x04', '\x05'};
#define MAC_LEN 6
static uint8_t mac[MAC_LEN];

static uint32_t now_seconds() {
    return xTaskGetTickCount() / configTICK_RATE_HZ;
}

static atomic_uint_least32_t last_inbound_seen = 0;
static atomic_bool associated = false;
static atomic_bool wifi_running = false;

static bool beacon_quirk;
static uint8_t probe_max_reties = 3;
static atomic_bool probe_in_progress = false;
static uint8_t probe_retry_count;
static uint8_t latest_ssid[SSID_LEN];
static uint8_t latest_bssid[BSSID_LEN];

typedef enum {
    SCAN_TYPE_UNKNOWN,
    SCAN_TYPE_PROBE,
    SCAN_TYPE_INCREMENTAL,
} ScanType;

typedef struct __attribute__((packed)) {
    uint8_t ssid[SSID_LEN];
    bool needs_password;
} ScanResult;

const ScanResult EMPTY_RESULT = {};

typedef void (*wifi_scan_callback)(wifi_ap_record_t *, int);

static struct {
    ScanType scan_type;
    wifi_scan_callback callback;
    uint32_t incremental_scan_time;
    ScanResult stored_ssids[SCAN_MAX_STORED_SSIDS];
    uint8_t stored_ssids_count;
    atomic_bool in_progress;
    atomic_bool should_reconnect;
} scan = {
    .scan_type = SCAN_TYPE_UNKNOWN,
    .callback = NULL,
    .incremental_scan_time = 0,
    .stored_ssids = {},
    .stored_ssids_count = 0,
    .in_progress = false,
    .should_reconnect = false,
};

typedef struct {
    size_t len;
    void *data;
    void *rx_buff;
} wifi_receive_buff;

typedef struct {
    size_t len;
    void *data;
} wifi_send_buff;

static void IRAM_ATTR free_wifi_receive_buff(wifi_receive_buff *buff) {
    if(buff->rx_buff) esp_wifi_internal_free_rx_buffer(buff->rx_buff);
    free(buff);
}

static void IRAM_ATTR free_wifi_send_buff(wifi_send_buff *buff) {
    if(buff->data) free(buff->data);
    free(buff);
}

static void send_link_status(uint8_t up) {
    ESP_LOGI(TAG, "Sending link status: %d", up);
    struct header header;
    header.type = MSG_PACKET_V2;
    header.up = up;
    header.size = htons(0);
    xSemaphoreTake(uart_mtx, portMAX_DELAY);
    uart_write_bytes(UART_NUM_0, intron, sizeof(intron));
    uart_write_bytes(UART_NUM_0, (const char*)&header, sizeof(header));
    xSemaphoreGive(uart_mtx);
}

static void do_wifi_scan() {
    wifi_scan_config_t config = {0};

    config.scan_type = WIFI_SCAN_TYPE_ACTIVE;
    switch (scan.scan_type) {
        case SCAN_TYPE_PROBE:
            config.show_hidden = true;
            config.scan_time.active.min = 120;
            config.scan_time.active.max = 300;
            break;
        case SCAN_TYPE_INCREMENTAL:
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

        case SCAN_TYPE_UNKNOWN:
        default:
            break;
    }

    ESP_ERROR_CHECK(esp_wifi_scan_start(&config, false));

    vTaskDelete(NULL);
}

static esp_err_t IRAM_ATTR start_wifi_scan(wifi_scan_callback callback, ScanType scan_type) {
    if (scan_type == SCAN_TYPE_UNKNOWN) {
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

    if (associated && scan_type != SCAN_TYPE_PROBE) {
        // The wifi scan behaves differently when the esp is connected to the AP.
        // Intentionally disconnect when starting a scan.
        // The connection will be automatically reestablished after the scan.
        esp_wifi_disconnect();
        associated = false;
    }

    xTaskCreate(&do_wifi_scan, "wifi_scan", 2048, NULL, tskIDLE_PRIORITY + 1, NULL);
    return ESP_OK;
}

static void IRAM_ATTR cache_current_ap_info() {
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        memcpy(latest_bssid, ap_info.bssid, BSSID_LEN);
        memcpy(latest_ssid, ap_info.ssid, SSID_LEN);
    }
}

static void probe_run();

static void IRAM_ATTR probe_handler(wifi_ap_record_t* aps, int ap_count) {
    bool found = false;

    if (associated) {
        cache_current_ap_info();
    }
    // Try to match BSSID first and if that fails go on and try SSID match . The BSSD check
    // should be sufficient, but there are APs that advertise mismatching BSSID in their
    // beacons and/or probe rensonse. That's the real culprit of the beacon timeout
    // disconnects and the primary motivation of this whole excercise.
    for (int i = 0; i < ap_count; ++i) {
        if (latest_bssid != NULL && 0 == memcmp(latest_bssid, aps[i].bssid, BSSID_LEN)) {
            found = true;
            beacon_quirk = false;
            break;
        }
    }
    if (beacon_quirk && !found) {
        for (int i = 0; i < ap_count; ++i) {
            if (latest_ssid != NULL && latest_ssid[0] && aps[i].ssid[0]) {
                if (0 == strncmp((char *)(latest_ssid), (char *)(aps[i].ssid), SSID_LEN)) {
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

static void IRAM_ATTR probe_run() {
    start_wifi_scan(&probe_handler, SCAN_TYPE_PROBE);
}

static void IRAM_ATTR handle_disconnect_and_try_reconnect() {
    associated = false;
    send_link_status(0);
    if (s_retry_num < CONFIG_ESP_MAXIMUM_RETRY) {
        esp_wifi_connect();
        s_retry_num++;
        ESP_LOGI(TAG, "retry to connect to the AP");
    }
    ESP_LOGI(TAG,"connect to the AP fail");
}

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        uint8_t current_protocol;
        ESP_ERROR_CHECK(esp_wifi_get_protocol(ESP_IF_WIFI_STA, &current_protocol));
        if (current_protocol != uart_nic_protocol) {
            ESP_ERROR_CHECK(esp_wifi_set_protocol(ESP_IF_WIFI_STA, uart_nic_protocol));
            return;
        }
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (scan.in_progress) {
            // We have intentionally disconnected from the wifi to make sure that the scan is not interupted by anything.
            // Lets handle the disconnection at the end of the scan.
            scan.should_reconnect = true;
        } else {
            handle_disconnect_and_try_reconnect();
        }

    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        last_inbound_seen = now_seconds();
        associated = true;
        beacon_quirk = true;
        send_link_status(1);
        s_retry_num = 0;
        ESP_ERROR_CHECK(esp_wifi_set_inactive_time(ESP_IF_WIFI_STA, INACTIVE_BEACON_SECONDS));
        cache_current_ap_info();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_SCAN_DONE) {
        scan.in_progress = false;
        const wifi_event_sta_scan_done_t *scan_data = (const wifi_event_sta_scan_done_t *)event_data;
        uint16_t ap_count = scan_data->number;

        if (!scan_data->status && ap_count) {
            wifi_ap_record_t *aps = (wifi_ap_record_t *)malloc(sizeof(wifi_ap_record_t) * ap_count);
            ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_count, aps));
            if (scan.callback != NULL) {
                scan.callback(aps, ap_count);
            }
            free(aps);
        } else if (scan.callback != NULL) {
            // if the scan failed, still call callback with no data to allow logic to redo the scan
            scan.callback(NULL, 0);
        }

        if (!scan.in_progress) {
            // If callback didn't start a new scan check the scan type, change it and rerun scan if possible
            switch(scan.scan_type) {
                case SCAN_TYPE_INCREMENTAL:
                    ESP_LOGI(TAG, "Restarting incremental scan");
                    start_wifi_scan(scan.callback, SCAN_TYPE_INCREMENTAL);
                break;
                default:
                    scan.scan_type = SCAN_TYPE_UNKNOWN;
                    if (scan.should_reconnect) {
                        scan.should_reconnect = false;
                        handle_disconnect_and_try_reconnect();
                    }
                 break;
            }
        }
    }
}

static int IRAM_ATTR wifi_receive_cb(void *buffer, uint16_t len, void *eb) {
    // ESP_LOGI(TAG, "Received wifi packet");

    // Seeing some traffic - we have signal :-)
    last_inbound_seen = now_seconds();

    // MAC filter
    if ((((const char*)buffer)[5] & 0x01) == 0) {
        for (uint i = 0; i < 6; ++i) {
            if(((const char*)buffer)[i] != mac[i]) {
                ESP_LOGI(TAG, "Dropping packet based on mac filter");
                goto cleanup;
            }
        }
    }

    wifi_receive_buff *buff = malloc(sizeof(wifi_receive_buff));
    if(!buff) {
        goto cleanup;
    }
    buff->len = len;
    buff->data = buffer;
    buff->rx_buff = eb;
    if (!xQueueSendToBack(uart_tx_queue, (void *)&buff, (TickType_t)0/*portMAX_DELAY*/)) {
        free_wifi_receive_buff(buff);
    }
    return 0;

cleanup:
    ESP_LOGI(TAG, "Failed to enqueue received wifi packet");
    esp_wifi_internal_free_rx_buffer(eb);
    return 0;
}

void wifi_init_sta(void) {
    esp_wifi_power_domain_on();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init_internal(&cfg));
    ESP_ERROR_CHECK(esp_supplicant_init());
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_wifi_internal_reg_rxcb(ESP_IF_WIFI_STA, wifi_receive_cb));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
}

static void send_device_info() {
    ESP_LOGI(TAG, "Sending device info");
    // MAC address
    int ret = esp_wifi_get_mac(WIFI_IF_STA, mac);
    if(ret != ESP_OK) {
        ESP_LOGI(TAG, "Failed to obtain MAC, returning last one or zeroes");
    }

    struct header header;
    header.type = MSG_DEVINFO_V2;
    header.version = FW_VERSION;
    header.size = htons(6);
    xSemaphoreTake(uart_mtx, portMAX_DELAY);
    uart_write_bytes(UART_NUM_0, intron, sizeof(intron));
    uart_write_bytes(UART_NUM_0, (const char*)&header, sizeof(header));
    uart_write_bytes(UART_NUM_0, (const char*)mac, sizeof(mac));
    xSemaphoreGive(uart_mtx);
}

static void IRAM_ATTR wait_for_intron() {
    // ESP_LOGI(TAG, "Waiting for intron");
    uint pos = 0;
    while(pos < sizeof(intron)) {
        char c;
        int read = uart_read_bytes(UART_NUM_0, (uint8_t*)&c, 1, portMAX_DELAY);
        if(read == 1) {
            if (c == intron[pos]) {
                pos++;
            } else {
                // ESP_LOGI(TAG, "Invalid: %c, val: %d\n", c, (int)c);
                pos = 0;
            }
        } else {
            ESP_LOGI(TAG, "Timeout!!!");
        }
    }
    // ESP_LOGI(TAG, "Intron found");
}

/**
 * @brief Read data from UART
 * 
 * @param buff Buffer to store the data
 * @param len Number of bytes to read
 * @return size_t Number of bytes actually read
 */
static size_t IRAM_ATTR read_uart(uint8_t *buff, size_t len) {
    size_t trr = 0;
    while(trr < len) {
        int read = uart_read_bytes(UART_NUM_0, ((uint8_t*)buff) + trr, len - trr, portMAX_DELAY);
        if(read < 0) {
            ESP_LOGI(TAG, "Failed to read from UART");
            if(trr != len) {
                ESP_LOGI(TAG, "Read %d != %d expected\n", trr, len);
            }
            return trr;
        }
        trr += read;
    }
    return trr;
}

static void IRAM_ATTR read_packet_message(uint16_t size) {
    // ESP_LOGI(TAG, "Receiving packet size: %d", size);
    // ESP_LOGI(TAG, "Allocating pbuf size: %d, free heap: %d", size, esp_get_free_heap_size());

    wifi_send_buff *buff = malloc(sizeof(wifi_send_buff));
    if(!buff) {
        goto nomem;
    }
    buff->len = size;
    buff->data = malloc(buff->len);
    if(!buff->data) {
        free(buff);
        goto nomem;
    }

    read_uart(buff->data, buff->len);

    if (!xQueueSendToBack(wifi_egress_queue, (void *)&buff, (TickType_t)0/*portMAX_DELAY*/)) {
        ESP_LOGI(TAG, "Out of space in egress queue");
        free_wifi_send_buff(buff);
    }
    return;

nomem:
    ESP_LOGI(TAG, "Out of mem for packet data");
    for(uint i = 0; i < size; ++i) {
        uint8_t c;
        read_uart(&c, 1);
    }
    return;
}

static void read_wifi_client_message() {
    read_uart((uint8_t*)intron, sizeof(intron));

    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config_t));

    uint8_t ssid_len = 0;
    read_uart(&ssid_len, 1);
    ESP_LOGI(TAG, "Reading SSID len: %d", ssid_len);
    if(ssid_len > sizeof(wifi_config.sta.ssid)) {
        ESP_LOGI(TAG, "SSID too long, trimming");
        ssid_len = sizeof(wifi_config.sta.ssid);
    }
    read_uart(wifi_config.sta.ssid, ssid_len);

    uint8_t pass_len = 0;
    read_uart(&pass_len, 1);
    ESP_LOGI(TAG, "Reading PASS len: %d", pass_len);
    if(pass_len > sizeof(wifi_config.sta.password)) {
        ESP_LOGI(TAG, "PASS too long, trimming");
        pass_len = sizeof(wifi_config.sta.password);
    }
    read_uart(wifi_config.sta.password, pass_len);

    ESP_LOGI(TAG, "Reconfiguring wifi");

    /* Setting a password implies station will connect to all security modes including WEP/WPA.
        * However these modes are deprecated and not advisable to be used. Incase your Access point
        * doesn't support WPA2, these mode can be enabled by commenting below line */
    if (strlen((char *)wifi_config.sta.password)) {
        wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    }
    wifi_config.sta.pmf_cfg.capable = 1;

    wifi_running = false;
    esp_wifi_stop();
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start());
    wifi_running = true;
    send_device_info();
}

static int get_link_status() {
    static wifi_ap_record_t ap_info;
    esp_err_t ret = esp_wifi_sta_get_ap_info(&ap_info);
    // ap_info is not important, just not receiven ESP_ERR_WIFI_NOT_CONNECT means we are associated
    const bool online = ret == ESP_OK;
    associated = online;
    return online;
}

static void check_online_status() {
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

static void IRAM_ATTR clear_stored_ssids() {
    scan.stored_ssids_count = 0;
}

static void IRAM_ATTR store_scanned_ssids(wifi_ap_record_t *aps, int ap_count) {
    for (uint8_t i = 0; i < ap_count; ++i) {
        bool found = false;
        if (scan.stored_ssids_count == SCAN_MAX_STORED_SSIDS) {
            ESP_LOGW(TAG, "Scan result storage is full");
            break;
        }
        for (uint8_t j = 0; j < scan.stored_ssids_count; ++j) {
            ESP_LOGI(TAG, "Comparing >%s< and %s", (char *)scan.stored_ssids[j].ssid, (char *)aps[i].ssid);
            if (memcmp(scan.stored_ssids[j].ssid, aps[i].ssid, SSID_LEN) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            memcpy(scan.stored_ssids[scan.stored_ssids_count].ssid, aps[i].ssid, SSID_LEN);
            scan.stored_ssids[scan.stored_ssids_count].needs_password = aps[i].authmode != WIFI_AUTH_OPEN || aps[i].pairwise_cipher != WIFI_CIPHER_TYPE_NONE;
            scan.stored_ssids_count ++;
            ESP_LOGI(TAG, "Found SSID: %s", aps[i].ssid);
        }
    }

    struct header header = {0};
    header.type = MSG_SCAN_AP_CNT;
    header.ap_count = scan.stored_ssids_count;
    xSemaphoreTake(uart_mtx, portMAX_DELAY);
    uart_write_bytes(UART_NUM_0, intron, sizeof(intron));
    uart_write_bytes(UART_NUM_0, (const char*)&header, sizeof(header));
    xSemaphoreGive(uart_mtx);

    ESP_LOGI(TAG, "Scan done. Found: %d", scan.stored_ssids_count);
}

static void IRAM_ATTR send_scanned_ssid(uint8_t index, const ScanResult* scan) {
    struct header header = {0};
    header.type = MSG_SCAN_AP_GET;
    header.ap_index = index;
    header.size = htons(sizeof(ScanResult));
    xSemaphoreTake(uart_mtx, portMAX_DELAY);
    uart_write_bytes(UART_NUM_0, intron, sizeof(intron));
    uart_write_bytes(UART_NUM_0, (const char*)&header, sizeof(header));
    uart_write_bytes(UART_NUM_0, (const char*)scan, sizeof(ScanResult));
    xSemaphoreGive(uart_mtx);
}

static void IRAM_ATTR handle_rx_msg_scan_start() {
    clear_stored_ssids();
    ESP_LOGI(TAG, "Starting scan...");

    start_wifi_scan(&store_scanned_ssids, SCAN_TYPE_INCREMENTAL);
}

static void IRAM_ATTR handle_rx_msg_scan_stop() {
    // Response is send automatically after scan is stopped
    scan.in_progress = false;
    scan.scan_type = SCAN_TYPE_UNKNOWN;
    scan.incremental_scan_time = 0;
    ESP_ERROR_CHECK(esp_wifi_scan_stop());
}

static void IRAM_ATTR handle_rx_msg_scan_get(struct header header) {
    if (header.ap_index < scan.stored_ssids_count) {
        send_scanned_ssid(header.ap_index, &scan.stored_ssids[header.ap_index]);
    } else {
        send_scanned_ssid(UINT8_MAX, &EMPTY_RESULT);
    }
}

static void IRAM_ATTR read_message() {
    wait_for_intron();

    struct header header;
    size_t read = uart_read_bytes(UART_NUM_0, (uint8_t*)&header, sizeof(header), portMAX_DELAY);
    if (read != sizeof(header)) {
        ESP_LOGI(TAG, "Cannot read message header");
        return;
    }

    header.size = ntohs(header.size);

    switch (header.type) {
        case MSG_PACKET_V2: {
            if (header.size > 0) {
                read_packet_message(header.size);
            } else {
                send_link_status(get_link_status());
            }
        }
        break;
        case MSG_CLIENTCONFIG_V2:
            read_wifi_client_message();
        break;
        case MSG_SCAN_START:
            handle_rx_msg_scan_start();
        break;
        case MSG_SCAN_STOP:
            handle_rx_msg_scan_stop();
        break;
        case MSG_SCAN_AP_CNT:
            ESP_LOGE(TAG, "MSG_SCAN_AP_CNT is only transmitted, never recieved");
        break;
        case MSG_SCAN_AP_GET:
            handle_rx_msg_scan_get(header);
        break;
        case MSG_DEVINFO_V2:
            ESP_LOGE(TAG, "MSG_DEVINFO_V2 is only transmitted, never recieved");
        break;
        default:
            ESP_LOGE(TAG, "Unknown message type: %d !!!", header.type);
        break;
    }

    // Check that we are receiving some packets from the AP. We do so in the
    // thread that receives messages from the main CPU because we know that one
    // will generate a message from time to time (at least the get-link one).
    // On the other hand, if we lose connectivity, we will receive no packets
    // from the AP and we would block forever and never get to the check.
    check_online_status();


}

static void IRAM_ATTR wifi_egress_thread(void *arg) {
    for(;;) {
        wifi_send_buff *buff;
        if(xQueueReceive(wifi_egress_queue, &buff, (TickType_t)portMAX_DELAY)) {
            if (!buff) {
                ESP_LOGI(TAG, "NULL pulled from egress queue");
                continue;
            }

            int8_t err = esp_wifi_internal_tx(ESP_IF_WIFI_STA, buff->data, buff->len);
            if (err != ESP_OK) {
                ESP_LOGI(TAG, "Failed to send packet !!!");
            }
            free_wifi_send_buff(buff);
        }
    }
}

static void IRAM_ATTR output_rx_thread(void *arg) {
    ESP_LOGI(TAG, "Started RX thread");
    for(;;) {
        read_message();
    }
}

static void IRAM_ATTR uart_tx_thread(void *arg) {
    // Send initial device info to let master know ESP is ready
    send_device_info();

    for(;;) {
        wifi_receive_buff *buff;
        if(xQueueReceive(uart_tx_queue, &buff, (TickType_t)1000 /*portMAX_DELAY*/)) {
            if (!buff) {
                ESP_LOGI(TAG, "Skipping packet with null buffer");
                continue;
            }
            // ESP_LOGI(TAG, "Printing packet to UART");
            struct header header;
            header.type = MSG_PACKET_V2;
            header.up = get_link_status();
            header.size = htons(buff->len);
            xSemaphoreTake(uart_mtx, portMAX_DELAY);
            uart_write_bytes(UART_NUM_0, intron, sizeof(intron));
            uart_write_bytes(UART_NUM_0, (const char*)&header, sizeof(header));
            uart_write_bytes(UART_NUM_0, (const char*)buff->data, buff->len);
            xSemaphoreGive(uart_mtx);
            // ESP_LOGI(TAG, "Packet UART out done");
            free_wifi_receive_buff(buff);
        }
    }
}

void app_main() {
    ESP_LOGI(TAG, "UART NIC");

	// esp_log_level_set("*", ESP_LOG_ERROR);

    ESP_ERROR_CHECK(nvs_flash_init());

    uart_set_pin(UART_NUM_0, 1, 3, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    // Configure parameters of an UART driver,
    // communication pins and install the driver
    uart_config_t uart_config = {
        .baud_rate = 4600000,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_driver_install(UART_NUM_0, 16384, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_0, &uart_config);
    uart_intr_config_t uart_intr = {
        .intr_enable_mask = UART_RXFIFO_FULL_INT_ENA_M
        | UART_RXFIFO_TOUT_INT_ENA_M
        | UART_FRM_ERR_INT_ENA_M
        | UART_RXFIFO_OVF_INT_ENA_M,
        .rxfifo_full_thresh = 80,
        .rx_timeout_thresh = 1,
        .txfifo_empty_intr_thresh = 40
    };
    uart_intr_config(UART_NUM_0, &uart_intr);

    ESP_LOGI(TAG, "UART RE-INITIALIZED");

    uart_mtx = xSemaphoreCreateMutex();
    if (!uart_mtx) {
        ESP_LOGI(TAG, "Could not create UART mutex");
        return;
    }

    uart_tx_queue = xQueueCreate(20, sizeof(wifi_receive_buff*));
    if (uart_tx_queue == 0) {
        ESP_LOGI(TAG, "Failed to create INPUT/TX queue");
        return;
    }

    wifi_egress_queue = xQueueCreate(20, sizeof(wifi_send_buff*));
    if (wifi_egress_queue == 0) {
        ESP_LOGI(TAG, "Failed to create WiFi TX queue");
        return;
    }

    scan.stored_ssids_count = SCAN_MAX_STORED_SSIDS;
    clear_stored_ssids();
    memset(latest_ssid, 0, SSID_LEN);
    memset(latest_bssid, 0, BSSID_LEN);

    ESP_LOGI(TAG, "Wifi init");
    esp_wifi_restore();
    wifi_init_sta();
    esp_wifi_set_ps(WIFI_PS_NONE);

    ESP_LOGI(TAG, "Creating RX thread");
    xTaskCreate(&output_rx_thread, "output_rx_thread", 4096, NULL, 1, NULL);
    ESP_LOGI(TAG, "Creating WiFi-out thread");
    xTaskCreate(&wifi_egress_thread, "wifi_egress_thread", 2048, NULL, 12, NULL);
    ESP_LOGI(TAG, "Creating TX thread");
    xTaskCreate(&uart_tx_thread, "uart_tx_thread", 2048, NULL, 14, NULL);
}
