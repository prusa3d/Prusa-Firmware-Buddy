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
#include "nvs.h"
#include "nvs_flash.h"

#include "driver/gpio.h"
#include "driver/uart.h"
#include "soc/uart_reg.h"

#include "esp_private/wifi.h"
#include "esp_wpa.h"
#include <arpa/inet.h>

#include "common.hpp"

SemaphoreHandle_t uart_mtx = nullptr;
QueueHandle_t uart_tx_queue = 0;
QueueHandle_t wifi_egress_queue = 0;

static char intron[8] = { 'U', 'N', '\x00', '\x01', '\x02', '\x03', '\x04', '\x05' };

struct WifiReceiveBuff {
    size_t len;
    void *data;
    void *rx_buff;
};

struct WifiSendBuff {
    size_t len;
    void *data;
};

static void IRAM_ATTR free_wifi_receive_buff(WifiReceiveBuff *buff) {
    if (buff->rx_buff) {
        esp_wifi_internal_free_rx_buffer(buff->rx_buff);
    }
    free(buff);
}

static void IRAM_ATTR free_wifi_send_buff(WifiSendBuff *buff) {
    if (buff->data) {
        free(buff->data);
    }
    free(buff);
}

void send_link_status(int8_t status) {
    ESP_LOGI(TAG, "Sending link status: %d", status);
    Header header;
    header.type = static_cast<uint8_t>(MsgType::packet);
    header.link_status = status;
    header.size = htons(0);
    xSemaphoreTake(uart_mtx, portMAX_DELAY);
    uart_write_bytes(UART_NUM_0, intron, sizeof(intron));
    uart_write_bytes(UART_NUM_0, (const char *)&header, sizeof(header));
    xSemaphoreGive(uart_mtx);
}

void IRAM_ATTR handle_disconnect_and_try_reconnect() {
    associated = false;
    send_link_status(0);
    if (s_retry_num < CONFIG_ESP_MAXIMUM_RETRY) {
        start_wifi_connect_task();
        s_retry_num++;
        ESP_LOGI(TAG, "retry to connect to the AP");
    } else {
        ESP_LOGI(TAG, "connect to the AP fail");
        connecting = false;
    }
}

static int IRAM_ATTR wifi_receive_cb(void *buffer, uint16_t len, void *eb) {
    // ESP_LOGI(TAG, "Received wifi packet");
    WifiReceiveBuff *buff = nullptr;

    // Seeing some traffic - we have signal :-)
    last_inbound_seen = now_seconds();

    // MAC filter
    if ((((const char *)buffer)[5] & 0x01) == 0) {
        for (uint i = 0; i < 6; ++i) {
            if (((const char *)buffer)[i] != mac[i]) {
                ESP_LOGI(TAG, "Dropping packet based on mac filter");
                goto cleanup;
            }
        }
    }

    buff = (WifiReceiveBuff *)malloc(sizeof(WifiReceiveBuff));
    if (!buff) {
        goto cleanup;
    }
    buff->len = len;
    buff->data = buffer;
    buff->rx_buff = eb;
    if (!xQueueSendToBack(uart_tx_queue, (void *)&buff, (TickType_t)0 /*portMAX_DELAY*/)) {
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
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, nullptr));
    ESP_ERROR_CHECK(esp_wifi_internal_reg_rxcb(WIFI_IF_STA, wifi_receive_cb));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
}

static void send_device_info() {
    ESP_LOGI(TAG, "Sending device info");
    // MAC address
    int ret = esp_wifi_get_mac(WIFI_IF_STA, mac);
    if (ret != ESP_OK) {
        ESP_LOGI(TAG, "Failed to obtain MAC, returning last one or zeroes");
    }

    Header header;
    header.type = static_cast<uint8_t>(MsgType::devinfo);
    header.version = FW_VERSION;
    header.size = htons(6);
    xSemaphoreTake(uart_mtx, portMAX_DELAY);
    uart_write_bytes(UART_NUM_0, intron, sizeof(intron));
    uart_write_bytes(UART_NUM_0, (const char *)&header, sizeof(header));
    uart_write_bytes(UART_NUM_0, (const char *)mac, sizeof(mac));
    xSemaphoreGive(uart_mtx);
}

