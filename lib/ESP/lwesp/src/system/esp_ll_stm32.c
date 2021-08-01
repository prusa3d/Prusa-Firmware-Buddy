/**
 * \file            esp_ll_stm32.c
 * \brief           Generic STM32 driver, included in various STM32 driver variants
 */

/*
 * Copyright (c) 2018 Tilen Majerle
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
 * AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * This file is part of ESP-AT library.
 *
 * Author:          Tilen MAJERLE <tilen@majerle.eu>
 */

/*
 * How it works
 *
 * On first call to \ref esp_ll_init, new thread is created and processed in usart_ll_thread function.
 * USART is configured in RX DMA mode and any incoming bytes are processed inside thread function.
 * DMA and USART implement interrupt handlers to notify main thread about new data ready to send to upper layer.
 *
 * More about UART + RX DMA: https://github.com/MaJerle/STM32_USART_DMA_RX
 *
 * \ref ESP_CFG_INPUT_USE_PROCESS must be enabled in `esp_config.h` to use this driver.
 */
#include "esp/esp.h"
#include "esp/esp_mem.h"
#include "esp/esp_input.h"
#include "system/esp_ll.h"

#if !__DOXYGEN__

#if !ESP_CFG_INPUT_USE_PROCESS
#error "ESP_CFG_INPUT_USE_PROCESS must be enabled in `esp_config.h` to use this driver."
#endif /* ESP_CFG_INPUT_USE_PROCESS */

#if !defined(ESP_USART_DMA_RX_BUFF_SIZE)
#define ESP_USART_DMA_RX_BUFF_SIZE      0x1000
#endif /* !defined(ESP_USART_DMA_RX_BUFF_SIZE) */

#if !defined(ESP_MEM_SIZE)
#define ESP_MEM_SIZE                    0x1000
#endif /* !defined(ESP_MEM_SIZE) */

#if !defined(ESP_USART_RDR_NAME)
#define ESP_USART_RDR_NAME              RDR
#endif /* !defined(ESP_USART_RDR_NAME) */

/* USART memory */
static uint8_t      usart_mem[ESP_USART_DMA_RX_BUFF_SIZE];
static uint8_t      is_running, initialized;
static size_t       old_pos;

/* USART thread */
static void usart_ll_thread(void const * arg);
static osThreadDef(usart_ll_thread, usart_ll_thread, osPriorityNormal, 0, 1024);
static osThreadId usart_ll_thread_id;

/* Message queue */
static osMessageQDef(usart_ll_mbox, 10, uint8_t);
static osMessageQId usart_ll_mbox_id;

/**
 * \brief           USART data processing
 */
static void
usart_ll_thread(void const * arg) {
    osEvent evt;
    size_t pos;
    static size_t old_pos;

    while (1) {
        /* Wait for the event message from DMA or USART */
        evt = osMessageGet(usart_ll_mbox_id, osWaitForever);
        if (evt.status != osEventMessage) {
            continue;
        }

        /* Read data */
#if defined(ESP_USART_DMA_RX_STREAM)
        pos = sizeof(usart_mem) - LL_DMA_GetDataLength(ESP_USART_DMA, ESP_USART_DMA_RX_STREAM);
#else
        pos = sizeof(usart_mem) - LL_DMA_GetDataLength(ESP_USART_DMA, ESP_USART_DMA_RX_CH);
#endif /* defined(ESP_USART_DMA_RX_STREAM) */
        if (pos != old_pos && is_running) {
            if (pos > old_pos) {
                esp_input_process(&usart_mem[old_pos], pos - old_pos);
            } else {
                esp_input_process(&usart_mem[old_pos], sizeof(usart_mem) - old_pos);
                esp_input_process(&usart_mem[0], pos);
            }
            old_pos = pos;
            if (old_pos == sizeof(usart_mem)) {
                old_pos = 0;
            }
        }
    }
}

/**
 * \brief           Configure UART using DMA for receive in double buffer mode and IDLE line detection
 */
