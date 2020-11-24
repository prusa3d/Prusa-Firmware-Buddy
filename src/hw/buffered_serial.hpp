#pragma once
#include "stm32f4xx_hal.h"
#include "uartrxbuff.h"
#include "FreeRTOS.h"

#ifdef __cplusplus

namespace buddy {
namespace hw {

    class BufferedSerial {
    public:
        BufferedSerial(
            UART_HandleTypeDef *uart, DMA_HandleTypeDef *rxDma, bool halfDuplex,
            uint8_t *rxBufPool, size_t rxBufPoolSize);
        ~BufferedSerial();

        /// Get ready and enable data reception
        void Open();

        /// Check if there are any received data in the buffer
        bool Available() const;

        /// Read up to `len` bytes (while blocking the task to up to `GetReadTimeoutMs()` milliseconds).
        ///
        /// Returns: number of bytes read.
        size_t Read(char *buf, size_t len);

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

        /// TODO: Get me out of here!
        static BufferedSerial uart2;

        /// Should be called from the Transmission Complete ISR of its related USART
        void WriteFinishedISR();

        /// Should be called from the Idle ISR of its related USART
        void IdleISR();

        /// Should be called from the RxHalfCplt ISR of its related DMA
        void FirstHalfReachedISR();

        /// Should be called from the RxCplt ISR of its related DMA
        void SecondHalfReachedISR();

    private:
        uint32_t readTimeoutMs;
        uint32_t writeTimeoutMs;
        bool isOpen;
        UART_HandleTypeDef *uart;
        DMA_HandleTypeDef *rxDma;
        uartrxbuff_t rxBuf;
        bool isHalfDuplex;
        EventGroupHandle_t eventGroup;
        bool pendingWrite;

        enum class Event : EventBits_t {
            WriteFinished = (1 << 0),
        };
    };

}
}

#endif

//
// FIXME: remove uart2 definition from this file
//
#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

void uart2_idle_cb();

#ifdef __cplusplus
}
#endif //__cplusplus
