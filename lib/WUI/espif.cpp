#include "espif.h"

#include <algorithm>
#include <bit>
#include <cstring>
#include <cstdint>
#include <atomic>
#include <cassert>
#include <cinttypes>
#include <timing.h>
#include <mutex>

#include <FreeRTOS.h>
#include <freertos_mutex.hpp>
#include <task.h>
#include <semphr.h>
#include <ccm_thread.hpp>
#include <bsod.h>

#include "main.h"
#include "../metric.h"
#include "pbuf_rx.h"
#include "wui.h"
#include <tasks.hpp>
#include <option/has_embedded_esp32.h>
#include <random.h>

extern "C" {
#include "stm32_port.h"
}

#include "ff.h"
#include "wui_api.h"

#include <lwip/def.h>
#include <lwip/ethip6.h>
#include <lwip/etharp.h>
#include <lwip/sys.h>

#include "log.h"
#include <Marlin/src/inc/MarlinConfigPre.h>

LOG_COMPONENT_DEF(ESPIF, LOG_SEVERITY_INFO);

static_assert(std::endian::native == std::endian::little, "STM<->ESP protocol assumes all involved CPUs are little endian.");
static_assert(ETHARP_HWADDR_LEN == 6);

/*
 * UART and other pin configuration for ESP01 module
 *
 * UART:                USART6
 * STM32 TX (ESP RX):   GPIOC, GPIO_PIN_6
 * STM32 RX (ESP TX):   GPIOC, GPIO_PIN_7
 * RESET:               GPIOC, GPIO_PIN_13
 * GPIO0:               GPIOE, GPIO_PIN_6
 * GPIO2:               not connected
 * CH_PD:               connected to board 3.3 V
 *
 * UART_DMA:           DMA2
 * UART_RX_STREAM      STREAM_1
 * UART_TX_STREAM      STREAM_6
 */

/*
 * ESP UART NIC
 *
 * This provides a LwIP NIC implementation on top of a simple UART protocol used to communicate MAC address, link
 * status and packets with ESP8266 attached on the other end of the UART. This requires custom FW running in the ESP
 * implementing the protocol.
 *
 * Known issues:
 * - This does not use netif state. All the state is kept in static varibles -> only on NIC is supported
 *   (Maybe it is worh encapsulating the state just for the sake of code clarity.)
 * - This runs at 1Mbaud even when ESP support 4.5Mbaud. There is some wierd coruption at higher speeds
 *   (ESP seems to miss part of the packet data)
 * - This does not offload checksum computation to ESP. Would be nice to enable parity and make the protocol more
 *   robust (i.e using some counter to match packet begin and end while ensuring no data is lost). Provided UART
 *   can be trusted not to alternate packet content the ESP would be able to compute packet checksums.
 *
 */

#define ESP_UART_HANDLE UART_HANDLE_FOR(esp)

enum ESPIFOperatingMode {
    ESPIF_UNINITIALIZED_MODE,
    ESPIF_WAIT_INIT,
    ESPIF_NEED_AP,
    ESPIF_RUNNING_MODE,
    ESPIF_FLASHING_MODE,
    ESPIF_WRONG_FW,
};

enum MessageType {
    MSG_DEVINFO_V2 = 0,
    MSG_CLIENTCONFIG_V2 = 6,
    MSG_PACKET_V2 = 7,
};

#if PRINTER_IS_PRUSA_XL
// ESP32 FW version
static const uint32_t SUPPORTED_FW_VERSION = 10;
#else
// ESP8266 FW version
static const uint32_t SUPPORTED_FW_VERSION = 10;
#endif

// NIC state
static std::atomic<uint8_t> fw_version;
static std::atomic<ESPIFOperatingMode> esp_operating_mode = ESPIF_UNINITIALIZED_MODE;
static std::atomic<bool> associated = false;
static std::atomic<netif *> active_esp_netif;
// 10 seconds (20 health-check loops spaced 500ms from each other)
static std::atomic<uint8_t> init_countdown = 20;
static std::atomic<bool> seen_intron = false;
static std::atomic<bool> seen_rx_packet = false;

