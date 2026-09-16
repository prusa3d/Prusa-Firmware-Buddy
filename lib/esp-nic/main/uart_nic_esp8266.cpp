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

#include <string.h>
#include <atomic>
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
#include "esp_aio.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "uart0_driver.hpp"

#include "esp_private/wifi.h"

#include "common.hpp"

// Externals with no header
extern "C" {
esp_err_t mac_init(void);
esp_err_t esp_supplicant_init(void);
}

// Note: `uart0_tx_queue` is a FreeRTOS queue of `uart0_tx_queue_item` elements.
//       Send items to this queue in order to transmit them via UART0
//       from ESP to printer. The queue is drained by realtime priority
//       task `uart0_tx_task`.
struct Uart0TxQueueItem {
    Header header;
    uint8_t *data;
    void *rx_buffer; // Note: This is some internal ESP buffer which we are not
                     //       sure if we can free before data is transmitted.
};
static QueueHandle_t uart0_tx_queue = nullptr;

// Note: `uart0_rx_queue` is a FreeRTOS queue of `uart0_rx_queue_item` elements.
//       Items are send to this queue from realtime priority task `uart0_rx_task`
//       whenever they are received via UART0 by ESP from the printer. The queue
//       is drained by lower priority task `main_task`.
struct Uart0RxQueueItem {
    void (*callback)(uint8_t *, Header);
    uint8_t *data;
    Header header;
};
static QueueHandle_t uart0_rx_queue = 0;

static void uart0_rx_skip_bytes(size_t size) {
    uint8_t c;
    for (uint32_t i = 0; i < size; ++i) {
        uart0_rx_bytes(&c, 1);
    }
}

// Note: We are using single global buffer here. It has two main parts:
//        * intron is read-only most of the time. The only write access is synchronized by means of critical section.
//        * header is used only by `uart0_tx_task`
union Message {
    uint8_t bytes[12];
    struct {
        uint8_t intron[8];
        Header header;
    };
};
static union Message tx_message = { .bytes = { 'U', 'N', '\x00', '\x01', '\x02', '\x03', '\x04', '\x05', 0, 0, 0, 0 } };

void IRAM_ATTR send_link_status(int8_t status) {
    Uart0TxQueueItem queue_item;
    queue_item.header.type = static_cast<uint8_t>(MsgType::packet);
    queue_item.header.link_status = status;
    queue_item.header.size = htons(0);
    queue_item.data = nullptr;
    queue_item.rx_buffer = nullptr;
    if (xQueueSendToBack(uart0_tx_queue, &queue_item, 0) != pdTRUE) {
        ESP_LOGE(TAG, "xQueueSendToBack failed (uart0tx)");
    }
}

void IRAM_ATTR handle_disconnect_and_try_reconnect() {
    associated = false;
    send_link_status(0);
    if (s_retry_num < CONFIG_ESP_MAXIMUM_RETRY) {
        start_wifi_connect_task();
        s_retry_num++;
        ESP_LOGI(TAG, "retry to connect to the AP");
        ESP_LOGI(TAG, "connect to the AP fail, now lowering RF power to reduce interference");
        esp_wifi_set_max_tx_power(48); // 12dB (down from 20dB) to reduce antenna reflections, needed for some modules (see ESP8266_RTOS_SDK#1200)
    } else {
        connecting = false;
    }
}

static int8_t get_link_status();

static int IRAM_ATTR wifi_receive_cb(void *buffer, uint16_t len, void *eb) {

    // Seeing some traffic - we have signal :-)
    last_inbound_seen = now_seconds();

    // MAC filter
    if ((((const char *)buffer)[5] & 0x01) == 0) {
        for (uint i = 0; i < 6; ++i) {
            if (((const char *)buffer)[i] != mac[i]) {
                goto cleanup;
            }
        }
    }

    Uart0TxQueueItem queue_item;
    queue_item.header.type = static_cast<uint8_t>(MsgType::packet);
    queue_item.header.link_status = get_link_status();
    queue_item.header.size = htons(len);
    queue_item.data = (uint8_t *)buffer;
    queue_item.rx_buffer = eb;
    if (xQueueSendToBack(uart0_tx_queue, &queue_item, 0) != pdTRUE) {
        ESP_LOGE(TAG, "xQueueSendToBack failed (uart0tx)");
    } else {
        return ESP_OK;
    }
cleanup:
    free(buffer);
    esp_wifi_internal_free_rx_buffer(eb);
    return ESP_FAIL;
}

