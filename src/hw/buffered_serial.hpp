#pragma once

#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include <inttypes.h>
#include <stdbool.h>
#include <device/hal.h>
#include "cmsis_os.h"

#ifdef __cplusplus

namespace buddy {
namespace hw {

    class BufferedSerial {
    public:
        enum class CommunicationMode { IT,
            DMA };
        typedef void (*HalfDuplexSwitchCallback_t)(bool transmit);

        BufferedSerial(
            UART_HandleTypeDef *uart, BufferedSerial::HalfDuplexSwitchCallback_t halfDuplexSwitchCallback,
            uint8_t *rxBufPool, size_t rxBufPoolSize, BufferedSerial::CommunicationMode txMode);
        ~BufferedSerial();

        /// Get ready and enable data reception
        void Open();

        /// Read up to `len` bytes (while blocking the task to up to `GetReadTimeoutMs()` milliseconds).
        ///
        /// Returns: number of bytes read.
        size_t Read(char *buf, size_t len, bool terminate_on_idle = false);

        /// Send bytes over the serial (while blocking the task to up to `GetWriteTimeoutMs()` milliseconds)
        ///
        /// Returns: number of bytes successfully written
        size_t Write(const char *buf, size_t len);

        /// Clear the reception buffer.
        void Flush();

        /// Stop receiving more data.
        void Close();

        /// Get timeout for read operations
        uint32_t GetReadTimeoutMs() const { return readTimeoutMs; }

        /// Set timeout for read operations
        void SetReadTimeoutMs(uint32_t timeout) { readTimeoutMs = timeout; }

        /// Get timeout for write operations
        uint32_t GetWriteTimeoutMs() const { return writeTimeoutMs; }

        /// Set timeout for write operations
        void SetWriteTimeoutMs(uint32_t timeout) { writeTimeoutMs = timeout; }

        /// Should be called from the Transmission Complete ISR of its related USART
        void WriteFinishedISR();

        /// Should be called from the Idle ISR of its related USART
        void IdleISR();

        /// Should be called from the RxHalfCplt ISR of its related DMA
        void FirstHalfReachedISR();

        /// Should be called from the RxCplt ISR of its related DMA
        void SecondHalfReachedISR();

        /// DMA will stop receiving upon error (framing, parity etc), this will reinitialize it if needed
        void ErrorRecovery();

        struct uartrxbuff_t {
            DMA_HandleTypeDef *phdma;

            /// Event group used to synchronize reading the buffer with DMA/UART interrupts
            EventGroupHandle_t event_group;

            /// Pointer to the buffer's memory itself
            uint8_t *buffer;

            /// Size of the buffer
            int buffer_size;

            /// Index of the next position in the buffer to read from
            int buffer_pos;

            /// position in buffer where idle occured (UINT32_MAX when no idle occured)
            uint32_t idle_at_NDTR;
        };

    private:
        uint32_t readTimeoutMs;
        uint32_t writeTimeoutMs;
        bool isOpen;
        UART_HandleTypeDef *uart;
        uartrxbuff_t rxBuf;
        CommunicationMode txMode;
        HalfDuplexSwitchCallback_t halfDuplexSwitchCallback;
        EventGroupHandle_t eventGroup;
        bool pendingWrite;

        uint8_t *rxBufPool;
        size_t rxBufPoolSize;

        enum class Event : EventBits_t {
            WriteFinished = (1 << 0),
        };

        /// This will start the Receiving DMA (or restart it on error)
        void StartReceiving();
    };

} // namespace hw
} // namespace buddy

#endif