// UART
static const uint32_t NIC_UART_BAUDRATE = 4600000;
static const uint32_t FLASH_UART_BAUDRATE = 115200;
static std::atomic<bool> esp_detected;
// Have we seen the ESP alive at least once?
// (so we never ever report it as not there or no firmware or whatever).
static std::atomic<bool> esp_was_ok = false;
uint8_t dma_buffer_rx[RX_BUFFER_LEN];
static size_t old_dma_pos = 0;
static FreeRTOS_Mutex uart_write_mutex;
static bool espif_initialized = false;
static bool uart_has_recovered_from_error = false;
// Note: We never transmit more than one message so we might as well allocate statically.
static struct __attribute__((packed)) {
    uint8_t intron[8];
    uint8_t type;
    uint8_t byte; // interpretation depends on particular type
    uint16_t size;
} tx_message = {
    .intron = { 'U', 'N', '\x00', '\x01', '\x02', '\x03', '\x04', '\x05' },
    .type = 0,
    .byte = 0,
    .size = 0,
};

static void uart_input(uint8_t *data, size_t size, struct netif *netif);

void espif_receive_data(UART_HandleTypeDef *huart) {
    LWIP_UNUSED_ARG(huart);
    notify_esp_data();
}

static void hard_reset_device() {
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
    osDelay(100);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
    esp_detected = false;
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == UART_INSTANCE_FOR(esp) && (huart->ErrorCode & HAL_UART_ERROR_NE || huart->ErrorCode & HAL_UART_ERROR_FE)) {
        __HAL_UART_DISABLE_IT(huart, UART_IT_IDLE);
        HAL_UART_DeInit(huart);
        if (HAL_UART_Init(huart) != HAL_OK) {
            Error_Handler();
        }
        assert("Data for DMA cannot be in CCMRAM" && can_be_used_by_dma(reinterpret_cast<uintptr_t>(dma_buffer_rx)));
        if (HAL_UART_Receive_DMA(huart, (uint8_t *)dma_buffer_rx, RX_BUFFER_LEN) != HAL_OK) {
            Error_Handler();
        }
        old_dma_pos = 0;
        __HAL_UART_ENABLE_IT(huart, UART_IT_IDLE);
        uart_has_recovered_from_error = true;
        esp_detected = true;
    }
}

static bool is_running(ESPIFOperatingMode mode) {
    switch (mode) {
    case ESPIF_FLASHING_MODE:
    case ESPIF_UNINITIALIZED_MODE:
    case ESPIF_WRONG_FW:
        return false;
    case ESPIF_WAIT_INIT:
    case ESPIF_NEED_AP:
    case ESPIF_RUNNING_MODE:
        return true;
    }

    assert(0);
    return false;
}

static TaskHandle_t espif_task = nullptr;
static SemaphoreHandle_t tx_semaphore = nullptr;
static HAL_StatusTypeDef tx_result;
static bool tx_waiting = false;
static pbuf *tx_pbuf = nullptr; // only valid when tx_waiting == true

void espif_tx_callback() {
    // This is an interrupt handler for UART transmit completion. We want
    // to keep it short, delegate to `espif_task`, waking it up if possible.
    // Note that we can't simply `assert(espif_task)` because transmit may
    // happen even before `espif_task` is created.
    if (espif_task != nullptr) {
        BaseType_t higher_priority_task_woken = pdFALSE;
        vTaskNotifyGiveFromISR(espif_task, &higher_priority_task_woken);
        portYIELD_FROM_ISR(higher_priority_task_woken);
    }
}

static void espif_task_step() {
    // block indefinitely until ISR wakes us...
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    // ...woken up, do something

    // Note that we can't simply `assert(tx_waiting)` because transmit may
    // be initiated by code outside of the espif module.
    if (tx_waiting) {
        if (tx_pbuf) {
            if constexpr (!option::has_embedded_esp32) {
                // Predictive flow control - delay for ESP to load big enough buffer into UART driver
                // This is hotfix for ESP8266 not supplying buffers fast enough
                // Possibly, this slows down upload a little bit, but it is still faster than handling corruption.
                osDelay(1);
            }

            uint8_t *data = (uint8_t *)tx_pbuf->payload;
            size_t size = tx_pbuf->len;
            assert("Data for DMA cannot be in CCMRAM" && can_be_used_by_dma(reinterpret_cast<uintptr_t>(data)));
            tx_pbuf = tx_pbuf->next;
            tx_result = HAL_UART_Transmit_DMA(&ESP_UART_HANDLE, data, size);
            if (tx_result != HAL_OK) {
                log_error(ESPIF, "HAL_UART_Transmit_DMA() failed: %d", tx_result);
                tx_waiting = false;
                xSemaphoreGive(tx_semaphore);
            }
        } else {
            tx_waiting = false;
            xSemaphoreGive(tx_semaphore);
        }
    }
}