static void
configure_uart(uint32_t baudrate) {
    static LL_USART_InitTypeDef usart_init;
    static LL_DMA_InitTypeDef dma_init;
    LL_GPIO_InitTypeDef gpio_init;

    if (!initialized) {
        /* Enable peripheral clocks */
        ESP_USART_CLK;
        ESP_USART_DMA_CLK;
        ESP_USART_TX_PORT_CLK;
        ESP_USART_RX_PORT_CLK;

#if defined(ESP_RESET_PIN)
        ESP_RESET_PORT_CLK;
#endif /* defined(ESP_RESET_PIN) */

#if defined(ESP_GPIO0_PIN)
        ESP_GPIO0_PORT_CLK;
#endif /* defined(ESP_GPIO0_PIN) */

#if defined(ESP_GPIO2_PIN)
        ESP_GPIO2_PORT_CLK;
#endif /* defined(ESP_GPIO2_PIN) */

#if defined(ESP_CH_PD_PIN)
        ESP_CH_PD_PORT_CLK;
#endif /* defined(ESP_CH_PD_PIN) */

        /* Global pin configuration */
        LL_GPIO_StructInit(&gpio_init);
        gpio_init.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
        gpio_init.Pull = LL_GPIO_PULL_UP;
        gpio_init.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
        gpio_init.Mode = LL_GPIO_MODE_OUTPUT;

#if defined(ESP_RESET_PIN)
        /* Configure RESET pin */
        gpio_init.Pin = ESP_RESET_PIN;
        LL_GPIO_Init(ESP_RESET_PORT, &gpio_init);
#endif /* defined(ESP_RESET_PIN) */

#if defined(ESP_GPIO0_PIN)
        /* Configure GPIO0 pin */
        gpio_init.Pin = ESP_GPIO0_PIN;
        LL_GPIO_Init(ESP_GPIO0_PORT, &gpio_init);
        LL_GPIO_SetOutputPin(ESP_GPIO0_PORT, ESP_GPIO0_PIN);
#endif /* defined(ESP_GPIO0_PIN) */

#if defined(ESP_GPIO2_PIN)
        /* Configure GPIO2 pin */
        gpio_init.Pin = ESP_GPIO2_PIN;
        LL_GPIO_Init(ESP_GPIO2_PORT, &gpio_init);
        LL_GPIO_SetOutputPin(ESP_GPIO2_PORT, ESP_GPIO2_PIN);
#endif /* defined(ESP_GPIO2_PIN) */

#if defined(ESP_CH_PD_PIN)
        /* Configure CH_PD pin */
        gpio_init.Pin = ESP_CH_PD_PIN;
        LL_GPIO_Init(ESP_CH_PD_PORT, &gpio_init);
        LL_GPIO_SetOutputPin(ESP_CH_PD_PORT, ESP_CH_PD_PIN);
#endif /* defined(ESP_CH_PD_PIN) */

        /* Configure USART pins */
        gpio_init.Mode = LL_GPIO_MODE_ALTERNATE;

        /* TX PIN */
        gpio_init.Alternate = ESP_USART_TX_PIN_AF;
        gpio_init.Pin = ESP_USART_TX_PIN;
        LL_GPIO_Init(ESP_USART_TX_PORT, &gpio_init);

        /* RX PIN */
        gpio_init.Alternate = ESP_USART_RX_PIN_AF;
        gpio_init.Pin = ESP_USART_RX_PIN;
        LL_GPIO_Init(ESP_USART_RX_PORT, &gpio_init);

        /* Configure UART */
        LL_USART_DeInit(ESP_USART);
        LL_USART_StructInit(&usart_init);
        usart_init.BaudRate = baudrate;
        usart_init.DataWidth = LL_USART_DATAWIDTH_8B;
        usart_init.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
        usart_init.OverSampling = LL_USART_OVERSAMPLING_16;
        usart_init.Parity = LL_USART_PARITY_NONE;
        usart_init.StopBits = LL_USART_STOPBITS_1;
        usart_init.TransferDirection = LL_USART_DIRECTION_TX_RX;
        LL_USART_Init(ESP_USART, &usart_init);

        /* Enable USART interrupts and DMA request */
        LL_USART_EnableIT_IDLE(ESP_USART);
        LL_USART_EnableIT_PE(ESP_USART);
        LL_USART_EnableIT_ERROR(ESP_USART);
        LL_USART_EnableDMAReq_RX(ESP_USART);

        /* Enable USART interrupts */
        NVIC_SetPriority(ESP_USART_IRQ, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0x07, 0x00));
        NVIC_EnableIRQ(ESP_USART_IRQ);

        /* Configure DMA */
        is_running = 0;
#if defined(ESP_USART_DMA_RX_STREAM)
        LL_DMA_DeInit(ESP_USART_DMA, ESP_USART_DMA_RX_STREAM);
        dma_init.Channel = ESP_USART_DMA_RX_CH;
#else
        LL_DMA_DeInit(ESP_USART_DMA, ESP_USART_DMA_RX_CH);
        dma_init.PeriphRequest = ESP_USART_DMA_RX_REQ_NUM;
#endif /* defined(ESP_USART_DMA_RX_STREAM) */
        dma_init.PeriphOrM2MSrcAddress = (uint32_t)&ESP_USART->ESP_USART_RDR_NAME;
        dma_init.MemoryOrM2MDstAddress = (uint32_t)usart_mem;
        dma_init.Direction = LL_DMA_DIRECTION_PERIPH_TO_MEMORY;
        dma_init.Mode = LL_DMA_MODE_CIRCULAR;
        dma_init.PeriphOrM2MSrcIncMode = LL_DMA_PERIPH_NOINCREMENT;
        dma_init.MemoryOrM2MDstIncMode = LL_DMA_MEMORY_INCREMENT;
        dma_init.PeriphOrM2MSrcDataSize = LL_DMA_PDATAALIGN_BYTE;
        dma_init.MemoryOrM2MDstDataSize = LL_DMA_MDATAALIGN_BYTE;
        dma_init.NbData = sizeof(usart_mem);
        dma_init.Priority = LL_DMA_PRIORITY_MEDIUM;
#if defined(ESP_USART_DMA_RX_STREAM)
        LL_DMA_Init(ESP_USART_DMA, ESP_USART_DMA_RX_STREAM, &dma_init);
#else
        LL_DMA_Init(ESP_USART_DMA, ESP_USART_DMA_RX_CH, &dma_init);
#endif /* defined(ESP_USART_DMA_RX_STREAM) */

        /* Enable DMA interrupts */
#if defined(ESP_USART_DMA_RX_STREAM)
        LL_DMA_EnableIT_HT(ESP_USART_DMA, ESP_USART_DMA_RX_STREAM);
        LL_DMA_EnableIT_TC(ESP_USART_DMA, ESP_USART_DMA_RX_STREAM);
        LL_DMA_EnableIT_TE(ESP_USART_DMA, ESP_USART_DMA_RX_STREAM);
        LL_DMA_EnableIT_FE(ESP_USART_DMA, ESP_USART_DMA_RX_STREAM);
        LL_DMA_EnableIT_DME(ESP_USART_DMA, ESP_USART_DMA_RX_STREAM);
#else
        LL_DMA_EnableIT_HT(ESP_USART_DMA, ESP_USART_DMA_RX_CH);
        LL_DMA_EnableIT_TC(ESP_USART_DMA, ESP_USART_DMA_RX_CH);
        LL_DMA_EnableIT_TE(ESP_USART_DMA, ESP_USART_DMA_RX_CH);
#endif /* defined(ESP_USART_DMA_RX_STREAM) */

        /* Enable DMA interrupts */
        NVIC_SetPriority(ESP_USART_DMA_RX_IRQ, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0x07, 0x00));
        NVIC_EnableIRQ(ESP_USART_DMA_RX_IRQ);

        old_pos = 0;
        is_running = 1;

        /* Start DMA and USART */
