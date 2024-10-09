#include "puppies/PuppyBus.hpp"

#include <device/board.h>
#include <device/peripherals.h>
#include <device/peripherals_uart.hpp>
#include "hwio_pindef.h"
#include "timing.h"

namespace buddy {
namespace puppies {

    using buddy::hw::Pin;
    using buddy::hw::RS485FlowControlPuppies;

    osMutexDef(puppyMutex);
    static osMutexId puppyMutexId;
    uint32_t PuppyBus::last_operation_time_us = 0;

    void PuppyBus::Open() {
        assert(puppyMutexId == nullptr);
        puppyMutexId = osMutexCreate(osMutex(puppyMutex));
        assert(puppyMutexId != nullptr);
        uart_for_puppies.Open();
    }

    void PuppyBus::Close() {
        uart_for_puppies.Close();
        assert(puppyMutexId != nullptr);
        osMutexDelete(puppyMutexId);
        puppyMutexId = nullptr;
    }

    void PuppyBus::HalfDuplexCallbackSwitch(bool transmit) {
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

    void PuppyBus::EnsurePause() {
        while (ticks_us() - PuppyBus::last_operation_time_us < PuppyBus::MINIMAL_PAUSE_BETWEEN_REQUESTS_US) {
            osDelay(1);
        }
    }

    PuppyBus::LockGuard::LockGuard() {
        osStatus res = osMutexWait(puppyMutexId, osWaitForever);
        assert(res == osOK);
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
            assert(res == osOK);
            UNUSED(res);
        }
    }
} // namespace puppies
} // namespace buddy