static void IRAM_ATTR wait_for_intron() {
    // ESP_LOGI(TAG, "Waiting for intron");
    uint pos = 0;
    while (pos < sizeof(intron)) {
        char c;
        int read = uart_read_bytes(UART_NUM_0, (uint8_t *)&c, 1, portMAX_DELAY);
        if (read == 1) {
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
    while (trr < len) {
        int read = uart_read_bytes(UART_NUM_0, ((uint8_t *)buff) + trr, len - trr, portMAX_DELAY);
        if (read < 0) {
            ESP_LOGI(TAG, "Failed to read from UART");
            if (trr != len) {
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

    WifiSendBuff *buff = (WifiSendBuff *)malloc(sizeof(WifiSendBuff));
    if (!buff) {
        goto nomem;
    }
    buff->len = size;
    buff->data = malloc(buff->len);
    if (!buff->data) {
        free(buff);
        goto nomem;
    }

    read_uart((uint8_t *)(buff->data), buff->len);

    if (!xQueueSendToBack(wifi_egress_queue, (void *)&buff, (TickType_t)0 /*portMAX_DELAY*/)) {
        ESP_LOGI(TAG, "Out of space in egress queue");
        free_wifi_send_buff(buff);
    }
    return;

nomem:
    ESP_LOGI(TAG, "Out of mem for packet data");
    for (uint i = 0; i < size; ++i) {
        uint8_t c;
        read_uart(&c, 1);
    }
    return;
}

static void read_wifi_client_message() {
    read_uart((uint8_t *)intron, sizeof(intron));

    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config_t));

    uint8_t ssid_len = 0;
    read_uart(&ssid_len, 1);
    ESP_LOGI(TAG, "Reading SSID len: %d", ssid_len);
    if (ssid_len > sizeof(wifi_config.sta.ssid)) {
        ESP_LOGI(TAG, "SSID too long, trimming");
        ssid_len = sizeof(wifi_config.sta.ssid);
    }
    read_uart(wifi_config.sta.ssid, ssid_len);

    uint8_t pass_len = 0;
    read_uart(&pass_len, 1);
    ESP_LOGI(TAG, "Reading PASS len: %d", pass_len);
    if (pass_len > sizeof(wifi_config.sta.password)) {
        ESP_LOGI(TAG, "PASS too long, trimming");
        pass_len = sizeof(wifi_config.sta.password);
    }
    read_uart(wifi_config.sta.password, pass_len);

    ESP_LOGI(TAG, "Reconfiguring wifi");

    /* Setting a password implies station will connect to all security modes including WEP/WPA.
     * However these modes are deprecated and not advisable to be used. Incase your Access point
     * doesn't support WPA2, these mode can be enabled by commenting below line */
    if (pass_len) {
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
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    wifi_running = true;
    send_device_info();
}

static int8_t get_link_status() {
    static wifi_ap_record_t ap_info = {};
    esp_err_t ret = esp_wifi_sta_get_ap_info(&ap_info);
    // ap_info is not important, just not receiven ESP_ERR_WIFI_NOT_CONNECT means we are associated
    int8_t signal = ap_info.rssi;
    if (signal >= 0) {
        // Cap the signal strength to "strong" to preserve back compatibility.
        // (signal this strong is mostly unheard of in practice anyway, but
        // theoretically possible)
        signal = -1;
    }
    const bool online = ret == ESP_OK;
    associated = online;
    return online ? signal : 0;
}

void IRAM_ATTR send_number_of_ssids() {
    Header header {};
    header.type = static_cast<uint8_t>(MsgType::scan_ap_count);
    header.ap_count = scan.stored_ssids_count;
    xSemaphoreTake(uart_mtx, portMAX_DELAY);
    uart_write_bytes(UART_NUM_0, intron, sizeof(intron));
    uart_write_bytes(UART_NUM_0, (const char *)&header, sizeof(header));
    xSemaphoreGive(uart_mtx);
}

void IRAM_ATTR send_scanned_ssid(uint8_t index, const ScanResult *scan) {
    Header header {};
    header.type = static_cast<uint8_t>(MsgType::scan_ap_get);
    header.ap_index = index;
    header.size = htons(sizeof(ScanResult));
    xSemaphoreTake(uart_mtx, portMAX_DELAY);
    uart_write_bytes(UART_NUM_0, intron, sizeof(intron));
    uart_write_bytes(UART_NUM_0, (const char *)&header, sizeof(header));
    uart_write_bytes(UART_NUM_0, (const char *)scan, sizeof(ScanResult));
    xSemaphoreGive(uart_mtx);
}

static void IRAM_ATTR read_message() {
    wait_for_intron();

    Header header;
    size_t read = uart_read_bytes(UART_NUM_0, (uint8_t *)&header, sizeof(header), portMAX_DELAY);
    if (read != sizeof(header)) {
        ESP_LOGI(TAG, "Cannot read message header");
        return;
    }

    header.size = ntohs(header.size);

    switch (header.type) {
    case static_cast<uint8_t>(MsgType::packet): {
        if (header.size > 0) {
            read_packet_message(header.size);
        } else {
            send_link_status(get_link_status());
        }
    } break;
    case static_cast<uint8_t>(MsgType::client_config):
        read_wifi_client_message();
        break;
    case static_cast<uint8_t>(MsgType::scan_start):
        handle_rx_msg_scan_start(nullptr, {});
        break;
    case static_cast<uint8_t>(MsgType::scan_stop):
        handle_rx_msg_scan_stop(nullptr, {});
        break;
    case static_cast<uint8_t>(MsgType::scan_ap_count):
        ESP_LOGE(TAG, "MsgType::scan_ap_count is only transmitted, never recieved");
        break;
    case static_cast<uint8_t>(MsgType::scan_ap_get):
        handle_rx_msg_scan_get(nullptr, header);
        break;
    case static_cast<uint8_t>(MsgType::devinfo):
        ESP_LOGE(TAG, "MsgType::devinfo is only transmitted, never recieved");
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

static void IRAM_ATTR wifi_egress_thread([[maybe_unused]] void *arg) {
    for (;;) {
        WifiSendBuff *buff;
        if (xQueueReceive(wifi_egress_queue, &buff, (TickType_t)portMAX_DELAY)) {
            if (!buff) {
                ESP_LOGI(TAG, "NULL pulled from egress queue");
                continue;
            }

            int8_t err = esp_wifi_internal_tx(WIFI_IF_STA, buff->data, buff->len);
            if (err != ESP_OK) {
                ESP_LOGI(TAG, "Failed to send packet !!!");
            }
            free_wifi_send_buff(buff);
        }
    }
}

static void IRAM_ATTR output_rx_thread([[maybe_unused]] void *arg) {
    ESP_LOGI(TAG, "Started RX thread");
    for (;;) {
        read_message();
    }
}

static void IRAM_ATTR uart_tx_thread([[maybe_unused]] void *arg) {
    // Send initial device info to let master know ESP is ready
    send_device_info();

    for (;;) {
        WifiReceiveBuff *buff;
        if (xQueueReceive(uart_tx_queue, &buff, (TickType_t)1000 /*portMAX_DELAY*/)) {
            if (!buff) {
                ESP_LOGI(TAG, "Skipping packet with null buffer");
                continue;
            }
            // ESP_LOGI(TAG, "Printing packet to UART");
            Header header;
            header.type = static_cast<uint8_t>(MsgType::packet);
            header.link_status = get_link_status();
            header.size = htons(buff->len);
            xSemaphoreTake(uart_mtx, portMAX_DELAY);
            uart_write_bytes(UART_NUM_0, intron, sizeof(intron));
            uart_write_bytes(UART_NUM_0, (const char *)&header, sizeof(header));
            uart_write_bytes(UART_NUM_0, (const char *)buff->data, buff->len);
            xSemaphoreGive(uart_mtx);
            // ESP_LOGI(TAG, "Packet UART out done");
            free_wifi_receive_buff(buff);
        }
    }
}

extern "C" void app_main() {
    ESP_LOGI(TAG, "UART NIC");

    // esp_log_level_set("*", ESP_LOG_ERROR);

    ESP_ERROR_CHECK(nvs_flash_init());

    uart_set_pin(UART_NUM_0, 1, 3, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    // Configure parameters of an UART driver,
    // communication pins and install the driver
    uart_config_t uart_config = {
        .baud_rate = 4600000,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
        .source_clk = UART_SCLK_DEFAULT
    };
    uart_driver_install(UART_NUM_0, 16384, 0, 0, nullptr, 0);
    uart_param_config(UART_NUM_0, &uart_config);
    uart_intr_config_t uart_intr = {
        .intr_enable_mask = UART_RXFIFO_FULL_INT_ENA_M
            | UART_RXFIFO_TOUT_INT_ENA_M
            | UART_FRM_ERR_INT_ENA_M
            | UART_RXFIFO_OVF_INT_ENA_M,
        .rx_timeout_thresh = 1,
        .txfifo_empty_intr_thresh = 40,
        .rxfifo_full_thresh = 80
    };
    uart_intr_config(UART_NUM_0, &uart_intr);

    ESP_LOGI(TAG, "UART RE-INITIALIZED");

    uart_mtx = xSemaphoreCreateMutex();
    if (!uart_mtx) {
        ESP_LOGI(TAG, "Could not create UART mutex");
        return;
    }

    uart_tx_queue = xQueueCreate(20, sizeof(WifiReceiveBuff *));
    if (uart_tx_queue == 0) {
        ESP_LOGI(TAG, "Failed to create INPUT/TX queue");
        return;
    }

    wifi_egress_queue = xQueueCreate(20, sizeof(WifiSendBuff *));
    if (wifi_egress_queue == 0) {
        ESP_LOGI(TAG, "Failed to create WiFi TX queue");
        return;
    }

    scan.stored_ssids_count = SCAN_MAX_STORED_SSIDS;
    clear_stored_ssids();
    memset(latest_ssid, 0, SSID_MAX_LEN);
    memset(latest_bssid, 0, BSSID_MAX_LEN);

    ESP_LOGI(TAG, "Wifi init");
    esp_wifi_restore();
    wifi_init_sta();
    esp_wifi_set_ps(WIFI_PS_NONE);

    ESP_LOGI(TAG, "Creating RX thread");
    xTaskCreate(&output_rx_thread, "output_rx_thread", 4096, nullptr, 1, nullptr);
    ESP_LOGI(TAG, "Creating WiFi-out thread");
    xTaskCreate(&wifi_egress_thread, "wifi_egress_thread", 2048, nullptr, 12, nullptr);
    ESP_LOGI(TAG, "Creating TX thread");
    xTaskCreate(&uart_tx_thread, "uart_tx_thread", 2048, nullptr, 14, nullptr);
}