static void espif_task_run(void const *) {
    for (;;) {
        espif_task_step();
    }
}

void espif_task_create() {
    assert(tx_semaphore == nullptr && espif_task == nullptr);

    tx_semaphore = xSemaphoreCreateBinary();
    if (tx_semaphore == nullptr) {
        bsod("espif_task_create (semaphore)");
    }

    osThreadCCMDef(esp_task, espif_task_run, TASK_PRIORITY_ESP, 0, 128);
    espif_task = osThreadCreate(osThread(esp_task), nullptr);
    if (espif_task == nullptr) {
        bsod("espif_task_create (task)");
    }
}

static void espif_tx_update_metrics(uint32_t len) {
    static metric_t metric_esp_out = METRIC("esp_out", METRIC_VALUE_CUSTOM, 1000, METRIC_HANDLER_ENABLE_ALL);
    static uint32_t bytes_sent = 0;
    bytes_sent += len;
    metric_record_custom(&metric_esp_out, " sent=%" PRIu32 "i", bytes_sent);
}

[[nodiscard]] static err_t espif_tx_raw(uint8_t message_type, uint8_t message_byte, pbuf *p) {
    std::lock_guard lock { uart_write_mutex };

    const uint16_t size = p ? p->tot_len : 0;
    espif_tx_update_metrics(sizeof(tx_message) + size);
    tx_message.type = message_type;
    tx_message.byte = message_byte;
    tx_message.size = htons(size);

    assert(!tx_waiting);
    taskENTER_CRITICAL();
    tx_waiting = true;
    tx_pbuf = p;
    assert("Data for DMA cannot be in CCMRAM" && can_be_used_by_dma(reinterpret_cast<uintptr_t>(&tx_message)));
    HAL_StatusTypeDef tx_result = HAL_UART_Transmit_DMA(&ESP_UART_HANDLE, (uint8_t *)&tx_message, sizeof(tx_message));
    if (tx_result == HAL_OK) {
        taskEXIT_CRITICAL();
        xSemaphoreTake(tx_semaphore, portMAX_DELAY);
    } else {
        tx_waiting = false;
        taskEXIT_CRITICAL();
        log_error(ESPIF, "HAL_UART_Transmit_DMA() failed: %d", tx_result);
    }
    return tx_result;
}

// Note: Use this if you are absolutely sure that `buffer` is large enough to accomodate `data`.
[[nodiscard]] static FORCE_INLINE uint8_t *buffer_append_unsafe(uint8_t *buffer, const uint8_t *data, size_t size) {
    memcpy(buffer, data, size);
    return buffer + size;
}

[[nodiscard]] static err_t espif_tx_msg_clientconfig_v2(const char *ssid, const char *pass) {
    // Generate new intron
    uint8_t new_intron[8];
    for (uint i = 0; i < 2; i++) {
        new_intron[i] = tx_message.intron[i];
    }
    for (uint i = 2; i < sizeof(tx_message.intron); i++) {
        new_intron[i] = rand_u();
    }

    const uint8_t ssid_len = strlen(ssid);
    const uint8_t pass_len = strlen(pass);
    const uint16_t length = sizeof(new_intron) + sizeof(ssid_len) + ssid_len + sizeof(pass_len) + pass_len;

    pbuf *p = pbuf_alloc(PBUF_RAW, length, PBUF_RAM);
    if (!p) {
        return ERR_MEM;
    }
    {
        assert(p->tot_len == length);
        uint8_t *buffer = (uint8_t *)p->payload;
        buffer = buffer_append_unsafe(buffer, new_intron, sizeof(new_intron));
        buffer = buffer_append_unsafe(buffer, &ssid_len, sizeof(ssid_len));
        buffer = buffer_append_unsafe(buffer, (uint8_t *)ssid, ssid_len);
        buffer = buffer_append_unsafe(buffer, &pass_len, sizeof(pass_len));
        buffer = buffer_append_unsafe(buffer, (uint8_t *)pass, pass_len);
        assert(buffer == (uint8_t *)p->payload + length);
    }

    err_t err = espif_tx_raw(MSG_CLIENTCONFIG_V2, 0, p);
    memcpy(tx_message.intron, new_intron, sizeof(tx_message.intron));
    pbuf_free(p);
    return err;
}

