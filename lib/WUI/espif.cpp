#include "espif.h"

#include <algorithm>
#include <cstring>
#include <cstdint>
#include <atomic>
#include <cassert>

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#include "main.h"

#include "stm32f4xx_hal_rng.h"

extern "C" {
#include "stm32_port.h"
}

#include "ff.h"
#include "wui_api.h"

#include "lwip/def.h"
#include "lwip/ethip6.h"
#include "lwip/etharp.h"

#include "log.h"

LOG_COMPONENT_DEF(ESPIF, LOG_SEVERITY_INFO);

// TODO: C++20:
// #include <bit>
// static_assert(std::endian::native == std::endian::little, "STM<->ESP protocol assumes all involved CPUs are little endian.");
static_assert(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__, "STM<->ESP protocol assumes all involved CPUs are little endian.");
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

enum ESPIFOperatingMode {
    ESPIF_UNINITIALIZED_MODE,
    ESPIF_RUNNING_MODE,
    ESPIF_FLASHING_MODE
};

enum MessageType {
    MSG_DEVINFO = 0,
    MSG_LINK = 1,
    MSG_CLIENTCONFIG = 3,
    MSG_PACKET = 4,
    MSG_INTRON = 5,
};

static const uint32_t ESP_FW_VERSION = 0;

// Thread stuff
static void uart_reader(void const *arg);
osThreadDef(UartBufferThread, uart_reader, osPriorityBelowNormal, 0, 512);

// NIC state
static std::atomic<uint16_t> fw_version;
static std::atomic<ESPIFOperatingMode> esp_operating_mode = ESPIF_UNINITIALIZED_MODE;
static bool associated = 0;
static std::atomic<TaskHandle_t> init_task_handle;

// UART
static const uint32_t NIC_UART_BAUDRATE = 1000000;
static const uint32_t FLASH_UART_BAUDRATE = 115200;
static const uint32_t CHARACTER_TIMEOUT_MS = 10;
uint8_t dma_buffer_rx[RX_BUFFER_LEN];
static size_t old_dma_pos = 0;
SemaphoreHandle_t uart_write_mutex = NULL;
osThreadId uart_reader_id = NULL;
static uint8_t intron[8] = { 'U', 'N', '\x00', '\x01', '\x02', '\x03', '\x04', '\x05' };

static void uart_input(uint8_t *data, size_t size, struct netif *netif);

void espif_receive_data(UART_HandleTypeDef *huart) {
    LWIP_UNUSED_ARG(huart);
    if (uart_reader_id != NULL) {
        osSignalSet(uart_reader_id, 0);
    }
}

static void hard_reset_device() {
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
    osDelay(10);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART6 && (huart->ErrorCode & HAL_UART_ERROR_NE || huart->ErrorCode & HAL_UART_ERROR_FE)) {
        __HAL_UART_DISABLE_IT(huart, UART_IT_IDLE);
        HAL_UART_DeInit(huart);
        if (HAL_UART_Init(huart) != HAL_OK) {
            Error_Handler();
        }
        if (HAL_UART_Receive_DMA(huart, (uint8_t *)dma_buffer_rx, RX_BUFFER_LEN) != HAL_OK) {
            Error_Handler();
        }
        old_dma_pos = 0;
        __HAL_UART_ENABLE_IT(huart, UART_IT_IDLE);
        log_error(ESPIF, "Recovered from UART error");
    }
}

/**
 * \brief           Send data to ESP device
 * \param[in]       data: Pointer to data to send
 * \param[in]       len: Number of bytes to send
 * \return          Operation result, ERR_OK if succeeded
 */
static err_t espif_transmit_data(const void *data, size_t len) {
    if (esp_operating_mode != ESPIF_RUNNING_MODE) {
        return ERR_USE;
    }

    int ret = HAL_UART_Transmit(&huart6, (uint8_t *)data, len, len * CHARACTER_TIMEOUT_MS);
    if (ret == HAL_OK) {
        return ERR_OK;
    } else {
        log_error(ESPIF, "UART TX fail: %d", ret);
        return ERR_TIMEOUT;
    }
}

