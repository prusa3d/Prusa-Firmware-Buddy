#pragma once

#include <cstdlib>
#include <device/peripherals.h>

namespace buddy {
namespace puppies {

    class PuppyBus {
    public:
        /// Bus access mutex
        class LockGuard {
            bool locked = false; ///< Set if the lock was acquired

        public:
            /// Acquire lock, bsod if it fails.
            [[nodiscard]] LockGuard();

            /**
             * @brief Try to acquire lock, don't bsod but return status.
             * @param is_locked set to true if the lock was acquired
             * Variable is_locked needs to be checked after this call.
             */
            [[nodiscard]] LockGuard(bool &is_locked);

            ~LockGuard();
        };

        /// Prepare puppy bus
        static void Open();

        /// Close puppy bus
        static void Close();

        /// Read data from puppy bus
        static size_t Read(uint8_t *buf, size_t len, uint32_t timeout);

        /// Write data to puppy bus
        static bool Write(const uint8_t *buf, size_t len);

        /// Reinitialize bus
        static void ErrorRecovery();

        /// Flush data in receive buffer
        static void Flush();

        /// callback for switching Transmit Enable pin of RS485
        static void HalfDuplexCallbackSwitch(bool transmit);

        /// Calling whis will ensure that there is delay on bus between two messages
        static void EnsurePause();

    private:
        /// Time when last operation was done
        static uint32_t last_operation_time_us;

        /// Minimal pause that EnsurePause will enforce
        static constexpr uint32_t MINIMAL_PAUSE_BETWEEN_REQUESTS_US = 350;
    };

} // namespace puppies
} // namespace buddy