[[nodiscard]] static err_t espif_tx_msg_packet(pbuf *p) {
    constexpr uint8_t up = 1;
    return espif_tx_raw(MSG_PACKET_V2, up, p);
}

static err_t espif_reconfigure_uart(const uint32_t baudrate) {
    ESP_UART_HANDLE.Init.BaudRate = baudrate;
    int hal_uart_res = HAL_UART_Init(&ESP_UART_HANDLE);
    if (hal_uart_res != HAL_OK) {
        log_error(ESPIF, "HAL_UART_Init() failed: %d", hal_uart_res);
        return ERR_IF;
    }

    assert("Data for DMA cannot be in CCMRAM" && can_be_used_by_dma(reinterpret_cast<uintptr_t>(dma_buffer_rx)));
    int hal_dma_res = HAL_UART_Receive_DMA(&ESP_UART_HANDLE, (uint8_t *)dma_buffer_rx, RX_BUFFER_LEN);
    if (hal_dma_res != HAL_OK) {
        log_error(ESPIF, "HAL_UART_Receive_DMA() failed: %d", hal_dma_res);
        return ERR_IF;
    }

    return ERR_OK;
}

void espif_input_once(struct netif *netif) {
    /* Read data */
    size_t pos = 0;

    /* Read data */
    uint32_t dma_bytes_left = __HAL_DMA_GET_COUNTER(ESP_UART_HANDLE.hdmarx); // no. of bytes left for buffer full
    pos = sizeof(dma_buffer_rx) - dma_bytes_left;
    if (pos != old_dma_pos && is_running(esp_operating_mode)) {
        if (pos > old_dma_pos) {
            uart_input(&dma_buffer_rx[old_dma_pos], pos - old_dma_pos, netif);
        } else {
            uart_input(&dma_buffer_rx[old_dma_pos], sizeof(dma_buffer_rx) - old_dma_pos, netif);
            if (pos > 0) {
                uart_input(&dma_buffer_rx[0], pos, netif);
            }
        }
        old_dma_pos = pos;
        if (old_dma_pos == sizeof(dma_buffer_rx)) {
            old_dma_pos = 0;
        }
    }
}

static void process_mac(uint8_t *data, struct netif *netif) {
    log_info(ESPIF, "MAC: %02x:%02x:%02x:%02x:%02x:%02x", data[0], data[1], data[2], data[3], data[4], data[5]);
    netif->hwaddr_len = ETHARP_HWADDR_LEN;
    memcpy(netif->hwaddr, data, ETHARP_HWADDR_LEN);

    ESPIFOperatingMode old = ESPIF_WAIT_INIT;
    if (esp_operating_mode.compare_exchange_strong(old, ESPIF_NEED_AP)) {
        uint16_t version = fw_version.load();
        if (version != SUPPORTED_FW_VERSION) {
            log_warning(ESPIF, "Firmware version mismatch: %d != %d", version, SUPPORTED_FW_VERSION);
            esp_operating_mode = ESPIF_WRONG_FW;
            return;
        }
        esp_operating_mode = ESPIF_NEED_AP;
        esp_was_ok = true;
    }
}

bool espif_link() {
    return associated;
}

static void process_link_change(bool link_up, struct netif *netif) {
    assert(netif != nullptr);
    if (link_up) {
        if (!associated.exchange(true)) {
            netif_set_link_up(netif);
            log_info(ESPIF, "Link went up");
        }
    } else {
        if (associated.exchange(false)) {
            log_info(ESPIF, "Link went down");
            netif_set_link_down(netif);
        }
    }
}

