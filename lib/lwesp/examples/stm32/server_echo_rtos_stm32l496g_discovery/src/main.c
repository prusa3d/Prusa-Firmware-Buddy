/**
 * \file            main.c
 * \brief           Main file
 */

/*
 * Copyright (c) 2020 Tilen MAJERLE
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
 * Author:          Tilen MAJERLE <tilen@majerle.eu>
 * Version:         v1.0.0
 */
#include "main.h"
#include "cmsis_os.h"

#include "lwesp/lwesp.h"
#include "station_manager.h"
#include "netconn_client.h"

static void LL_Init(void);
void SystemClock_Config(void);
static void USART_Printf_Init(void);

static void init_thread(void* arg);

static lwespr_t lwesp_callback_func(lwesp_evt_t* evt);
static lwespr_t server_callback_func(lwesp_evt_t* evt);

/**
 * \brief           Program entry point
 */
int
main(void) {
    LL_Init();                                  /* Reset of all peripherals, initializes the Flash interface and the Systick. */
    SystemClock_Config();                       /* Configure the system clock */
    USART_Printf_Init();                        /* Init USART for printf */

    printf("Application running on STM32L496G-Discovery!\r\n");

    /* Initialize, create first thread and start kernel */
    osKernelInitialize();
    const osThreadAttr_t attr = {
            .stack_size = 512
    };
    osThreadNew(init_thread, NULL, &attr);
    osKernelStart();

    while (1) {}
}

/**
 * \brief           Initialization thread
 * \param[in]       arg: Thread argument
 */
static void
init_thread(void* arg) {
    /* Initialize ESP with default callback function */
    printf("Initializing LwESP\r\n");
    if (lwesp_init(lwesp_callback_func, 1) != lwespOK) {
        printf("Cannot initialize LwESP!\r\n");
    } else {
        printf("LwESP initialized!\r\n");
    }

    while (1) {
        /* Periodically check if device connected to network */
        if (!lwesp_sta_is_joined()) {
            /*
             * Connect to access point.
             *
             * Try unlimited time until access point accepts up.
             * Check for station_manager.c to define preferred access points ESP should connect to
             */
            connect_to_preferred_access_point(1);
        }
        lwesp_delay(1000);
    }

	osThreadExit();
}

/**
 * \brief           Request data for connection
 */
static
uint8_t reply_data[20];

/**
 * \brief           Event callback function for connection-only
 * \param[in]       evt: Event information with data
 * \return          \ref lwespOK on success, member of \ref lwespr_t otherwise
 */
static lwespr_t
server_callback_func(lwesp_evt_t* evt) {
    lwesp_conn_p conn;
    lwespr_t res;

    conn = lwesp_conn_get_from_evt(evt);          /* Get connection handle from event */
    if (conn == NULL) {
        return lwespERR;
    }
    switch (lwesp_evt_get_type(evt)) {
        case LWESP_EVT_CONN_ACTIVE: {             /* Connection just active */
            printf("Connection active!\r\n");
            /* Do nothing, wait for remote side to send some data */
            break;
        }
        case LWESP_EVT_CONN_CLOSE: {              /* Connection closed */
            if (lwesp_evt_conn_close_is_forced(evt)) {
                printf("Connection closed by server!\r\n");
            } else {
                printf("Connection closed by remote side!\r\n");
            }
            break;
        }
        case LWESP_EVT_CONN_RECV: {               /* Data received from remote side */
            lwesp_pbuf_p pbuf = lwesp_evt_conn_recv_get_buff(evt);
            size_t length;
            lwesp_conn_recved(conn, pbuf);        /* Notify stack about received pbuf */

            length = lwesp_pbuf_length(pbuf, 1);  /* Get length of received packet */
            printf("Received %d bytes on connection\r\n", (int)length);

            length = lwesp_pbuf_copy(pbuf, reply_data, LWESP_MIN(length, sizeof(reply_data)), 0);   /* Copy data from pbuf to memory */
            res = lwesp_conn_send(conn, reply_data, length, NULL, 0); /* Start sending data in non-blocking mode */
            if (res == lwespOK) {
                printf("Sending response back to remote side..\r\n");
            } else {
                printf("Cannot send reply to remote side! Closing connection..\r\n");
            }
            break;
        }
        case LWESP_EVT_CONN_SEND: {               /* Data send event */
            lwespr_t res = lwesp_evt_conn_send_get_result(evt);
            if (res == lwespOK) {
                printf("Data sent successfully to client\r\n");
            } else {
                printf("Error while sending data!\r\n");
            }
            break;
        }
        default: break;
    }
    return lwespOK;
}

/**
 * \brief           Event callback function for ESP stack
 * \param[in]       evt: Event information with data
 * \return          \ref lwespOK on success, member of \ref lwespr_t otherwise
 */
