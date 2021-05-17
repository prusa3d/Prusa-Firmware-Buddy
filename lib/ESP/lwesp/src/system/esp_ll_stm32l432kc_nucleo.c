/**
 * \file            esp_ll_stm32l432kc_nucleo.c
 * \brief           Low-level communication with ESP device for STM32L432KC-Nucleo using DMA
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
 * Author:          Miha Česnik
 */

/*
 * Default UART configuration is:
 *
 * UART:                USART1
 * STM32 TX (ESP RX):   GPIOA, GPIO_PIN_9
 * STM32 RX (ESP TX):   GPIOA, GPIO_PIN_10;
 * RESET:               GPIOA, GPIO_PIN_12
 * CH_PD:               GPIOB, GPIO_PIN_0
 * GPIO0:               GPIOA, GPIO_PIN_7
 * GPIO2:               GPIOA, GPIO_PIN_6
 *
 * USART_DMA:           DMA1
 * USART_DMA_CHANNEL:   DMA_CHANNEL_5
 * USART_DMA_REQ_NUM:   2
 */

#if !__DOXYGEN__

#include "stm32l4xx_ll_bus.h"
#include "stm32l4xx_ll_usart.h"
#include "stm32l4xx_ll_gpio.h"
#include "stm32l4xx_ll_dma.h"
#include "stm32l4xx_ll_rcc.h"
#include "stm32l4xx_ll_pwr.h"

/* USART */
#define ESP_USART                           USART1
#define ESP_USART_CLK                       LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART1)
#define ESP_USART_IRQ                       USART1_IRQn
#define ESP_USART_IRQHANDLER                USART1_IRQHandler

/* DMA settings */
#define ESP_USART_DMA                       DMA1
#define ESP_USART_DMA_CLK                   LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1)
#define ESP_USART_DMA_RX_CH                 LL_DMA_CHANNEL_5
#define ESP_USART_DMA_RX_REQ_NUM            LL_DMA_REQUEST_2
#define ESP_USART_DMA_RX_IRQ                DMA1_Channel5_IRQn
#define ESP_USART_DMA_RX_IRQHANDLER         DMA1_Channel5_IRQHandler

/* DMA flags management */
#define ESP_USART_DMA_RX_IS_TC              LL_DMA_IsActiveFlag_TC5(ESP_USART_DMA)
#define ESP_USART_DMA_RX_IS_HT              LL_DMA_IsActiveFlag_HT5(ESP_USART_DMA)
#define ESP_USART_DMA_RX_CLEAR_TC           LL_DMA_ClearFlag_TC5(ESP_USART_DMA)
#define ESP_USART_DMA_RX_CLEAR_HT           LL_DMA_ClearFlag_HT5(ESP_USART_DMA)

/* USART TX PIN */
#define ESP_USART_TX_PORT_CLK               LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA)
#define ESP_USART_TX_PORT                   GPIOA
#define ESP_USART_TX_PIN                    LL_GPIO_PIN_9
#define ESP_USART_TX_PIN_AF                 LL_GPIO_AF_7

/* USART RX PIN */
#define ESP_USART_RX_PORT_CLK               LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA)
#define ESP_USART_RX_PORT                   GPIOA
#define ESP_USART_RX_PIN                    LL_GPIO_PIN_10
#define ESP_USART_RX_PIN_AF                 LL_GPIO_AF_7

/* RESET PIN */
#define ESP_RESET_PORT_CLK                  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA)
#define ESP_RESET_PORT                      GPIOA
#define ESP_RESET_PIN                       LL_GPIO_PIN_12

/* GPIO0 PIN */
#define ESP_GPIO0_PORT_CLK                  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA)
#define ESP_GPIO0_PORT                      GPIOA
#define ESP_GPIO0_PIN                       LL_GPIO_PIN_7

/* GPIO2 PIN */
#define ESP_GPIO2_PORT_CLK                  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA)
#define ESP_GPIO2_PORT                      GPIOA
#define ESP_GPIO2_PIN                       LL_GPIO_PIN_6

/* CH_PD PIN */
#define ESP_CH_PD_PORT_CLK                  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB)
#define ESP_CH_PD_PORT                      GPIOB
#define ESP_CH_PD_PIN                       LL_GPIO_PIN_0

/* Include STM32 generic driver */
#include "../system/esp_ll_stm32.c"

#endif /* !__DOXYGEN__ */