static void uart_input(uint8_t *data, size_t size, struct netif *netif) {
    esp_detected = true;

    // record metrics
    static metric_t metric_esp_in = METRIC("esp_in", METRIC_VALUE_CUSTOM, 1000, METRIC_HANDLER_ENABLE_ALL);
    static uint32_t bytes_received = 0;
    bytes_received += size;
    metric_record_custom(&metric_esp_in, " recv=%" PRIu32 "i", bytes_received);

    static enum ProtocolState {
        Intron,
        HeaderByte0,
        HeaderByte1,
        HeaderByte2,
        HeaderByte3,
        PacketData,
        PacketDataThrowaway,
        MACData,
    } state
        = Intron;

    static uint intron_read = 0;

    static uint8_t message_type = MSG_CLIENTCONFIG_V2; // might as well initialize to something invalid

    static uint mac_read = 0; // Amount of MAC bytes already read
    static uint8_t mac_data[ETHARP_HWADDR_LEN];

    static uint16_t rx_len = 0; // Length of RX packet

    static struct pbuf *rx_buff = NULL; // First RX pbuf for current packet (chain head)
    static struct pbuf *rx_buff_cur = NULL; // Current pbuf for data receive (part of rx_buff chain)
    static uint32_t rx_read = 0; // Amount of bytes already read into rx_buff_cur

    const uint8_t *end = &data[size];
    for (uint8_t *c = &data[0]; c < end;) {
        switch (state) {
        case Intron:
            if (*c++ == tx_message.intron[intron_read]) {
                intron_read++;
                if (intron_read >= sizeof(tx_message.intron)) {
                    state = HeaderByte0;
                    intron_read = 0;
                    seen_intron = true;
                }
            } else {
                intron_read = 0;
            }

            break;

        case HeaderByte0:
            message_type = *c++;
            switch (message_type) {
            case MSG_DEVINFO_V2:
            case MSG_PACKET_V2:
                state = HeaderByte1;
                break;
            default:
                log_warning(ESPIF, "Unknown message type: %d", message_type);
                state = Intron;
            }
            break;

        case HeaderByte1:
            switch (message_type) {
            case MSG_DEVINFO_V2:
                fw_version.store(*c++);
                if (fw_version < 10) {
                    process_mac(mac_data, netif);
                    state = Intron;
                } else {
                    state = HeaderByte2;
                }
                break;
            case MSG_PACKET_V2:
                process_link_change(*c++, netif);
                state = HeaderByte2;
                break;
            default:
                assert(false && "internal inconsistency");
                state = Intron;
            }
            break;

        case MACData:
            while (c < end && mac_read < sizeof(mac_data)) {
                mac_data[mac_read++] = *c++;
            }
            if (mac_read == sizeof(mac_data)) {
                process_mac(mac_data, netif);
                mac_read = 0;
                state = Intron;
            }
            break;

        case HeaderByte2:
            rx_len = (*c++) << 8;
            state = HeaderByte3;
            break;

        case HeaderByte3:
            rx_len = rx_len | (*c++);
            switch (message_type) {
            case MSG_DEVINFO_V2:
                state = MACData;
                break;
            case MSG_PACKET_V2:
                if (rx_len == 0) {
                    state = Intron;
                    break;
                }
                rx_buff = pbuf_alloc_rx(rx_len);
                if (rx_buff) {
                    rx_buff_cur = rx_buff;
                    rx_read = 0;
                    state = PacketData;
                } else {
                    log_warning(ESPIF, "pbuf_alloc_rx() failed, dropping packet");
                    rx_read = 0;
                    state = PacketDataThrowaway;
                }
                break;
            default:
                assert(false && "internal inconsistency");
                state = Intron;
            }
            break;

        case PacketData: {
            // Copy input to current pbuf (until end of input or current pbuf)
            const uint32_t to_read = std::min(rx_buff_cur->len - rx_read, (uint32_t)(end - c));
            memcpy((uint8_t *)rx_buff_cur->payload + rx_read, c, to_read);
            c += to_read;
            rx_read += to_read;

            // Switch to next pbuf
            if (rx_read == rx_buff_cur->len) {
                rx_buff_cur = rx_buff_cur->next;
                rx_read = 0;
            }

            // Filled all pbufs in a packet (current set to next = NULL)
            if (!rx_buff_cur) {
                if (netif->input(rx_buff, netif) != ERR_OK) {
                    log_warning(ESPIF, "tcpip_input() failed, dropping packet");
                    pbuf_free(rx_buff);
                    state = Intron;
                    break;
                }
                seen_rx_packet = true;
                state = Intron;
            }
        } break;
        case PacketDataThrowaway:
            const uint32_t to_read = std::min(rx_len - rx_read, (uint32_t)(end - c));
            c += to_read;
            rx_read += to_read;
            if (rx_read == rx_len) {
                state = Intron;
            }
        }
    }
}

