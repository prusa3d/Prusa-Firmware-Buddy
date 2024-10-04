#include "w25x_communication.h"
#include "w25x.h"
#include "string.h"
#include <logging/log.hpp>
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include <buddy/ccm_thread.hpp>
#include "stm32f4xx_hal.h"

LOG_COMPONENT_REF(W25X);

/// The SPI used by this module
static SPI_HandleTypeDef *spi_handle;

/// Timeout for SPI operations
static const uint32_t TIMEOUT_MS = 1000;

/// Event group used to wake up a thread when SPI operation completes
static EventGroupHandle_t event_group;

/// Buffer located in SRAM (not in core-coupled RAM)
/// It is used when we user gives us a buffer located in core-coupled RAM, which we can't pass to the DMA.
static uint8_t block_buffer[128];

/// Pending error from last operation(s)
static int current_error;

/// Check whether there is no pending error
static inline bool no_error();

/// Convert HAL error to our generic error
static inline int hal_status_to_error(HAL_StatusTypeDef status);

/// Set current error
static inline void set_error(int error);

/// Check whether buffer at given location can be passed to a DMA
/// (the buffer must be in standard ram, not the core-coupled one)
static inline bool memory_supports_dma_transfer(const void *location);

/// Check whether we can use the DMA from the current context
static inline bool dma_is_available();

/// Receive data over DMA
static int receive_dma(uint8_t *buffer, uint32_t len);

/// Send data over DMA
static int send_dma(const uint8_t *buffer, uint32_t len);

enum {
    EVENT_RX_COMPLETE = (1 << 0),
    EVENT_TX_COMPLETE = (1 << 1),
};

extern "C" bool w25x_communication_init(bool init_event_group) {
    // check SPI handle has been assigned using w25x_spi_assign
    if (spi_handle == NULL) {
        return false;
    }

    // clear error
    current_error = 0;

    // create an eventgroup for ISR DMA events
    if (init_event_group && event_group == NULL) {
        event_group = xEventGroupCreate();
        if (event_group == NULL) {
            return false;
        }
    }

    return true;
}

extern "C" bool w25x_communication_abort() {
    if (spi_handle == NULL) {
        return false;
    }

    (void)HAL_SPI_Abort(spi_handle);
    return true;
}

int w25x_fetch_error() {
    int error = current_error;
    current_error = 0;
    return error;
}

extern "C" void w25x_receive(uint8_t *buffer, uint32_t len) {
    if (!no_error()) {
        return;
    }

    if (len > 1 && dma_is_available()) {
        if (memory_supports_dma_transfer(buffer)) {
            set_error(receive_dma(buffer, len));
            return;
        } else {
            while (no_error() && len) {
                uint32_t block_len = len > sizeof(block_buffer) ? sizeof(block_buffer) : len;
                set_error(receive_dma(block_buffer, block_len));
                memcpy(buffer, block_buffer, block_len);
                buffer += block_len;
                len -= block_len;
            }
        }
    } else {
        HAL_StatusTypeDef status = HAL_SPI_Receive(spi_handle, buffer, len, TIMEOUT_MS);
        set_error(hal_status_to_error(status));
    }
}

extern "C" uint8_t w25x_receive_byte() {
    uint8_t byte;
    w25x_receive(&byte, 1);
    return byte;
}

extern "C" void w25x_send(const uint8_t *buffer, uint32_t len) {
    if (len > 1 && dma_is_available()) {
        if (memory_supports_dma_transfer(buffer)) {
            set_error(send_dma(buffer, len));
        } else {
            while (no_error() && len) {
                uint32_t block_len = len > sizeof(block_buffer) ? sizeof(block_buffer) : len;
                memcpy(block_buffer, buffer, block_len);
                set_error(send_dma(block_buffer, block_len));
                buffer += block_len;
                len -= block_len;
            }
        }
    } else {
        HAL_StatusTypeDef status = HAL_SPI_Transmit(spi_handle, (uint8_t *)buffer, len, TIMEOUT_MS);
        set_error(hal_status_to_error(status));
    }
}