void IRAM_ATTR wifi_init_sta(void) {
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(mac_init());
    esp_wifi_set_rx_pbuf_mem_type(WIFI_RX_PBUF_DRAM);
    ESP_ERROR_CHECK(esp_wifi_init_internal(&cfg));
    ESP_ERROR_CHECK(esp_supplicant_init());
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, nullptr));
    ESP_ERROR_CHECK(esp_wifi_internal_reg_rxcb(ESP_IF_WIFI_STA, wifi_receive_cb));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
}

static void IRAM_ATTR send_device_info() {
    esp_wifi_get_mac(WIFI_IF_STA, mac); // ignore error

    Uart0TxQueueItem queue_item;
    queue_item.header.type = static_cast<uint8_t>(MsgType::devinfo);
    queue_item.header.version = FW_VERSION;
    queue_item.header.size = htons(6);
    queue_item.data = nullptr;
    queue_item.rx_buffer = nullptr;
    if (xQueueSendToBack(uart0_tx_queue, &queue_item, 0) != pdTRUE) {
        ESP_LOGE(TAG, "xQueueSendToBack failed (uart0tx)");
    }
}

static void IRAM_ATTR wait_for_intron() {
    // Hope for the best...
    uint8_t intron[8];
    uart0_rx_bytes(intron, 8);
    while (memcmp(intron, tx_message.intron, 8) != 0) {
        // ...but be prepared for the worst.
        for (int i = 0; i < 7; ++i) {
            intron[i] = intron[i + 1];
        }
        uart0_rx_bytes(&intron[7], 1);
    }
}

static int8_t IRAM_ATTR get_link_status() {
    static wifi_ap_record_t ap_info = {};
    esp_err_t ret = esp_wifi_sta_get_ap_info(&ap_info);
    const bool online = ret == ESP_OK;
    associated = online;
    int8_t signal = ap_info.rssi;
    if (signal >= 0) {
        // Cap the signal strangth to "strong" to preserve back compatibility.
        // (signal this strong is mostly unheard of in practice anyway, but
        // theoretically possible)
        signal = -1;
    }
    return online ? signal : 0;
}

static void IRAM_ATTR handle_rx_msg_packet_v2(uint8_t *data, Header header) {
    if (header.size == 0) {
        send_link_status(get_link_status());
    } else {
        esp_wifi_internal_tx(ESP_IF_WIFI_STA, data, header.size);
        free(data);
    }
}

static void IRAM_ATTR handle_rx_msg_clientconfig_v2(uint8_t *data, [[maybe_unused]] Header header) {
    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config_t));

    {
        taskENTER_CRITICAL();
        memcpy(tx_message.intron, data, sizeof(tx_message.intron));
        taskEXIT_CRITICAL();
        data += sizeof(tx_message.intron);
    }
    uint8_t ssid_length = 0;
    {
        memcpy(&ssid_length, data, sizeof(ssid_length));
        data += sizeof(ssid_length);
        size_t memcpy_size = ssid_length < sizeof(wifi_config.sta.ssid)
            ? ssid_length
            : sizeof(wifi_config.sta.ssid);
        memcpy(wifi_config.sta.ssid, data, memcpy_size);
        data += ssid_length;
    }
    uint8_t password_length = 0;
    {
        memcpy(&password_length, data, sizeof(password_length));
        data += sizeof(password_length);
        size_t memcpy_size = password_length < sizeof(wifi_config.sta.password)
            ? password_length
            : sizeof(wifi_config.sta.password);
        memcpy(wifi_config.sta.password, data, memcpy_size);
        data += password_length;
    }

    /* Setting a password implies station will connect to all security modes including WEP/WPA.
     * However these modes are deprecated and not advisable to be used. Incase your Access point
     * doesn't support WPA2, these mode can be enabled by commenting below line */
    if (password_length) {
        wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    }
    wifi_config.sta.pmf_cfg.capable = 1;

    // If scan is in progress we need to stop it manually here to prevent reconnect to previous AP.
    if (scan.in_progress) {
        scan.should_reconnect = false;
        force_stop_wifi_scan();
    }

    wifi_running = false;
    esp_wifi_stop();
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    wifi_running = true;
    send_device_info();
}

