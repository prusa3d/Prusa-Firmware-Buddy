#include "hal/HAL_RS485.hpp"
#include "PuppyConfig.hpp"

#include "stm32g0xx_hal.h"
#include "cmsis_os.h"
#include "HAL_Common.hpp"
#include <cstring>
#include "buddy/priorities_config.h"

#define USART  USART1
#define DMA    DMA1
#define DMA_RX DMA1_Channel2
#define DMA_TX DMA1_Channel3

#define DMA_CHANNEL_INDEX_RX          ((((uint32_t)DMA_RX - (uint32_t)DMA1_Channel1) / ((uint32_t)DMA1_Channel2 - (uint32_t)DMA1_Channel1)) << 2U)
#define DMAMUX_CHANNEL_RX             ((DMAMUX_Channel_TypeDef *)(uint32_t)((uint32_t)DMAMUX1_Channel0 + ((DMA_CHANNEL_INDEX_RX >> 2U) * ((uint32_t)DMAMUX1_Channel1 - (uint32_t)DMAMUX1_Channel0))))
#define DMAMUX_CHANNEL_NUMBER_RX      ((((uint32_t)DMA_RX & 0xFFU) - 8U) / 20U)
#define DMAMUX_CHANNEL_STATUS_RX      DMAMUX1_ChannelStatus
#define DMAMUX_CHANNEL_STATUS_MASK_RX (1UL << (DMAMUX_CHANNEL_NUMBER_RX & 0x1FU))

#define DMA_CHANNEL_INDEX_TX          ((((uint32_t)DMA_TX - (uint32_t)DMA1_Channel1) / ((uint32_t)DMA1_Channel2 - (uint32_t)DMA1_Channel1)) << 2U)
#define DMAMUX_CHANNEL_TX             ((DMAMUX_Channel_TypeDef *)(uint32_t)((uint32_t)DMAMUX1_Channel0 + ((DMA_CHANNEL_INDEX_TX >> 2U) * ((uint32_t)DMAMUX1_Channel1 - (uint32_t)DMAMUX1_Channel0))))
#define DMAMUX_CHANNEL_NUMBER_TX      ((((uint32_t)DMA_TX & 0xFFU) - 8U) / 20U)
#define DMAMUX_CHANNEL_STATUS_TX      DMAMUX1_ChannelStatus
#define DMAMUX_CHANNEL_STATUS_MASK_TX (1UL << (DMAMUX_CHANNEL_NUMBER_TX & 0x1FU))

namespace hal::RS485Driver {

static uint8_t s_modbusAddress;
static UART_HandleTypeDef s_huart;

static OnReceiveCallback s_pOnReceive = nullptr;

static uint8_t s_RX_buffer_1[RS485_BUFFER_SIZE];
static uint8_t s_RX_buffer_2[RS485_BUFFER_SIZE];
static uint8_t s_TX_buffer[RS485_BUFFER_SIZE];

bool Init(uint8_t modbusAddress) {
    s_modbusAddress = modbusAddress;

    // GPIO pins config
    GPIO_InitTypeDef GPIO_InitStruct {};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF1_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = RS485_DE_SIGNAL_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = 0;
    HAL_GPIO_Init(RS485_DE_SIGNAL_GPIO_PORT, &GPIO_InitStruct);

    // Peripheral clock enable
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();
    RCC_PeriphCLKInitTypeDef PeriphClkInit {};

    // Initializes the peripherals clocks
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
    PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
        Error_Handler();
    }

    // initialize USART
    memset(&s_huart, 0, sizeof(s_huart));
    s_huart.Instance = USART1;
    s_huart.Init.BaudRate = RS485_BAUDRATE;
    s_huart.Init.WordLength = UART_WORDLENGTH_8B;
    s_huart.Init.StopBits = RS485_STOP_BITS;
    s_huart.Init.Parity = RS485_PARITY;
    s_huart.Init.Mode = UART_MODE_TX_RX;
    s_huart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    s_huart.Init.OverSampling = UART_OVERSAMPLING_16;
    s_huart.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    s_huart.Init.ClockPrescaler = UART_PRESCALER_DIV1;

    if (HAL_RS485Ex_Init(&s_huart, UART_DE_POLARITY_HIGH, RS485_ASSERTION_TIME, RS485_DEASSERTION_TIME) != HAL_OK) {
        return false;
    }