/**
 * @brief Send packet using ESPIF NIC
 *
 * @param netif Output NETIF handle
 * @param p buffer (chain) to send
 */
static err_t low_level_output([[maybe_unused]] struct netif *netif, struct pbuf *p) {
    if (!is_running(esp_operating_mode)) {
        log_error(ESPIF, "Cannot send packet, not in running mode.");
        return ERR_IF;
    }

    if (espif_tx_msg_packet(p) != ERR_OK) {
        log_error(ESPIF, "espif_tx_msg_packet() failed");
        return ERR_IF;
    }
    return ERR_OK;
}

static void force_down() {
    struct netif *iface = active_esp_netif; // Atomic load
    assert(iface != nullptr); // Already initialized
    process_link_change(false, iface);
}

static void reset_intron() {
    std::lock_guard lock { uart_write_mutex };
    for (uint i = 2; i < sizeof(tx_message.intron); i++) {
        tx_message.intron[i] = i - 2;
    }
}

void espif_init_hw() {
    if (espif_initialized) {
        bsod("espif_init_hw() called twice");
    }

    espif_reconfigure_uart(NIC_UART_BAUDRATE);
    esp_operating_mode = ESPIF_WAIT_INIT;
    espif_initialized = true;
};

/**
 * @brief Initalize ESPIF network interface
 *
 * This initializes NET interface. This is supposed to be called at most once.
 *
 * @param netif Interface to initialize
 * @return err_t Possible error encountered during initialization
 */
err_t espif_init(struct netif *netif) {
#if BOARD_VER_HIGHER_OR_EQUAL_TO(0, 5, 0)
    // This is temporary, remove once everyone has compatible hardware.
    // Requires new sandwich rev. 06 or rev. 05 with R83 removed.

    #if HAS_EMBEDDED_ESP32()
    TaskDeps::wait(TaskDeps::Tasks::espif);
    #endif
#endif

    struct netif *previous = active_esp_netif.exchange(netif);
    assert(previous == nullptr);
    (void)previous; // Avoid warnings in release

    // Initialize lwip netif
    netif->name[0] = 'w';
    netif->name[1] = 'l';
    netif->output = etharp_output;
#if LWIP_IPV6
    netif->output_ip6 = ethip6_output;
#endif
    netif->linkoutput = low_level_output;

    // LL init
    netif->hwaddr_len = 0;
    // TODO: This assumes LwIP can live with hwaddr not being set until ESP reports it
    netif->mtu = 1500;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;

    reset_intron();
    esp_operating_mode = ESPIF_WAIT_INIT;
    return ERR_OK;
}

void espif_flash_initialize(const bool take_down_interfaces) {
    // NOTE: There is no extra synchronization with reader thread. This assumes
    // it is not a problem if reader thread reads some garbage until it notices
    // operating mode change.
    // NOTE: This holds the writer mutex only during this call. Holding this one
    // all the time the ESP is being flashed might block LwIP thread and prevent
    // ethernet from being serviced. Still, all the writers must have finished -
    // this holds the lock and new writers will fail as mode is set to flashing.
    {
        std::lock_guard lock { uart_write_mutex };
        esp_operating_mode = ESPIF_FLASHING_MODE;
        espif_reconfigure_uart(FLASH_UART_BAUDRATE);
        loader_stm32_config_t loader_config = {
            .huart = &ESP_UART_HANDLE,
            .port_io0 = GPIOE,
#if (BOARD_IS_XBUDDY || BOARD_IS_XLBUDDY)
            .pin_num_io0 = GPIO_PIN_15,
#else
            .pin_num_io0 = GPIO_PIN_6,
#endif
            .port_rst = GPIOC,
            .pin_num_rst = GPIO_PIN_13,
        };
        loader_port_stm32_init(&loader_config);
    }
    if (take_down_interfaces) {
        force_down();
    }
}