static void IRAM_ATTR handle_rx_msg_unknown(uint8_t *data, [[maybe_unused]] Header header) {
    if (data) {
        free(data);
    }
}

void IRAM_ATTR send_number_of_ssids() {
    Uart0TxQueueItem queue_item {};
    queue_item.header.type = static_cast<uint8_t>(MsgType::scan_ap_count);
    queue_item.header.ap_count = scan.stored_ssids_count;
    if (xQueueSendToBack(uart0_tx_queue, &queue_item, 0) != pdTRUE) {
        ESP_LOGE(TAG, "xQueueSendToBack failed (uart0tx)");
    }
}

void IRAM_ATTR send_scanned_ssid(uint8_t index, const ScanResult *ap_info) {
    Uart0TxQueueItem queue_item {};
    queue_item.header.type = static_cast<uint8_t>(MsgType::scan_ap_get);
    queue_item.header.ap_index = index;
    queue_item.header.size = htons(sizeof(*ap_info));
    queue_item.data = (uint8_t *)(ap_info);

    if (xQueueSendToBack(uart0_tx_queue, &queue_item, 0) != pdTRUE) {
        ESP_LOGE(TAG, "xQueueSendToBack failed (uart0tx)");
    }
}

static void IRAM_ATTR read_message() {
    wait_for_intron();
    Uart0RxQueueItem queue_item {};
    uart0_rx_bytes((uint8_t *)&queue_item.header, sizeof(queue_item.header));

    switch (queue_item.header.type) {
    case static_cast<uint8_t>(MsgType::packet):
        queue_item.callback = handle_rx_msg_packet_v2;
        break;
    case static_cast<uint8_t>(MsgType::client_config):
        queue_item.callback = handle_rx_msg_clientconfig_v2;
        break;
    case static_cast<uint8_t>(MsgType::scan_start):
        queue_item.callback = handle_rx_msg_scan_start;
        break;
    case static_cast<uint8_t>(MsgType::scan_stop):
        queue_item.callback = handle_rx_msg_scan_stop;
        break;
    case static_cast<uint8_t>(MsgType::scan_ap_count):
        ESP_LOGE(TAG, "MsgType::scan_ap_count is only transmitted, never recieved");
        queue_item.callback = handle_rx_msg_unknown;
        break;
    case static_cast<uint8_t>(MsgType::scan_ap_get):
        queue_item.callback = handle_rx_msg_scan_get;
        break;
    case static_cast<uint8_t>(MsgType::devinfo):
        ESP_LOGE(TAG, "MSG_DEVINFIO_V2 is only transmitted, never recieved");
        queue_item.callback = handle_rx_msg_unknown;
        break;
    default:
        queue_item.callback = handle_rx_msg_unknown;
        break;
    }

    queue_item.header.size = ntohs(queue_item.header.size);
    if (queue_item.header.size != 0 && queue_item.header.size <= 2000) {
        queue_item.data = (uint8_t *)malloc(queue_item.header.size);
        if (queue_item.data) {
            uart0_rx_bytes(queue_item.data, queue_item.header.size);
        } else {
            uart0_rx_skip_bytes(queue_item.header.size);
        }
    } else {
        queue_item.data = nullptr;
        uart0_rx_skip_bytes(queue_item.header.size);
    }

    if (xQueueSendToBack(uart0_rx_queue, &queue_item, 0) != pdTRUE) {
        ESP_LOGE(TAG, "xQueueSendToBack failed (uart0rx)");
        if (queue_item.data) {
            free(queue_item.data);
        }
    }
}