    if (HAL_UARTEx_DisableFifoMode(&s_huart) != HAL_OK) {
        return false;
    }

    // DMA RX init

    // Disable DMA
    DMA_RX->CCR &= ~DMA_CCR_EN;
    // clear DMA flags
    DMAMUX_CHANNEL_STATUS_RX->CFR = DMAMUX_CHANNEL_STATUS_MASK_RX; // clear DMAMUX sync IRQ flag
    DMA->IFCR |= DMA_FLAG_GI1 << (DMA_CHANNEL_INDEX_RX & 0x1CU); // clear all DMA IRQ flags
    // Set the DMA Channel configuration
    DMA_RX->CCR = DMA_PRIORITY_HIGH | DMA_MDATAALIGN_BYTE | DMA_PDATAALIGN_BYTE | DMA_MINC_ENABLE | DMA_PINC_DISABLE | DMA_NORMAL | DMA_PERIPH_TO_MEMORY;
    // Set peripheral request to DMAMUX channel
    DMAMUX_CHANNEL_RX->CCR = (DMA_REQUEST_USART1_RX & DMAMUX_CxCR_DMAREQ_ID);
    // Clear the DMAMUX synchro overrun flag
    DMAMUX_CHANNEL_STATUS_RX->CFR = DMAMUX_CHANNEL_STATUS_MASK_RX;

    // DMA TX init

    // Disable DMA
    DMA_TX->CCR &= ~DMA_CCR_EN;
    // clear DMA flags
    DMAMUX_CHANNEL_STATUS_TX->CFR = DMAMUX_CHANNEL_STATUS_MASK_TX; // clear DMAMUX sync IRQ flag
    DMA->IFCR |= DMA_FLAG_GI1 << (DMA_CHANNEL_INDEX_TX & 0x1CU); // clear all DMA IRQ flags
    // Set the DMA Channel configuration
    DMA_TX->CCR = DMA_PRIORITY_LOW | DMA_MDATAALIGN_BYTE | DMA_PDATAALIGN_BYTE | DMA_MINC_ENABLE | DMA_PINC_DISABLE | DMA_NORMAL | DMA_MEMORY_TO_PERIPH;
    // Set peripheral request to DMAMUX channel
    DMAMUX_CHANNEL_TX->CCR = (DMA_REQUEST_USART1_TX & DMAMUX_CxCR_DMAREQ_ID);
    // Clear the DMAMUX synchro overrun flag
    DMAMUX_CHANNEL_STATUS_TX->CFR = DMAMUX_CHANNEL_STATUS_MASK_TX;

    // DMA interrupt init
    HAL_NVIC_SetPriority(DMA1_Channel2_3_IRQn, IRQ_PRIORITY_DMA1_CHANNEL2_3, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);

    // USART1 interrupt init
    HAL_NVIC_SetPriority(USART1_IRQn, ISR_PRIORITY_RS485, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);

    return true;
}

void SetOnReceiveCallback(OnReceiveCallback pCallback) {
    s_pOnReceive = pCallback;
};

void StartReceiving() {
    HAL_NVIC_DisableIRQ(DMA1_Channel2_3_IRQn);
    HAL_NVIC_DisableIRQ(USART1_IRQn);

    //*** Configure DMA

    // Disable DMA
    DMA_RX->CCR &= ~DMA_CCR_EN;

    // clear DMA flags
    DMAMUX_CHANNEL_STATUS_RX->CFR = DMAMUX_CHANNEL_STATUS_MASK_RX; // clear DMAMUX sync IRQ flag
    DMA->IFCR |= DMA_FLAG_GI1 << (DMA_CHANNEL_INDEX_RX & 0x1CU); // clear all DMA IRQ flags

    // Configure the source, destination address and the data length
    DMA_RX->CPAR = (uint32_t)(&USART->RDR);
    DMA_RX->CMAR = (uint32_t)s_RX_buffer_1;
    DMA_RX->CNDTR = RS485_BUFFER_SIZE;

    // Enable DMA interrupts
    DMA_RX->CCR &= ~DMA_IT_HT; // disable half-transfer
    DMA_RX->CCR |= (DMA_IT_TC | DMA_IT_TE); // enable transfer-complete and transfer-error

    // Enable DMA
    DMA_RX->CCR |= DMA_CCR_EN;

    //*** Configure USART

    // Disable Transmitter and Receiver
    CLEAR_BIT(USART->CR1, USART_CR1_TE | USART_CR1_RE);

    // switch RS485 transceiver to RX mode
    HAL_GPIO_WritePin(RS485_DE_SIGNAL_GPIO_PORT, RS485_DE_SIGNAL_GPIO_PIN, GPIO_PIN_RESET);

    // Enable Receiver Timeout feature
    MODIFY_REG(USART->RTOR, USART_RTOR_RTO, RS485_RX_TIMEOUT_BITS);
    SET_BIT(USART->CR2, USART_CR2_RTOEN);
    SET_BIT(USART->CR1, USART_CR1_RTOIE);

    // Enable DMA transfer
    SET_BIT(USART->CR3, USART_CR3_DMAR);

    // Enable Receiver
    SET_BIT(USART->CR1, USART_CR1_RE);

    HAL_NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
}

