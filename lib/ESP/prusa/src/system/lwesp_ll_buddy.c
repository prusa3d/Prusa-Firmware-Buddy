#include "esp/esp.h"
#include "esp/esp_mem.h"
#include "esp/esp_input.h"
#include "system/esp_ll.h"
#include "lwesp_ll_buddy.h"
#include "main.h"
#include "stm32_port.h"
#include "dbg.h"
#include "ff.h"

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

//Thread stuffs
static osThreadId UartBufferThread_id;
//message queue stuffs
osMessageQDef(uartBufferMbox, 16, NULL);
static osMessageQId uartBufferMbox_id;

static uint32_t esp_working_mode;
static uint32_t initialized;

uint8_t dma_buffer_rx[RX_BUFFER_LEN];

void esp_set_operating_mode(uint32_t mode) {
    esp_working_mode = mode;
}

uint32_t esp_get_operating_mode(void) {
    return esp_working_mode;
}

void esp_receive_data(UART_HandleTypeDef *huart) {
    ESP_UNUSED(huart);
    if (uartBufferMbox_id != NULL) {
        uint32_t message = 0;
        osMessagePut(uartBufferMbox_id, message, 0);
    }
}

/**
 * \brief           USART data processing
 */
void StartUartBufferThread(void const *arg) {
    size_t old_pos = 0;
    size_t pos = 0;

    ESP_UNUSED(arg);

    while (1) {
        /* Wait for the event message from DMA or USART */
        osMessageGet(uartBufferMbox_id, osWaitForever);

        /* Read data */
        uint32_t dma_bytes_left = __HAL_DMA_GET_COUNTER(huart6.hdmarx); // no. of bytes left for buffer full
        pos = sizeof(dma_buffer_rx) - dma_bytes_left;
        if (pos != old_pos && esp_get_operating_mode() == ESP_RUNNING_MODE) {
            if (pos > old_pos) {
                esp_input_process(&dma_buffer_rx[old_pos], pos - old_pos);
            } else {
                esp_input_process(&dma_buffer_rx[old_pos], sizeof(dma_buffer_rx) - old_pos);
                if (pos > 0) {
                    esp_input_process(&dma_buffer_rx[0], pos);
                }
            }
            old_pos = pos;
            if (old_pos == sizeof(dma_buffer_rx)) {
                old_pos = 0;
            }
        }
        HAL_UART_Receive_DMA(&huart6, (uint8_t *)dma_buffer_rx, RX_BUFFER_LEN);
    }
}

void esp_hard_reset_device() {
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
    esp_delay(10);
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
        __HAL_UART_ENABLE_IT(huart, UART_IT_IDLE);
    }
}

/**
 * \brief           Send data to ESP device
 * \param[in]       data: Pointer to data to send
 * \param[in]       len: Number of bytes to send
 * \return          Number of bytes sent
 */
size_t
esp_transmit_data(const void *data, size_t len) {
    if (esp_get_operating_mode() != ESP_RUNNING_MODE) {
        return 0;
    }

    for (size_t i = 0; i < len; ++i) {
        HAL_UART_Transmit(&huart6, (uint8_t *)(data + i), 1, 10);
    }
    return len;
}

espr_t esp_reconfigure_uart(const uint32_t baudrate) {
    huart6.Init.BaudRate = baudrate;
    int hal_uart_res = HAL_UART_Init(&huart6);
    if (hal_uart_res != HAL_OK) {
        _dbg("ESP LL: HAL_UART_Init failed with: %d", hal_uart_res);
        return espERR;
    }

    int hal_dma_res = HAL_UART_Receive_DMA(&huart6, (uint8_t *)dma_buffer_rx, RX_BUFFER_LEN);
    if (hal_dma_res != HAL_OK) {
        _dbg("ESP LL: HAL_UART_Receive_DMA failed with: %d", hal_dma_res);
        return espERR;
    }

    return espOK;
}

static void baudrate_change_evt(espr_t res, void *arg) {
    if (res != espOK) {
        _dbg("ESP baudrate change failed !!!");
        return;
    }
    uint32_t baudrate = (uint32_t)arg;
    _dbg("ESP baudrate change success, reconfiguring UART for %d", baudrate);
    esp_reconfigure_uart(baudrate);
}

espr_t esp_set_baudrate(const uint32_t baudrate) {
    return esp_set_at_baudrate(baudrate, baudrate_change_evt, (void *)baudrate, 1);
}

/**
 * \brief           Callback function called from initialization process
 */
espr_t
esp_ll_init(esp_ll_t *ll) {
#if !ESP_CFG_MEM_CUSTOM
    static uint8_t memory[ESP_MEM_SIZE];
    esp_mem_region_t mem_regions[] = {
        { memory, sizeof(memory) }
    };

    if (!initialized) {
        esp_mem_assignmemory(mem_regions, ESP_ARRAYSIZE(mem_regions)); /* Assign memory for allocations */
    }
#endif /* !ESP_CFG_MEM_CUSTOM */
    if (!initialized) {
        ll->send_fn = esp_transmit_data; /* Set callback function to send data */

        /* Create mbox and start thread */
        if (uartBufferMbox_id == NULL) {
            uartBufferMbox_id = osMessageCreate(osMessageQ(uartBufferMbox), NULL);
            if (uartBufferMbox_id == NULL) {
                _dbg("ESP LL: failed to create UART buffer mbox");
                return espERR;
            }
        }
        if (UartBufferThread_id == NULL) {
            osThreadDef(UartBufferThread, StartUartBufferThread, osPriorityNormal, 0, 512);
            UartBufferThread_id = osThreadCreate(osThread(UartBufferThread), NULL);
            if (UartBufferThread_id == NULL) {
                _dbg("ESP LL: failed to start UART buffer thread");
                return espERR;
            }
        }
    }

    esp_reconfigure_uart(ll->uart.baudrate);
    esp_set_operating_mode(ESP_RUNNING_MODE);
    initialized = 1;
    return espOK;
}

/**
 * \brief           Callback function to de-init low-level communication part
 */
espr_t
esp_ll_deinit(esp_ll_t *ll) {
    if (uartBufferMbox_id != NULL) {
        osMessageQId tmp = uartBufferMbox_id;
        uartBufferMbox_id = NULL;
        osMessageDelete(tmp);
    }
    if (UartBufferThread_id != NULL) {
        osThreadId tmp = UartBufferThread_id;
        UartBufferThread_id = NULL;
        osThreadTerminate(tmp);
    }
    initialized = 0;
    ESP_UNUSED(ll);
    return espOK;
}

espr_t esp_flash_initialize() {
    espr_t err = esp_ll_deinit(NULL);
    if (err != espOK) {
        return err;
    }
    esp_set_operating_mode(ESP_FLASHING_MODE);
    esp_reconfigure_uart(115200);
    loader_stm32_config_t loader_config = {
        .huart = &huart6,
        .port_io0 = GPIOE,
        .pin_num_io0 = GPIO_PIN_6,
        .port_rst = GPIOC,
        .pin_num_rst = GPIO_PIN_13,
    };
    loader_port_stm32_init(&loader_config);
    return espOK;
}
