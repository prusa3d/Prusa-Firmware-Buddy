
#include "puppies/PuppyBus.hpp"
#include <device/peripherals.h>
#include <device/peripherals_uart.hpp>
#include "hwio_pindef.h"
#include "buffered_serial.hpp"
#include "timing.h"

namespace buddy {
namespace puppies {

    using buddy::hw::BufferedSerial;
    using buddy::hw::Pin;
    using buddy::hw::RS485FlowControlPuppies;

    // Rx data has to absorb any reponse. Standard Modbus response fits in 256 bytes
    static uint8_t uart_pupies_rx_data[256];
    BufferedSerial PuppyBus::bufferedSerial(&uart_handle_for_puppies, PuppyBus::HalfDuplexCallbackSwitch, uart_pupies_rx_data, sizeof(uart_pupies_rx_data), BufferedSerial::CommunicationMode::DMA);
    osMutexDef(puppyMutex);
    static osMutexId puppyMutexId;
    uint32_t PuppyBus::last_operation_time_us = 0;

    void PuppyBus::Open() {
        assert(puppyMutexId == nullptr);
        puppyMutexId = osMutexCreate(osMutex(puppyMutex));
        assert(puppyMutexId != nullptr);
        bufferedSerial.Open();
    }

    void PuppyBus::Close() {
        bufferedSerial.Close();
        assert(puppyMutexId != nullptr);
        osMutexDelete(puppyMutexId);
        puppyMutexId = nullptr;
    }

    void PuppyBus::HalfDuplexCallbackSwitch(bool transmit) {
        if (transmit) {
            RS485FlowControlPuppies.write(Pin::State::high);
        } else {
            RS485FlowControlPuppies.write(Pin::State::low);
        }
    }

    void PuppyBus::Flush() {
        bufferedSerial.Flush();
    }

    void PuppyBus::ErrorRecovery() {
        bufferedSerial.ErrorRecovery();
        Flush();
    }

    size_t PuppyBus::Read(uint8_t *buf, size_t len, uint32_t timeout) {
        bufferedSerial.SetReadTimeoutMs(timeout);
        auto res = bufferedSerial.Read(reinterpret_cast<char *>(buf), len, true);
        PuppyBus::last_operation_time_us = ticks_us();
        return res;
    }

    bool PuppyBus::Write(const uint8_t *buf, size_t len) {
        auto res = bufferedSerial.Write(reinterpret_cast<const char *>(buf), len) == len;
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

extern "C" void uart3_idle_cb() {
    buddy::puppies::PuppyBus::bufferedSerial.IdleISR();
}
