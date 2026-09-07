#include "hal.hpp"

#include <freertos/binary_semaphore.hpp>
#include <stm32c0xx_hal.h>
#include <stm32c0xx_ll_usart.h>
#include <utils/byte_utils.hpp>

namespace hal::rs485 {
// Do not mindlessly reorder members, this leads to a better codegen!
struct State {
    // Note: Since rs485 is half duplex, we only need one set of data+size
    //       State is implicitly derived from which interrupts are enabled
    volatile std::byte *data = nullptr;
    volatile std::size_t size = 0;
    freertos::BinarySemaphore semaphore {};
    freertos::BinarySemaphore timer_semaphore {};
    alignas(uint16_t) std::byte buffer[256];
};
State state;

// Modbus specifies that between received message and next transmission
// there should be a silent interval of at least 3.5 character times.
static constexpr uint32_t bauds_per_character = 10;
static constexpr uint32_t baud_rate = 230'400;
static constexpr uint32_t timer_tick_rate = 48'000'000;
static constexpr uint32_t timer_ticks_per_baud = timer_tick_rate / baud_rate;
static constexpr uint32_t timer_ticks_per_character = timer_ticks_per_baud * bauds_per_character;
// Note that we use 2.5 instead of 3.5 here, because IDLE event is actually 1 character
static constexpr uint16_t timer_ticks_per_silent_interval = static_cast<uint16_t>(2.5 * timer_ticks_per_character);

void init() {
    __HAL_RCC_TIM14_CLK_ENABLE();

    // We are using TIM14 to ensure minimal pause between RX and TX.
    // TIM14 is setup to run in one-pulse mode, always resetting the counter
    // after line goes idle.
    // If we are in fact transmiting in a response to received frame, we enable
    // interrupt generation and wait for the semaphore.

    static_assert(timer_tick_rate == HSE_VALUE); // We don't need any prescaler
    WRITE_REG(TIM14->ARR, timer_ticks_per_silent_interval - 1);
    WRITE_REG(TIM14->CR1, TIM_CR1_OPM | TIM_CR1_URS);

    HAL_NVIC_SetPriority(TIM14_IRQn, ISR_PRIORITY_DEFAULT, 0);
    HAL_NVIC_EnableIRQ(TIM14_IRQn);

    LL_USART_DisableDirectionRx(USART2);
}

WritableBytes maybe_transmit_and_then_receive(WritableBytes tx_data) {
    // clear possible overrun error
    (void)LL_USART_ReceiveData8(USART2);
    LL_USART_ClearFlag_ORE(USART2);

    if (tx_data.empty()) {
        // setup receiving
        state.data = state.buffer;
        state.size = sizeof(state.buffer);
        LL_USART_EnableDirectionRx(USART2);
        LL_USART_EnableIT_RXNE(USART2);
        LL_USART_EnableIT_IDLE(USART2);
    } else {
        // ensure silent interval
        WRITE_REG(TIM14->DIER, TIM_DIER_UIE);
        state.timer_semaphore.acquire();

        // setup transmitting
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET); // DE/RE high: Tx mode
        state.data = tx_data.data();
        state.size = tx_data.size();
        LL_USART_EnableIT_TXE(USART2);
    }
    // and now we wait
    state.semaphore.acquire();
    return { state.buffer, sizeof(state.buffer) - state.size };
}

} // namespace hal::rs485

extern "C" void USART2_IRQHandler(void) {
    using namespace hal::rs485;
    if (LL_USART_IsEnabledIT_TXE(USART2) && LL_USART_IsActiveFlag_TXE(USART2) && state.size > 0) {
        LL_USART_TransmitData8(USART2, (uint8_t)*state.data);
        ++state.data;
        --state.size;
        if (state.size == 0) {
            LL_USART_DisableIT_TXE(USART2);

            // tx buffer drained, wait for transmission complete
            LL_USART_EnableIT_TC(USART2);
        }
    }
    if (LL_USART_IsEnabledIT_TC(USART2) && LL_USART_IsActiveFlag_TC(USART2)) {
        LL_USART_ClearFlag_TC(USART2);
        LL_USART_DisableIT_TC(USART2);

        // transmit complete, go receiving
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET); // DE/RE low: Rx mode
        state.data = state.buffer;
        state.size = sizeof(state.buffer);
        LL_USART_EnableDirectionRx(USART2);
        LL_USART_EnableIT_RXNE(USART2);
        LL_USART_EnableIT_IDLE(USART2);
    }
    if (LL_USART_IsEnabledIT_RXNE(USART2) && LL_USART_IsActiveFlag_RXNE(USART2)) {
        if (state.size == 0) {
            // silently discard data and let modbus CRC handle this
            (void)LL_USART_ReceiveData8(USART2);
        } else {
            *state.data = (std::byte)LL_USART_ReceiveData8(USART2);
            ++state.data;
            --state.size;
        }
    }
    if (LL_USART_IsEnabledIT_RXNE(USART2) && LL_USART_IsActiveFlag_ORE(USART2)) {
        LL_USART_ClearFlag_ORE(USART2);
        // overrun error - we are too slow to process data, no need to do anything special here
        // CRC check will fail
    }
    if (LL_USART_IsEnabledIT_IDLE(USART2) && LL_USART_IsActiveFlag_IDLE(USART2)) {
        if (state.size < sizeof(state.buffer)) {

            // immediately after receiving last byte, start the timer
            WRITE_REG(TIM14->EGR, TIM_EGR_UG);
            WRITE_REG(TIM14->SR, 0);
            SET_BIT(TIM14->CR1, TIM_CR1_CEN);

            // something received, wake-up the task
            LL_USART_ClearFlag_IDLE(USART2);
            LL_USART_DisableIT_IDLE(USART2);
            LL_USART_DisableIT_RXNE(USART2);
            LL_USART_DisableDirectionRx(USART2);
            state.semaphore.release_from_isr();
        } else {
            // idle expired and nothing received, keep waiting for another one
            LL_USART_ClearFlag_IDLE(USART2);
        }
    }
}

extern "C" void TIM14_IRQHandler(void) {
    using namespace hal::rs485;
    if (READ_REG(TIM14->SR) & TIM_SR_UIF) {
        WRITE_REG(TIM14->SR, 0);
        WRITE_REG(TIM14->DIER, 0);
        state.timer_semaphore.release_from_isr();
    }
}
