#include "lwesp/lwesp.h"
#include "lwesp/lwesp_mem.h"
#include "lwesp/lwesp_input.h"
#include "system/lwesp_ll.h"
#include "lwesp_ll_buddy.h"

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

//UART buffer stuffs
#define RX_BUFFER_LEN 0x500
#if !defined(LWESP_MEM_SIZE)
    #define LWESP_MEM_SIZE 0x500
#endif /* !defined(LWESP_MEM_SIZE) */

static uint32_t esp_working_mode;
static uint32_t initialized;

uint8_t dma_buffer_rx[RX_BUFFER_LEN];

void esp_set_operating_mode(uint32_t mode) {
    if (mode == ESP_RUNNING_MODE) {
        __HAL_UART_ENABLE_IT(&huart6, UART_IT_IDLE);
    } else if (mode == ESP_FLASHING_MODE) {
        __HAL_UART_DISABLE_IT(&huart6, UART_IT_IDLE);
    }
    esp_working_mode = mode;
}

uint32_t esp_get_operating_mode(void) {
    return esp_working_mode;
}

void esp_receive_data(UART_HandleTypeDef *huart) {
    LWESP_UNUSED(huart);
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

    LWESP_UNUSED(arg);

    while (1) {
        /* Wait for the event message from DMA or USART */
        osMessageGet(uartBufferMbox_id, osWaitForever);

        /* Read data */
        uint32_t dma_bytes_left = __HAL_DMA_GET_COUNTER(huart6.hdmarx); // no. of bytes left for buffer full
        pos = sizeof(dma_buffer_rx) - dma_bytes_left;
        if (pos != old_pos && esp_get_operating_mode() == ESP_RUNNING_MODE) {
            if (pos > old_pos) {
                lwesp_input_process(&dma_buffer_rx[old_pos], pos - old_pos);
            } else {
                lwesp_input_process(&dma_buffer_rx[old_pos], sizeof(dma_buffer_rx) - old_pos);
                if (pos > 0) {
                    lwesp_input_process(&dma_buffer_rx[0], pos);
                }
            }
            old_pos = pos;
            if (old_pos == sizeof(dma_buffer_rx)) {
                old_pos = 0;
            }
        }
    }
}

uint8_t reset_device(uint8_t state) {
    if (state) {
        // pin sate to reset
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
    } else {
        // pin sate to set
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
    }
    return 1;
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

void configure_uart(uint32_t baudrate) {
    if (HAL_UART_Receive_DMA(&huart6, (uint8_t *)dma_buffer_rx, RX_BUFFER_LEN) != HAL_OK) {
        Error_Handler();
    }

    /* Create mbox and start thread */
    if (uartBufferMbox_id == NULL) {
        uartBufferMbox_id = osMessageCreate(osMessageQ(uartBufferMbox), NULL);
        if (uartBufferMbox_id == NULL) {
            printf("error!");
        }
    }
    if (UartBufferThread_id == NULL) {
        osThreadDef(UartBufferThread, StartUartBufferThread, osPriorityNormal, 0, 100);
        UartBufferThread_id = osThreadCreate(osThread(UartBufferThread), NULL);
        if (UartBufferThread_id == NULL) {
            printf("error!");
        }
    }
}

/**
 * \brief           Callback function called from initialization process
 */
lwespr_t
lwesp_ll_init(lwesp_ll_t *ll) {
#if !LWESP_CFG_MEM_CUSTOM
    static uint8_t memory[LWESP_MEM_SIZE];
    lwesp_mem_region_t mem_regions[] = {
        { memory, sizeof(memory) }
    };

    if (!initialized) {
        lwesp_mem_assignmemory(mem_regions, LWESP_ARRAYSIZE(mem_regions)); /* Assign memory for allocations */
    }
#endif /* !LWESP_CFG_MEM_CUSTOM */
    if (!initialized) {
        ll->send_fn = esp_transmit_data; /* Set callback function to send data */
        ll->reset_fn = reset_device;     /* Set callback for hardware reset */
    }

    configure_uart(ll->uart.baudrate); /* Initialize UART for communication */
    esp_set_operating_mode(ESP_RUNNING_MODE);
    initialized = 1;
    return lwespOK;
}

/**
 * \brief           Callback function to de-init low-level communication part
 */
lwespr_t
lwesp_ll_deinit(lwesp_ll_t *ll) {
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
    LWESP_UNUSED(ll);
    return lwespOK;
}