#if defined(ESP_USART_DMA_RX_STREAM)
        LL_DMA_EnableStream(ESP_USART_DMA, ESP_USART_DMA_RX_STREAM);
#else
        LL_DMA_EnableChannel(ESP_USART_DMA, ESP_USART_DMA_RX_CH);
#endif /* defined(ESP_USART_DMA_RX_STREAM) */
        LL_USART_Enable(ESP_USART);
    } else {
        osDelay(10);
        LL_USART_Disable(ESP_USART);
        usart_init.BaudRate = baudrate;
        LL_USART_Init(ESP_USART, &usart_init);
        LL_USART_Enable(ESP_USART);
    }

    /* Create mbox and start thread */
    if (usart_ll_mbox_id == NULL) {
        usart_ll_mbox_id = osMessageCreate(osMessageQ(usart_ll_mbox), NULL);
    }
    if (usart_ll_thread_id == NULL) {
        usart_ll_thread_id = osThreadCreate(osThread(usart_ll_thread), usart_ll_mbox_id);
    }

#if defined(ESP_RESET_PIN)
    /* Reset device on first init */
    if (!initialized) {
        LL_GPIO_ResetOutputPin(ESP_RESET_PORT, ESP_RESET_PIN);
        osDelay(1);
        LL_GPIO_SetOutputPin(ESP_RESET_PORT, ESP_RESET_PIN);
        osDelay(200);
    }