void espif_flash_deinitialize() {
    espif_reconfigure_uart(NIC_UART_BAUDRATE);
    reset_intron();
    hard_reset_device(); // Reset device to receive MAC address
    esp_operating_mode = ESPIF_WAIT_INIT;
}

/**
 * @brief Ask ESP to join AP
 *
 * This, just sends join command. It is not a big problem if network interface is not configured.
 *
 * @param ssid SSID
 * @param pass Password
 * @return err_t
 */
err_t espif_join_ap(const char *ssid, const char *pass) {
    if (!is_running(esp_operating_mode)) {
        return ERR_IF;
    }
    log_info(ESPIF, "Joining AP %s:*(%d)", ssid, strlen(pass));
    esp_operating_mode = ESPIF_RUNNING_MODE;

    return espif_tx_msg_clientconfig_v2(ssid, pass);
}

bool espif_tick() {
    const auto current_init = init_countdown.load();
    if (current_init > 0) {
        // In theory, this load - condition - store sequence is racy.
        // Nevertheless, we have only one thread that writes in there and it's
        // atomic to allow reading things at the same time.
        init_countdown.store(current_init - 1);
    }

    if (uart_has_recovered_from_error) {
        log_warning(ESPIF, "Recovered from UART error");
        uart_has_recovered_from_error = false;
    }

    if (espif_link()) {
        const bool was_alive = seen_intron.exchange(false);
        if (!seen_rx_packet.exchange(false) && is_running(esp_operating_mode)) {
            // Poke the ESP somewhat to see if it's still alive and provoke it to
            // do some activity during next round.
            std::ignore = espif_tx_msg_packet(nullptr);
        }
        return was_alive;
    }

    return false;
}

bool espif_need_ap() {
    return esp_operating_mode == ESPIF_NEED_AP;
}

void espif_reset() {
    // Don't touch it in case we are flashing right now. If so, it'll get reset
    // when done.
    if (esp_operating_mode != ESPIF_FLASHING_MODE) {
        reset_intron();
        force_down();
        hard_reset_device(); // Reset device to receive MAC address
        esp_operating_mode = ESPIF_WAIT_INIT;
    }
}

EspFwState esp_fw_state() {
    ESPIFOperatingMode mode = esp_operating_mode.load();
    const bool detected = esp_detected.load();
    // Once we see the ESP work at least once, we never ever complain about
    // it not having firmware or similar. If we didn't do this, we could report
    // it to be missing just after it is reset for inactivity. It'll likely
    // just wake up in a moment.
    const bool seen_ok = esp_was_ok.load();
    switch (mode) {
    case ESPIF_UNINITIALIZED_MODE:
        if (seen_ok) {
            return EspFwState::Ok;
        }
        return EspFwState::Unknown;
    case ESPIF_WAIT_INIT:
        if (seen_ok) {
            return EspFwState::Ok;
        }
        if (detected) {
            if (init_countdown > 0) {
                return EspFwState::Unknown;
            } else {
                return EspFwState::NoFirmware;
            }
        } else {
            return EspFwState::NoEsp;
        }
    case ESPIF_NEED_AP:
    case ESPIF_RUNNING_MODE:
        return EspFwState::Ok;
    case ESPIF_FLASHING_MODE:
        return EspFwState::Flashing;
    case ESPIF_WRONG_FW:
        return EspFwState::WrongVersion;
    }
    assert(0);
    return EspFwState::NoEsp;
}

EspLinkState esp_link_state() {
    ESPIFOperatingMode mode = esp_operating_mode.load();
    switch (mode) {
    case ESPIF_WAIT_INIT:
    case ESPIF_WRONG_FW:
    case ESPIF_FLASHING_MODE:
    case ESPIF_UNINITIALIZED_MODE:
        return EspLinkState::Init;
    case ESPIF_NEED_AP:
        return EspLinkState::NoAp;
        return EspLinkState::Down;
    case ESPIF_RUNNING_MODE: {
        if (espif_link()) {
            if (seen_intron) {
                return EspLinkState::Up;
            } else {
                return EspLinkState::Silent;
            }
        } else {
            return EspLinkState::Down;
        }
    }
    }
    assert(0);
    return EspLinkState::Init;
}