static err_t espif_reconfigure_uart(const uint32_t baudrate) {
    log_info(ESPIF, "Reconfiguring UART for %d baud", baudrate);
    huart6.Init.BaudRate = baudrate;
    int hal_uart_res = HAL_UART_Init(&huart6);
    if (hal_uart_res != HAL_OK) {
        log_error(ESPIF, "ESP LL: HAL_UART_Init failed with: %d", hal_uart_res);
        return ERR_IF;
    }

    int hal_dma_res = HAL_UART_Receive_DMA(&huart6, (uint8_t *)dma_buffer_rx, RX_BUFFER_LEN);
    if (hal_dma_res != HAL_OK) {
        log_error(ESPIF, "ESP LL: HAL_UART_Receive_DMA failed with: %d", hal_dma_res);
        return ERR_IF;
    }

    return ERR_OK;
}

/**
 * @brief USART data processing thread body
 *
 * @param arg Pointer to netif used to process incoming packets
 *
 * TODO: This needs to be synchronized with UART DMA state. Current method somehow works, but there are isses to solve:
 *   - UART errors cause inconsitencies that result in reading whole buffer as input
 *   - Current blocking of the thread in case of no incoming UART data is suboptimal.
 */
static void uart_reader(void const *arg) {
    struct netif *netif = (struct netif *)arg;
    size_t pos = 0;
    log_info(ESPIF, "Reader thread running");

    while (true) {
        /* Wait for the event from DMA or USART */
        /* There is 1000ms max wait time to ensure some small standalone message
           does not get stuck in DMA buffer for too long. */
        osSignalWait(0, 1000);

        /* Read data */
        uint32_t dma_bytes_left = __HAL_DMA_GET_COUNTER(huart6.hdmarx); // no. of bytes left for buffer full
        pos = sizeof(dma_buffer_rx) - dma_bytes_left;
        if (pos != old_dma_pos && esp_operating_mode == ESPIF_RUNNING_MODE) {
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
}

static void process_mac(uint8_t *data, struct netif *netif) {
    log_info(ESPIF, "MAC: %02x:%02x:%02x:%02x:%02x:%02x", data[0], data[1], data[2], data[3], data[4], data[5]);
    xTaskNotify(init_task_handle, 0, eNoAction);
    netif->hwaddr_len = ETHARP_HWADDR_LEN;
    memcpy(netif->hwaddr, data, ETHARP_HWADDR_LEN);
    log_info(ESPIF, "MAC set");
}

bool espif_link(struct netif *netif) {
    return associated;
}

static void process_link_change(bool link_up, struct netif *netif) {
    log_info(ESPIF, "Link status changed: %d", link_up);
    associated = link_up;
    if (link_up) {
        netif_set_link_up(netif);
    } else {
        netif_set_link_down(netif);
    }
}

static void uart_input(uint8_t *data, size_t size, struct netif *netif) {
    log_debug(ESPIF, "Received ESP data len: %d", size);

    static enum ProtocolState {
        Intron,
        MessageType,
        Packet,
        PacketLen,
        PacketData,
        Link,
        DevInfo,
        FWVersion,
        MACData,
    } state
        = Intron;

    static uint intron_read = 0;
    static uint fw_version_read = 0;
    static uint32_t fw_version = 0;
    static uint mac_read = 0; // Amount of MAC bytes already read
    static uint8_t mac_data[ETHARP_HWADDR_LEN];

    static uint32_t rx_len = 0;  // Length of RX packet
    static uint rx_len_read = 0; // Amount of rx_len bytes already read

    static struct pbuf *rx_buff = NULL;     // First RX pbuf for current packet (chain head)
    static struct pbuf *rx_buff_cur = NULL; // Current pbuf for data receive (part of rx_buff chain)
    static uint32_t rx_read = 0;            // Amount of bytes already read into rx_buff_cur

    const uint8_t *end = &data[size];
    for (uint8_t *c = &data[0]; c < end;) {
        if (size < 200) {
            log_debug(ESPIF, "Processing data at %02x = %c", *c, *c);
        }

        switch (state) {
        case Intron:
            if (*c++ == intron[intron_read]) {
                intron_read++;
                log_debug(ESPIF, "Intron at %d", intron_read);
                if (intron_read >= sizeof(intron)) {
                    log_debug(ESPIF, "Intron detected");
                    state = MessageType;
                    intron_read = 0;
                }
            } else {
                intron_read = 0;
            }

            break;

        case MessageType:
            switch (*c) {
            case MSG_DEVINFO:
                log_debug(ESPIF, "Incomming devinfo message");
                state = DevInfo;
                break;
            case MSG_LINK:
                log_debug(ESPIF, "Incomming linkstatus message");
                state = Link;
                break;
            case MSG_PACKET:
                log_debug(ESPIF, "Incomming packet message");
                state = Packet;
                break;
            default:
                log_error(ESPIF, "Unknown message type %d", *c);
                state = Intron;
            }
            c++;
            break;

        case Link:
            process_link_change(*c++, netif);
            state = Intron;
            break;

        case DevInfo:
            state = FWVersion;
            fw_version_read = 0;
            break;

        case FWVersion:
            ((uint8_t *)&fw_version)[fw_version_read++] = *c++;
            if (fw_version_read == sizeof(fw_version)) {
                log_info(ESPIF, "ESP FW version: %d, supported version: %d", fw_version, ESP_FW_VERSION);
                mac_read = 0;
                state = MACData;
            }
            break;

        case MACData:
            while (c < end && mac_read < sizeof(mac_data)) {
                log_debug(ESPIF, "Read MAC byte at %d: %02x", mac_read, *c);
                mac_data[mac_read++] = *c++;
            }
            if (mac_read == sizeof(mac_data)) {
                process_mac(mac_data, netif);
                state = Intron;
            }
            break;

        case Packet:
            rx_len_read = 0;
            state = PacketLen;
            break;

        case PacketLen:
            if (rx_len_read < sizeof(rx_len)) {
                ((uint8_t *)&rx_len)[rx_len_read++] = *c++;
            } else {
                log_debug(ESPIF, "Reading packet size: %d", rx_len);
#if ETH_PAD_SIZE
                rx_len += ETH_PAD_SIZE; /* allow room for Ethernet padding */
#endif
                rx_buff = pbuf_alloc(PBUF_RAW, rx_len, PBUF_POOL);
                if (rx_buff) {
                    rx_buff_cur = rx_buff;
                    rx_read = 0;
                    state = PacketData;
                } else {
                    log_error(ESPIF, "Dropping packet due to out of RAM");
                    state = Intron;
                }
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
                log_debug(ESPIF, "Read packet size: %d", rx_len);
                if (netif->input(rx_buff, netif) != ERR_OK) {
                    log_error(ESPIF, "ethernetif_input: IP input error");
                    pbuf_free(rx_buff);
                    state = Intron;
                    break;
                }
                log_debug(ESPIF, "Input packet processed ok");
                state = Intron;
            }
        } break;
        }
    }
    log_debug(ESPIF, "Processed %d from UART", size);
}

/**
 * @brief Send intron
 *
 * Send intron sequence to ESP
 *
 * To improve security ESPIF generates random intron.
 */
static void generate_intron() {
    xSemaphoreTake(uart_write_mutex, portMAX_DELAY);

    // Send message header using old intro to ESP
    espif_transmit_data(intron, sizeof(intron));
    uint8_t msg_type = MSG_INTRON;

    // Generate new intron
    for (uint i = 2; i < sizeof(intron); i++) {
        intron[i] = HAL_RNG_GetRandomNumber(&hrng);
    }

    log_info(ESPIF, "New intron: %.*s", 8, intron);

    // Send new intron data
    espif_transmit_data(&msg_type, 1);
    espif_transmit_data(intron, sizeof(intron));

    xSemaphoreGive(uart_write_mutex);
}

/**
 * @brief Send packet using ESPIF NIC
 *
 * @param netif Output NETIF handle
 * @param p buffer (chain) to send
 */
static err_t low_level_output(struct netif *netif, struct pbuf *p) {
    if (esp_operating_mode != ESPIF_RUNNING_MODE) {
        log_error(ESPIF, "Cannot send packet, not in running mode.");
        return ERR_IF;
    }

    const uint32_t len = p->tot_len;
    log_debug(ESPIF, "Low level output packet size: %d", len);

    xSemaphoreTake(uart_write_mutex, portMAX_DELAY);
    espif_transmit_data(intron, sizeof(intron));
    uint8_t msg_type = MSG_PACKET;
    espif_transmit_data(&msg_type, 1);
    espif_transmit_data(&len, sizeof(len));
    while (p != NULL) {
        if (espif_transmit_data(p->payload, p->len) != ERR_OK) {
            log_error(ESPIF, "Low level output packet failed");
            xSemaphoreGive(uart_write_mutex);
            return ERR_IF;
        }
        p = p->next;
    }

    xSemaphoreGive(uart_write_mutex);
    return ERR_OK;
}

static err_t reset_and_wait() {
    // Capture this task handle to manage wakeup from input thread
    init_task_handle = xTaskGetCurrentTaskHandle();

    // Reset device to receive MAC address
    log_debug(ESPIF, "Resetting ESP to receive MAC address");
    hard_reset_device();

    // Wait for esp ready (receiveing device info)
    if (!xTaskNotifyWait(0, 0, NULL, pdMS_TO_TICKS(3000))) {
        log_error(ESPIF, "Timeout waiting for ESP bootup");
        return ERR_IF;
    }
    log_info(ESPIF, "ESP up and running");
    generate_intron();
    return ERR_OK;
}

/**
 * @brief Initalize ESPIF network interface
 *
 * This initializes NET interface. This is supposed to be called at most once.
 *
 * @param netif Interface to initialize
 * @return err_t Possible error encountered during initialization
 */
err_t espif_init(struct netif *netif) {
    log_info(ESPIF, "LwIP init");
    if (uart_write_mutex) {
        log_error(ESPIF, "Already initialized !!!");
        assert(0);
        return ERR_ALREADY;
    }

    espif_reconfigure_uart(NIC_UART_BAUDRATE);
    esp_operating_mode = ESPIF_RUNNING_MODE;

    // Create mutex to protect UART writes
    uart_write_mutex = xSemaphoreCreateMutex();
    if (!uart_write_mutex) {
        return ERR_IF;
    }

    // Create UART reader thread
    uart_reader_id = osThreadCreate(osThread(UartBufferThread), netif);
    if (uart_reader_id == NULL) {
        log_error(ESPIF, "Failed to start UART reader");
        return ERR_MEM;
    }

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

    return reset_and_wait();
}

err_t espif_flash_initialize() {
    // NOTE: There is no extra synchronization with reader thread. This assumes
    // it is not a problem if reader thread reads some garbage until it notices
    // operating mode change.
    // NOTE: This holds the writer mutex only during this call. Holding this one
    // all the time the ESP is being flashed might block LwIP thread and prevent
    // ethernet from being serviced. Still, all the writers must have finished -
    // this holds the lock and new writers will fail as mode is set to flashing.
    xSemaphoreTake(uart_write_mutex, portMAX_DELAY);
    esp_operating_mode = ESPIF_FLASHING_MODE;
    espif_reconfigure_uart(FLASH_UART_BAUDRATE);
    loader_stm32_config_t loader_config = {
        .huart = &huart6,
        .port_io0 = GPIOE,
        .pin_num_io0 = GPIO_PIN_6,
        .port_rst = GPIOC,
        .pin_num_rst = GPIO_PIN_13,
    };
    loader_port_stm32_init(&loader_config);
    xSemaphoreGive(uart_write_mutex);
    return ERR_OK;
}

err_t espif_flash_deinitialize() {
    espif_reconfigure_uart(NIC_UART_BAUDRATE);
    esp_operating_mode = ESPIF_RUNNING_MODE;
    return reset_and_wait();
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
    if (esp_operating_mode != ESPIF_RUNNING_MODE) {
        log_error(ESPIF, "Cannot joing AP, not in running mode.");
        return ERR_IF;
    }
    log_info(ESPIF, "Joining AP %s:%s", ssid, pass);

    xSemaphoreTake(uart_write_mutex, portMAX_DELAY);
    espif_transmit_data(intron, sizeof(intron));
    uint8_t msg_type = MSG_CLIENTCONFIG;
    espif_transmit_data(&msg_type, sizeof(msg_type));
    uint8_t ssid_len = strlen(ssid);
    uint8_t pass_len = strlen(pass);
    espif_transmit_data(&ssid_len, sizeof(ssid_len));
    espif_transmit_data(ssid, ssid_len);
    espif_transmit_data(&pass_len, sizeof(pass_len));
    espif_transmit_data(pass, pass_len);
    xSemaphoreGive(uart_write_mutex);

    return ERR_OK;
}