#endif /* ESP_RESET_PIN */
}

/**
 * \brief           Send data to ESP device
 * \param[in]       data: Pointer to data to send
 * \param[in]       len: Number of bytes to send
 * \return          Number of bytes sent
 */
static size_t
send_data(const void* data, size_t len) {
    const uint8_t* d = data;

    for (size_t i = 0; i < len; i++, d++) {
        LL_USART_TransmitData8(ESP_USART, *d);
        while (!LL_USART_IsActiveFlag_TXE(ESP_USART)) {}
    }
    return len;
}

/**
 * \brief           Callback function called from initialization process
 * \note            This function may be called multiple times if AT baudrate is changed from application
 * \param[in,out]   ll: Pointer to \ref esp_ll_t structure to fill data for communication functions
 * \param[in]       baudrate: Baudrate to use on AT port
 * \return          Member of \ref espr_t enumeration
 */
espr_t
esp_ll_init(esp_ll_t* ll) {
    static uint8_t memory[ESP_MEM_SIZE];
    esp_mem_region_t mem_regions[] = {
        { memory, sizeof(memory) }
    };

    if (!initialized) {
        ll->send_fn = send_data;                /* Set callback function to send data */

        esp_mem_assignmemory(mem_regions, ESP_ARRAYSIZE(mem_regions));  /* Assign memory for allocations */
    }

    configure_uart(ll->uart.baudrate);          /* Initialize UART for communication */
    initialized = 1;
    return espOK;
}

/**
 * \brief           Callback function to de-init low-level communication part
 * \param[in,out]   ll: Pointer to \ref esp_ll_t structure to fill data for communication functions
 * \return          \ref espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_ll_deinit(esp_ll_t* ll) {
    if (usart_ll_mbox_id != NULL) {
        osMessageQId tmp = usart_ll_mbox_id;
        usart_ll_mbox_id = NULL;
        osMessageDelete(tmp);
    }
    if (usart_ll_thread_id != NULL) {
        osThreadId tmp = usart_ll_thread_id;
        usart_ll_thread_id = NULL;
        osThreadTerminate(tmp);
    }
    initialized = 0;
    return espOK;
}

/**
 * \brief           UART global interrupt handler
 */
void
ESP_USART_IRQHANDLER(void) {
    if (LL_USART_IsActiveFlag_IDLE(ESP_USART)) {
        LL_USART_ClearFlag_IDLE(ESP_USART);
        if (usart_ll_mbox_id != NULL) {
            osMessagePut(usart_ll_mbox_id, 0, 0);
        }
    }
    LL_USART_ClearFlag_PE(ESP_USART);
    LL_USART_ClearFlag_FE(ESP_USART);
    LL_USART_ClearFlag_ORE(ESP_USART);
    LL_USART_ClearFlag_NE(ESP_USART);
}

/**
 * \brief           UART DMA stream/channel handler
 */
void
ESP_USART_DMA_RX_IRQHANDLER(void) {
    ESP_USART_DMA_RX_CLEAR_TC;
    ESP_USART_DMA_RX_CLEAR_HT;
    if (usart_ll_mbox_id != NULL) {
        osMessagePut(usart_ll_mbox_id, 0, 0);
    }
}

#endif /* !__DOXYGEN__ */
