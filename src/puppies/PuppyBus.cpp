#include "puppies/PuppyBus.hpp"

#include <device/board.h>
#include <device/peripherals.hpp>
#include <device/peripherals_uart.hpp>
#include "hwio_pindef.h"
#include "timing.h"
#include <freertos/binary_semaphore.hpp>
#include <bsod/bsod.h>

namespace buddy {
namespace puppies {

    using buddy::hw::Pin;
    using buddy::hw::RS485FlowControlPuppies;

    namespace {
        constexpr uint32_t baud_rate = 230'400;
        constexpr uint32_t bauds_per_character = 10; // 8N1

        /// Modbus 3.5-character silent interval. IDLE event covers 1 character,
        /// so the enforced value is 2.5 characters.
        constexpr float modbus_silence = 2.5f;
        constexpr uint32_t MINIMAL_PAUSE_BETWEEN_REQUESTS_US = static_cast<uint32_t>(1.0f + (modbus_silence * bauds_per_character * 1'000'000 / baud_rate));

        /// Longer pause for legacy bootloaders (MODULAR_BED, Dwarf).
        constexpr uint32_t BOOTSTRAP_PAUSE_BETWEEN_REQUESTS_US = static_cast<uint32_t>(1.0f + (8.0f * bauds_per_character * 1'000'000 / baud_rate));
    } // namespace

    osMutexDef(puppyMutex);
    static osMutexId puppyMutexId;
    uint32_t PuppyBus::last_operation_time_us = 0;
    freertos::BinarySemaphore delay_semaphore;

    void PuppyBus::Open() {
        debug_assert(puppyMutexId == nullptr);
        puppyMutexId = osMutexCreate(osMutex(puppyMutex));
        debug_assert(puppyMutexId != nullptr);
        uart_for_puppies.Open();
    }

    void PuppyBus::Close() {
        uart_for_puppies.Close();
        debug_assert(puppyMutexId != nullptr);
        osMutexDelete(puppyMutexId);
        puppyMutexId = nullptr;
    }

    void PuppyBus::HalfDuplexCallbackSwitch(bool transmit) {
        // WARNING: called within high priority ISR - never use RTT logging in this function
        //
        // Some revisions of xBuddy board have pull-down resistor on receive pin.
        // As soon as you set RS485 transceiver to transmit mode, it disconnects
        // the receive pin and it gets pulled down on those xBuddy revisions.
        // This means that the UART on STM32F4 starts seeing start-bit, 8 zero bits
        // and missing stop bit and reports this as a framing error, which puts
        // entire bus into invalid state.
        //
        // To workaround this issue, we disable receiver just before setting the pin
        // high and and enable it just after setting the pin low.
        //
        // Correct solution will probably be to get rid of the BufferedSerial
        // altogether, as we should always either be transmiting or receiving to idle,
        // never doing both at the same time.
        if (transmit) {
#if BOARD_IS_XBUDDY()
            uart_for_puppies.disable_receive();
#endif
            RS485FlowControlPuppies.write(Pin::State::high);
        } else {
            RS485FlowControlPuppies.write(Pin::State::low);
#if BOARD_IS_XBUDDY()
            uart_for_puppies.enable_receive();
#endif
        }
    }

    void PuppyBus::Flush() {
        uart_for_puppies.Flush();
    }

    void PuppyBus::ErrorRecovery() {
        uart_for_puppies.ErrorRecovery();
        Flush();
    }

    size_t PuppyBus::Read(uint8_t *buf, size_t len, uint32_t timeout) {
        uart_for_puppies.SetReadTimeoutMs(timeout);
        auto res = uart_for_puppies.Read(reinterpret_cast<char *>(buf), len, true);
        PuppyBus::last_operation_time_us = ticks_us();
        return res;
    }

    bool PuppyBus::Write(const uint8_t *buf, size_t len) {
        auto res = uart_for_puppies.Write(reinterpret_cast<const char *>(buf), len) == len;
        PuppyBus::last_operation_time_us = ticks_us();

        return res;
    }

    void PuppyBus::ensure_pause(uint32_t pause_us) {
        debug_assert(!xPortIsInsideInterrupt());
        const uint32_t elapsed_us = ticks_us() - last_operation_time_us;
        if (pause_us <= elapsed_us) {
            return;
        }
        const uint32_t remaining_us = pause_us - elapsed_us;
        debug_assert(remaining_us < 0xFFFF);

        // Delays shorter than 2 us would be more difficult to handle.
        // When ARR is set to N-1, it will take exactly N cycles to generate update event.
        const auto arr = std::max<uint32_t>(2, remaining_us) - 1;

        // Arm TIM4 in one-pulse mode: counter runs once from 0 to ARR (~us),
        // fires the update IRQ, and self-disables (CR1.CEN cleared by OPM).
        __HAL_TIM_SET_COUNTER(&htim4, 0);
        __HAL_TIM_SET_AUTORELOAD(&htim4, arr);
        __HAL_TIM_CLEAR_FLAG(&htim4, TIM_FLAG_UPDATE);
        __HAL_TIM_ENABLE(&htim4);

        delay_semaphore.acquire();
    }

    void PuppyBus::EnsurePause(Pause pause) {
        uint32_t pause_us;
        switch (pause) {
        case Pause::Short:
            pause_us = MINIMAL_PAUSE_BETWEEN_REQUESTS_US;
            break;
        case Pause::Long:
            pause_us = BOOTSTRAP_PAUSE_BETWEEN_REQUESTS_US;
            break;
        }
        ensure_pause(pause_us);
    }

    PuppyBus::LockGuard::LockGuard() {
        osStatus res = osMutexWait(puppyMutexId, osWaitForever);
        debug_assert(res == osOK);
        locked = true;
        UNUSED(res);
    }

    PuppyBus::LockGuard::LockGuard(bool &is_locked) {
        osStatus res = osMutexWait(puppyMutexId, osWaitForever);
        locked = (res == osOK);
        is_locked = locked;
    }

    PuppyBus::LockGuard::~LockGuard() {
        if (locked) {
            osStatus res = osMutexRelease(puppyMutexId);
            debug_assert(res == osOK);
            UNUSED(res);
        }
    }
} // namespace puppies
} // namespace buddy

extern "C" void TIM4_IRQHandler() {
    __HAL_TIM_CLEAR_FLAG(&htim4, TIM_FLAG_UPDATE);
    buddy::puppies::delay_semaphore.release_from_isr();
}