extern "C" void w25x_send_byte(uint8_t byte) {
    w25x_send(&byte, sizeof(byte));
}

static inline int hal_status_to_error(HAL_StatusTypeDef status) {
    return (int)status;
}

static inline void set_error(int error) {
    current_error = error;
}

static inline bool no_error() {
    return current_error == 0;
}

static inline bool memory_supports_dma_transfer(const void *location) {
    return (uintptr_t)location >= 0x20000000;
}

static inline bool dma_is_available() {
    return event_group && !xPortIsInsideInterrupt() && xTaskGetSchedulerState() == taskSCHEDULER_RUNNING;
}

static int receive_dma(uint8_t *buffer, uint32_t len) {
    xEventGroupClearBits(event_group, EVENT_RX_COMPLETE);

    // initiate DMA transfer
    assert(can_be_used_by_dma(buffer));
    HAL_StatusTypeDef status = HAL_SPI_Receive_DMA(spi_handle, buffer, len);
    if (status != HAL_OK) {
        return hal_status_to_error(status);
    }

    // wait for the receive-complete callback
    EventBits_t bits = xEventGroupWaitBits(event_group, EVENT_RX_COMPLETE,
        /*xClearOnExit=*/pdTRUE,
        /*xWaitForAllBits=*/pdTRUE,
        /*xTicksToWait=*/TIMEOUT_MS / portTICK_PERIOD_MS);

    // check we did not timeout
    if ((bits & EVENT_RX_COMPLETE) == 0) {
        log_error(W25X, "DMA read timed out");
        HAL_SPI_Abort(spi_handle);
        return hal_status_to_error(HAL_TIMEOUT);
    }

    return 0;
}

static int send_dma(const uint8_t *buffer, uint32_t len) {
    xEventGroupClearBits(event_group, EVENT_TX_COMPLETE);

    // initiate DMA transfer
    assert(can_be_used_by_dma(buffer));
    HAL_StatusTypeDef status = HAL_SPI_Transmit_DMA(spi_handle, (uint8_t *)buffer, len);
    if (status != HAL_OK) {
        log_error(W25X, "DMA write error %u", status);
        return hal_status_to_error(status);
    }

    // wait for the transmit-complete callback
    EventBits_t bits = xEventGroupWaitBits(event_group, EVENT_TX_COMPLETE,
        /*xClearOnExit=*/pdTRUE,
        /*xWaitForAllBits=*/pdTRUE,
        /*xTicksToWait=*/TIMEOUT_MS / portTICK_PERIOD_MS);

    // check we did not timeout
    if ((bits & EVENT_TX_COMPLETE) == 0) {
        log_error(W25X, "DMA write timed out");
        HAL_SPI_Abort(spi_handle);
        return hal_status_to_error(HAL_TIMEOUT);
    }

    return 0;
}

static void set_events_from_isr(EventBits_t events) {
    if (event_group == NULL) {
        return;
    }
    BaseType_t higherPriorityTaskWoken = pdFALSE;
    BaseType_t result = xEventGroupSetBitsFromISR(event_group, events, &higherPriorityTaskWoken);
    if (result != pdFAIL) {
        // Switch context after returning from ISR if we have just woken a higher-priority task
        portYIELD_FROM_ISR(higherPriorityTaskWoken);
    }
}

void w25x_spi_assign(SPI_HandleTypeDef *spi) {
    spi_handle = spi;
}

void w25x_spi_transfer_complete_callback(void) {
    set_events_from_isr(EVENT_TX_COMPLETE);
}

void w25x_spi_receive_complete_callback(void) {
    set_events_from_isr(EVENT_RX_COMPLETE);
}

void w25x_set_error(int error) {
    set_error(error);
}