bool Transmit(uint8_t *pData, uint32_t dataSize) {
    // check arguments
    if (pData == nullptr || dataSize == 0 || dataSize > RS485_BUFFER_SIZE) {
        return false;
    }

    // prepare TX buffer
    memcpy(s_TX_buffer, pData, dataSize);

    HAL_NVIC_DisableIRQ(DMA1_Channel2_3_IRQn);
    HAL_NVIC_DisableIRQ(USART1_IRQn);

    // Disable Receiving
    CLEAR_BIT(USART->CR3, USART_CR3_DMAR);
    CLEAR_BIT(USART->CR1, USART_CR1_RE);
    CLEAR_BIT(DMA_RX->CCR, DMA_CCR_EN);

    // switch RS485 transceiver to TX mode
    HAL_GPIO_WritePin(RS485_DE_SIGNAL_GPIO_PORT, RS485_DE_SIGNAL_GPIO_PIN, GPIO_PIN_SET);

    // Disable DMA
    DMA_TX->CCR &= ~DMA_CCR_EN;

    // clear DMA flags
    DMAMUX_CHANNEL_STATUS_TX->CFR = DMAMUX_CHANNEL_STATUS_MASK_TX; // clear DMAMUX sync IRQ flag
    DMA->IFCR |= DMA_FLAG_GI1 << (DMA_CHANNEL_INDEX_TX & 0x1CU); // clear all DMA IRQ flags

    // Configure the source, destination address and the data length
    DMA_TX->CPAR = (uint32_t)(&USART->TDR);
    DMA_TX->CMAR = (uint32_t)s_TX_buffer;
    DMA_TX->CNDTR = dataSize;

    // Enable DMA interrupts
    DMA_TX->CCR &= ~(DMA_IT_HT | DMA_IT_TC); // disable half-transfer and transfer-complete
    DMA_TX->CCR |= (DMA_IT_TE); // enable transfer-error

    // Enable DMA
    DMA_TX->CCR |= DMA_CCR_EN;

    // Enable the UART Transmit Complete Interrupt
    SET_BIT(USART->CR1, USART_CR1_TCIE);

    // Enable Transmitter
    SET_BIT(USART->CR1, USART_CR1_TE);
    SET_BIT(USART->CR3, USART_CR3_DMAT);

    HAL_NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);
    HAL_NVIC_EnableIRQ(USART1_IRQn);

    return true;
}

// private
void FinishTransmit() {
    // Disable Transmitter
    CLEAR_BIT(USART->CR3, USART_CR3_DMAT);
    CLEAR_BIT(USART->CR1, USART_CR1_TE);
    CLEAR_BIT(DMA_TX->CCR, DMA_CCR_EN);
    DMA->IFCR |= DMA_FLAG_GI1 << (DMA_CHANNEL_INDEX_TX & 0x1CU); // clear all TX DMA IRQ flags

    // Disable the UART Transmit Complete Interrupt
    SET_BIT(USART->ICR, USART_ICR_TCCF);
    CLEAR_BIT(USART->CR1, USART_CR1_TCIE);

    // switch RS485 transceiver to RX mode
    HAL_GPIO_WritePin(RS485_DE_SIGNAL_GPIO_PORT, RS485_DE_SIGNAL_GPIO_PIN, GPIO_PIN_RESET);

    // Enable Receiver
    SET_BIT(USART->CR1, USART_CR1_RE);

    DMA_RX->CNDTR = RS485_BUFFER_SIZE;
    SET_BIT(DMA_RX->CCR, DMA_CCR_EN);
    SET_BIT(USART->CR1, USART_CR1_RE);
    SET_BIT(USART->CR3, USART_CR3_DMAR);
}