static void IRAM_ATTR uart0_rx_task([[maybe_unused]] void *arg) {
    // wait for uart0_driver_install() to finish
    (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    for (;;) {
        read_message();
    }
}

static void IRAM_ATTR main_task([[maybe_unused]] void *arg) {
    // Wait because printer sends reset for whatever reason.
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    // Send initial device info to let master know ESP is ready
    send_device_info();

    for (;;) {
        Uart0RxQueueItem queue_item;
        if (xQueueReceive(uart0_rx_queue, &queue_item, portMAX_DELAY) == pdTRUE) {
            (*queue_item.callback)(queue_item.data, queue_item.header);
            check_online_status();
        }
    }
}

static void IRAM_ATTR uart0_tx_task([[maybe_unused]] void *arg) {
    // wait for uart0_driver_install() to finish
    (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    // consume messages from uart0_tx_queue, forever
    for (;;) {
        Uart0TxQueueItem queue_item;
        if (xQueueReceive(uart0_tx_queue, &queue_item, portMAX_DELAY) == pdTRUE) {

            // send fix-sized part of the message (intron + header)
            tx_message.header = queue_item.header;
            uart0_tx_bytes(tx_message.bytes, sizeof(tx_message.bytes));

            // send variable-sized part of the message (payload depending on message type)
            switch (MsgType { queue_item.header.type }) {
            case MsgType::packet: {
                // size may be empty when we are using packet to only send link status
                uint16_t size = ntohs(queue_item.header.size);
                if (size) {
                    uart0_tx_bytes(queue_item.data, size);
                    free(queue_item.data);
                }
                if (queue_item.rx_buffer) {
                    free(queue_item.rx_buffer);
                }
            } break;
            case MsgType::devinfo:
                uart0_tx_bytes(mac, sizeof(mac));
                break;
            case MsgType::client_config:
                ESP_LOGE(TAG, "MsgType::client_config is only received, never transmitted");
                break;
            case MsgType::scan_start:
                ESP_LOGE(TAG, "MsgType::scan_start is only received, never transmitted");
                break;
            case MsgType::scan_ap_count:
                // no data to send
                break;
            case MsgType::scan_stop:
                ESP_LOGE(TAG, "MsgType::scan_stop is only received, never transmitted");
                break;
            case MsgType::scan_ap_get:
                uart0_tx_bytes(queue_item.data, ntohs(queue_item.header.size));
                break;
            }
        }
    }
}

extern "C" void IRAM_ATTR app_main() {
    ESP_LOGI(TAG, "UART NIC");

    esp_log_level_set("*", ESP_LOG_ERROR);

    ESP_ERROR_CHECK(nvs_flash_init());

    ESP_LOGI(TAG, "Wifi init");
    esp_wifi_restore();
    wifi_init_sta();
    esp_wifi_set_ps(WIFI_PS_NONE);

    uart0_rx_queue = xQueueCreate(20, sizeof(Uart0RxQueueItem));
    if (uart0_rx_queue == 0) {
        ESP_LOGE(TAG, "xQueueCreate failed (uart0rx)");
        abort();
    }
    uart0_tx_queue = xQueueCreate(20, sizeof(Uart0TxQueueItem));
    if (uart0_tx_queue == 0) {
        ESP_LOGE(TAG, "xQueueCreate failed (uart0tx)");
        abort();
    }

    // Initialize scan ap storage
    scan.stored_ssids_count = SCAN_MAX_STORED_SSIDS;
    clear_stored_ssids();
    memset(latest_bssid, 0, BSSID_MAX_LEN);
    memset(latest_ssid, 0, SSID_MAX_LEN);

    TaskHandle_t uart0_rx_task_handle;
    TaskHandle_t uart0_tx_task_handle;
    if (xTaskCreate(uart0_rx_task, "uart0rx", 1024, nullptr, 14, &uart0_rx_task_handle) != pdPASS) {
        ESP_LOGE(TAG, "xTaskCreate failed (uart0rx)");
        abort();
    }
    if (xTaskCreate(uart0_tx_task, "uart0tx", 1024, nullptr, 14, &uart0_tx_task_handle) != pdPASS) {
        ESP_LOGE(TAG, "xTaskCreate failed (uart0tx)");
        abort();
    }
    if (uart0_driver_install(uart0_rx_task_handle, uart0_tx_task_handle) != ESP_OK) {
        ESP_LOGE(TAG, "uart0_driver_install failed");
        abort();
    }
    if (xTaskCreate(main_task, "main", 2048, nullptr, 12, nullptr) != pdPASS) {
        ESP_LOGE(TAG, "xTaskCreate failed (main)");
        abort();
    }

    // Note: These are not the only tasks running. There are also ESP system tasks present, see
    //       https://docs.espressif.com/projects/esp8266-rtos-sdk/en/latest/api-guides/system-tasks.html
}
