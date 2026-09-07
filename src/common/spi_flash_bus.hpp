/// @file
/// Shared SPI bus abstraction for flash memory devices.
///
/// Owns the SPI handle, DMA, and bus mutex. Multiple flash drivers
/// (NOR, NAND) share a single SpiFlashBus instance.
#pragma once

#include <cstdint>
#include <common/Pin.hpp>
#include <device/hal.h>
#include <freertos/binary_semaphore.hpp>
#include <freertos/mutex.hpp>

class SpiFlashBus {
public:
    // Currently less than 1 CPU cycle, so no delay is done
    static constexpr uint32_t cs_select_delay_ns = 5;
    static constexpr uint32_t cs_deselect_delay_ns = 50;

    static SpiFlashBus &instance();

    /// Initialize the SPI peripheral. Safe to call multiple times,
    /// only the first call performs the actual init.
    void init();

    /// Assert chip select (drive LOW), with setup time delay.
    void select(const buddy::hw::OutputPin &cs);

    /// Deassert chip select (drive HIGH), with deselect time delay.
    void deselect(const buddy::hw::OutputPin &cs);

    /// Send data over SPI. Uses DMA for transfers >4 bytes when available.
    void send(const uint8_t *buffer, uint32_t len);
    void send_byte(uint8_t byte);

    /// Receive data over SPI. Uses DMA for transfers >4 bytes when available.
    void receive(uint8_t *buffer, uint32_t len);
    uint8_t receive_byte();

    /// Set/fetch pending error state.
    void set_error(int error);
    int fetch_error();

    /// Acquire/release the bus mutex. Must be held for any SPI transaction.
    /// Becomes a no-op after reinit_for_crash_dump() (scheduler stopped).
    void lock();
    void unlock();

    /// Prepare for crash dump: abort DMA, disable bus mutex.
    /// After this call, lock/unlock become no-ops.
    void reinit_for_crash_dump();

    SPI_HandleTypeDef *handle() { return spi_handle; }
    bool is_scheduler_stopped() const { return scheduler_stopped; }

    /// DMA ISR callbacks -- call from HAL SPI callbacks.
    void on_tx_complete();
    void on_rx_complete();
    void on_error();

private:
    SPI_HandleTypeDef *spi_handle;

    /// Bus mutex protects the physical SPI bus from concurrent access
    /// by multiple flash drivers.
    freertos::Mutex bus_mutex;
    bool scheduler_stopped = false;

    /// We use the same semaphore for both receive and transmit,
    /// there must only be one operation in flight anyway.
    freertos::BinarySemaphore dma_semaphore;

    /// Status returned from DMA callbacks.
    volatile HAL_StatusTypeDef dma_status = HAL_OK;

    /// Buffer located in SRAM (not in core-coupled RAM).
    /// It is used when the caller gives us a buffer located in core-coupled
    /// RAM, which we can't pass to the DMA.
    uint8_t block_buffer[128];

    bool initialized = false;

    /// Pending error from last operation(s).
    int current_error = 0;

    explicit SpiFlashBus(SPI_HandleTypeDef *spi_handle);

    HAL_StatusTypeDef receive_dma(uint8_t *buffer, uint32_t len);
    HAL_StatusTypeDef send_dma(const uint8_t *buffer, uint32_t len);
    HAL_StatusTypeDef wait_for_dma(HAL_StatusTypeDef);
    void release_dma_from_isr(HAL_StatusTypeDef status);
};