void DMA_IRQHandler() {
    uint32_t flag_it = DMA->ISR;

    //*** DMA Channel 2 - RX IRQ

    // transfer-error or transfer-complete events
    // transfer-complete event means error, because message is longer than expected
    if ((flag_it & (DMA_FLAG_TE1 << (DMA_CHANNEL_INDEX_RX & 0x1CU))) != 0U
        || (flag_it & (DMA_FLAG_TC1 << (DMA_CHANNEL_INDEX_RX & 0x1CU))) != 0U) {

        // disable receiving
        CLEAR_BIT(USART->CR3, USART_CR3_DMAR);
        CLEAR_BIT(USART->CR1, USART_CR1_RE);
        CLEAR_BIT(DMA_RX->CCR, DMA_CCR_EN);

        // clear flags
        DMAMUX_CHANNEL_STATUS_RX->CFR = DMAMUX_CHANNEL_STATUS_MASK_RX;
        DMA->IFCR |= DMA_FLAG_GI1 << (DMA_CHANNEL_INDEX_RX & 0x1CU);

        // enable receiving
        DMA_RX->CNDTR = RS485_BUFFER_SIZE;
        SET_BIT(DMA_RX->CCR, DMA_CCR_EN);
        SET_BIT(USART->CR1, USART_CR1_RE);
        SET_BIT(USART->CR3, USART_CR3_DMAR);
    }

    //*** DMA Channel 3 - TX IRQ

    // TX DMA transfer error
    uint32_t expectedFlags = (DMA_FLAG_TE1 << (DMA_CHANNEL_INDEX_TX & 0x1CU));
    if ((flag_it & expectedFlags) != 0U) {
        FinishTransmit();
    }
}

void USART_IRQHandler() {
    uint32_t isrflags = READ_REG(USART->ISR);

    // RX frame end detected
    if ((isrflags & USART_ISR_RTOF) != 0U) {
        __HAL_UART_CLEAR_FLAG(&s_huart, UART_CLEAR_RTOF);

        // disable DMA
        CLEAR_BIT(USART->CR3, USART_CR3_DMAR);
        CLEAR_BIT(DMA_RX->CCR, DMA_CCR_EN);

        // set new DMA buffer
        uint8_t *buffer = (uint8_t *)DMA_RX->CMAR;
        uint32_t dataSize = RS485_BUFFER_SIZE - DMA_RX->CNDTR;
        DMA_RX->CMAR = (uint32_t)((buffer == s_RX_buffer_1) ? s_RX_buffer_2 : s_RX_buffer_1);
        DMA_RX->CNDTR = RS485_BUFFER_SIZE;

        // enable new DMA transfer
        SET_BIT(DMA_RX->CCR, DMA_CCR_EN);
        SET_BIT(USART->CR3, USART_CR3_DMAR);

        // process received data
        if (buffer[0] == s_modbusAddress && dataSize > 0 && s_pOnReceive != nullptr) {
            s_pOnReceive(buffer, dataSize);
        }
    }

    uint32_t cr1its = READ_REG(USART->CR1);

    // TX transmission complete
    if (((isrflags & USART_ISR_TC) != 0U) && ((cr1its & USART_CR1_TCIE) != 0U)) {
        FinishTransmit();
    }

    // process errors
    uint32_t errorflags = (isrflags & (uint32_t)(USART_ISR_PE | USART_ISR_FE | USART_ISR_ORE | USART_ISR_NE));
    if (errorflags != 0U) {
        // parity error
        if ((isrflags & USART_ISR_PE) != 0U) {
            __HAL_UART_CLEAR_FLAG(&s_huart, UART_CLEAR_PEF);
        }

        // framing error
        if ((isrflags & USART_ISR_FE) != 0U) {
            __HAL_UART_CLEAR_FLAG(&s_huart, UART_CLEAR_FEF);
        }

        // noise error
        if ((isrflags & USART_ISR_NE) != 0U) {
            __HAL_UART_CLEAR_FLAG(&s_huart, UART_CLEAR_NEF);
        }

        // overrun
        if ((isrflags & USART_ISR_ORE) != 0U) {
            __HAL_UART_CLEAR_FLAG(&s_huart, UART_CLEAR_OREF);
        }
    }
}

} // namespace hal::RS485Driver