static lwespr_t
lwesp_callback_func(lwesp_evt_t* evt) {
    lwespr_t res;
    switch (lwesp_evt_get_type(evt)) {
        case LWESP_EVT_AT_VERSION_NOT_SUPPORTED: {
            lwesp_sw_version_t v_min, v_curr;

            lwesp_get_min_at_fw_version(&v_min);
            lwesp_get_current_at_fw_version(&v_curr);

            printf("Current ESP8266 AT version is not supported by library!\r\n");
            printf("Minimum required AT version is: %d.%d.%d\r\n", (int)v_min.major, (int)v_min.minor, (int)v_min.patch);
            printf("Current AT version is: %d.%d.%d\r\n", (int)v_curr.major, (int)v_curr.minor, (int)v_curr.patch);
            break;
        }
        case LWESP_EVT_INIT_FINISH: {
            printf("Library initialized!\r\n");
            break;
        }
        case LWESP_EVT_RESET: {
            printf("Device reset sequence finished!\r\n");
            break;
        }
        case LWESP_EVT_RESET_DETECTED: {
            printf("Device reset detected!\r\n");
            break;
        }
        case LWESP_EVT_WIFI_CONNECTED: {
            /* Start server on port 80 and set callback for new connections */
            printf("Wifi connected\r\n");
            if ((res = lwesp_set_server(1, 80, LWESP_CFG_MAX_CONNS, 100, server_callback_func, NULL, NULL, 0)) == lwespOK) {
                printf("Starting server on port 80..\r\n");
            } else {
                printf("Cannot start server on port 80..\r\n");
            }
            break;
        }
        case LWESP_EVT_WIFI_DISCONNECTED: {
            /* Stop server on port 80, others parameters are don't care */
            printf("Wifi disconnected\r\n");
            if ((res = lwesp_set_server(0, 80, LWESP_CFG_MAX_CONNS, 100, server_callback_func, NULL, NULL, 0)) == lwespOK) {
                printf("Disabling server on port 80..\r\n");
            } else {
                printf("Cannot disable server on port 80..\r\n");
            }
            break;
        }
        default: break;
    }
    LWESP_UNUSED(res);
    return lwespOK;
}

/**
 * \brief           Low-Layer initialization
 */
static void
LL_Init(void) {
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

    NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
    NVIC_SetPriority(MemoryManagement_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0, 0));
    NVIC_SetPriority(BusFault_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0, 0));
    NVIC_SetPriority(UsageFault_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0, 0));
    NVIC_SetPriority(SVCall_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0, 0));
    NVIC_SetPriority(DebugMonitor_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0, 0));
    NVIC_SetPriority(PendSV_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0, 0));
    NVIC_SetPriority(SysTick_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 15, 0));
}

/**
 * \brief           System clock configuration
 */
void
SystemClock_Config(void) {
    /* Configure flash latency */
    LL_FLASH_SetLatency(LL_FLASH_LATENCY_4);
    if (LL_FLASH_GetLatency() != LL_FLASH_LATENCY_4) {
        while (1) {}
    }

    /* Set voltage scaling */
    LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);

    /* Enable MSI */
    LL_RCC_MSI_Enable();
    while (LL_RCC_MSI_IsReady() != 1) {}
    LL_RCC_MSI_EnableRangeSelection();
    LL_RCC_MSI_SetRange(LL_RCC_MSIRANGE_6);
    LL_RCC_MSI_SetCalibTrimming(0);

    /* Configure PLL */
    LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_MSI, LL_RCC_PLLM_DIV_1, 40, LL_RCC_PLLR_DIV_2);
    LL_RCC_PLL_EnableDomain_SYS();
    LL_RCC_PLL_Enable();
    while (LL_RCC_PLL_IsReady() != 1) {}

    /* Configure system clock to PLL */
    LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
    while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL) {}

    /* Set prescalers */
    LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
    LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
    LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);

    /* SysTick_IRQn interrupt configuration */
    LL_Init1msTick(80000000);
    LL_SYSTICK_SetClkSource(LL_SYSTICK_CLKSOURCE_HCLK);
    LL_SetSystemCoreClock(80000000);
    NVIC_SetPriority(SysTick_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 15, 0));
    LL_SYSTICK_EnableIT();
}

/**
 * \brief           Init USART2 for printf output
 */
static void
USART_Printf_Init(void) {
    LL_USART_InitTypeDef USART_InitStruct;
    LL_GPIO_InitTypeDef GPIO_InitStruct;

    /* Peripheral clock enable */
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART2);
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOC);

    /*
     * USART2 GPIO Configuration
     *
     * PA2  ------> USART2_TX
     * PD6  ------> USART2_RX
     */
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_7;

    GPIO_InitStruct.Pin = LL_GPIO_PIN_2;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LL_GPIO_PIN_6;
    LL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    USART_InitStruct.BaudRate = 921600;
    USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;
    USART_InitStruct.StopBits = LL_USART_STOPBITS_1;
    USART_InitStruct.Parity = LL_USART_PARITY_NONE;
    USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX_RX;
    USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
    USART_InitStruct.OverSampling = LL_USART_OVERSAMPLING_16;
    LL_USART_Init(USART2, &USART_InitStruct);

    LL_USART_ConfigAsyncMode(USART2);           /* Configure USART in async mode */
    LL_USART_Enable(USART2);                    /* Enable USART */
}

/**
 * \brief           Printf character handler
 * \param[in]       ch: Character to send
 * \param[in]       f: File pointer
 * \return          Written character
 */
#ifdef __GNUC__
int __io_putchar(int ch) {
#else
int fputc(int ch, FILE* fil) {
#endif
    LL_USART_TransmitData8(USART2, (uint8_t)ch);
    while (!LL_USART_IsActiveFlag_TXE(USART2)) {}
    return ch;
}
