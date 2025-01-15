#pragma once

#include <tuple>
#include <optional>
#include <atomic>
#include <device/hal.h>
#include <device/board.h>
#include <device/peripherals.h>
#include <freertos/timing.hpp>
#include <Pin.hpp>
#include <libs/circularqueue.h>
#include <timing.h>
#include <bsod.h>
#include <lis2dh12_reg.h>

/**
 * LIS2DH12Poller uses a timer and SPI DMA transfer to periodically check for a
 * new samples from the LIS2DH12 accelerometer. The samples are stored in an
 * internal queue and can be read by the user.
 *
 * This class substitutes the internal accelerometer FIFO which seems to be
 * unreliable.
 *
 * Note that since we don't use the accelerometer interrupt pin and we run
 * asynchronously to the accelerometer, there is an inherent race condition when
 * we read the accelerometer status, but before reading the data the
 * accelerometer already has new data. This occurs based on the sampling rate of
 * the accelerometer and the polling rate of the LIS2DH12Poller. In practice,
 * this is much much rarer compared to the observed FIFO issues.
 */
class LIS2DH12Poller {
    using Record = std::tuple<int16_t, int16_t, int16_t>;

    enum class Request {
        none,
        status,
        sample
    };

    SPI_HandleTypeDef *spi;
    buddy::hw::OutputPin chip_select;
    TIM_HandleTypeDef *polling_timer;
    stmdev_ctx_t stlib_context = {};

    std::atomic<bool> is_running;

    std::atomic<Request> pending_request = Request::none;
    uint8_t dma_buffer[7];

    uint32_t start_time = 0;
    std::atomic<bool> hw_good_flag = false;
    std::atomic<int> overflow_counter = 0;
    std::atomic<uint32_t> total_sample_count = 0;

    AtomicCircularQueue<Record, unsigned, 32> sample_queue;

    static int32_t write_reg(void *handle, uint8_t reg, const uint8_t *bufp, uint16_t len) {
        auto poller = static_cast<LIS2DH12Poller *>(handle);
        reg &= ~0b10000000; // Clear RW bit to force write command
        reg |= 0b01000000; // Set MS bit to force multiple command
        poller->chip_select.write(buddy::hw::Pin::State::low);
        HAL_StatusTypeDef reg_stat = HAL_SPI_Transmit(poller->spi, &reg, 1, 1000);
        HAL_StatusTypeDef data_stat = HAL_SPI_Transmit(poller->spi, (uint8_t *)bufp, len, 1000);
        poller->chip_select.write(buddy::hw::Pin::State::high);
        return reg_stat == HAL_OK && data_stat == HAL_OK ? 0 : -1;
    }

    static int32_t read_reg(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len) {
        auto poller = static_cast<LIS2DH12Poller *>(handle);
        reg |= 0b11000000; // Set RW and MS bit to force read command and multiple command
        poller->chip_select.write(buddy::hw::Pin::State::low);
        HAL_StatusTypeDef reg_stat = HAL_SPI_Transmit(poller->spi, &reg, 1, 1000);
        HAL_StatusTypeDef data_stat = HAL_SPI_Receive(poller->spi, bufp, len, 1000);
        poller->chip_select.write(buddy::hw::Pin::State::high);
        return reg_stat == HAL_OK && data_stat == HAL_OK ? 0 : -1;
    }

    void async_read_reg_start(uint8_t reg, uint16_t len) {
        chip_select.write(buddy::hw::Pin::State::high); // Be sure to end any pending transaction
        reg |= 0b11000000; // Set RW and MS bit to force read command and multiple command
        dma_buffer[0] = reg;
        chip_select.write(buddy::hw::Pin::State::low);
        auto res = HAL_SPI_TransmitReceive_DMA(spi, dma_buffer, dma_buffer, len + 1);
        if (res != HAL_OK) {
            hw_good_flag = false;
        }
    }

    void request_status_async() {
        async_read_reg_start(LIS2DH12_STATUS_REG, 1);
        pending_request = Request::status;
    }

    void request_sample_async() {
        async_read_reg_start(LIS2DH12_OUT_X_L, 6);
        pending_request = Request::sample;
    }

    void finalize_async_request_chain() {
        pending_request = Request::none;
    }

    bool is_chip_alive() {
        uint8_t who_am_i;
        lis2dh12_device_id_get(&stlib_context, &who_am_i);
        return who_am_i == LIS2DH12_ID;
    }

public:
    // Given an SPI handle, a chip select pin and roughly 3 kHz polling timer,
    // create a LIS2DH12Poller instance.
    LIS2DH12Poller(SPI_HandleTypeDef *spi, buddy::hw::OutputPin cs,
        TIM_HandleTypeDef *polling_timer)
        : spi { spi }
        , chip_select { cs }
        , polling_timer { polling_timer } {
        stlib_context.write_reg = write_reg;
        stlib_context.read_reg = read_reg;
        stlib_context.mdelay = nullptr;
        stlib_context.handle = this;
        chip_select.write(buddy::hw::Pin::State::high);
    }

    LIS2DH12Poller(const LIS2DH12Poller &) = delete;
    LIS2DH12Poller &operator=(const LIS2DH12Poller &) = delete;

    LIS2DH12Poller(LIS2DH12Poller &&other) = delete;
    LIS2DH12Poller &operator=(LIS2DH12Poller &&other) = delete;

    bool setup_accelerometer() {
        if (!is_chip_alive()) {
            hw_good_flag = false;
            return false;
        }

        lis2dh12_boot_set(&stlib_context, 1);
        freertos::delay(25);
        lis2dh12_boot_set(&stlib_context, 0);
        freertos::delay(5);

        lis2dh12_block_data_update_set(&stlib_context, 1);

        // switch between FIFO and Bypass to reset the accelerometer
        lis2dh12_fifo_mode_set(&stlib_context, LIS2DH12_FIFO_MODE);
        lis2dh12_fifo_mode_set(&stlib_context, LIS2DH12_BYPASS_MODE);

        lis2dh12_operating_mode_set(&stlib_context, LIS2DH12_NM_10bit);

        hw_good_flag = true;
        return true;
    }

    void start() {
        total_sample_count = 0;
        overflow_counter = 0;

        lis2dh12_data_rate_set(&stlib_context, LIS2DH12_ODR_5kHz376_LP_1kHz344_NM_HP);

        // Busy wait for the **second** sample to be ready in order to capture
        // the precise start time
        for (int i = 0; i != 2; i++) {
            lis2dh12_status_reg_t status;
            do {
                start_time = ticks_us();
                lis2dh12_status_get(&stlib_context, &status);
            } while (!status.zyxda);

            int16_t buf[3];
            lis2dh12_acceleration_raw_get(&stlib_context, buf);
        }

        pending_request = Request::none;
        is_running = true;
        if (HAL_TIM_Base_Start_IT(polling_timer) != HAL_OK) {
            bsod("Failed to start accelerometer polling timer");
        }
    }

    void stop() {
        is_running = false;
        HAL_TIM_Base_Stop_IT(polling_timer);
        HAL_SPI_Abort(spi);
        chip_select.write(buddy::hw::Pin::State::high);

        lis2dh12_data_rate_set(&stlib_context, LIS2DH12_POWER_DOWN);
    }

    int available() {
        return sample_queue.count();
    }

    std::optional<std::tuple<int16_t, int16_t, int16_t>> get_sample() {
        if (sample_queue.isEmpty()) {
            return std::nullopt;
        }
        return sample_queue.dequeue();
    }

    float get_sampling_rate() {
        if (start_time == 0) {
            return 0;
        }

        // In order to increase precision of the frequency measurement (as we
        // are limited by the frequency of the polling timer), we busy-poll for
        // two samples to get the precise end time.
        is_running = false;
        while (pending_request != Request::none)
            ;

        uint32_t end_time = 0;
        for (int i = 0; i != 2; i++) {
            lis2dh12_status_reg_t status;
            do {
                end_time = ticks_us();
                lis2dh12_status_get(&stlib_context, &status);
            } while (!status.zyxda);

            int16_t buf[3];
            lis2dh12_acceleration_raw_get(&stlib_context, buf);
            total_sample_count++;
            if (!sample_queue.enqueue(std::make_tuple(buf[0], buf[1], buf[2]))) {
                overflow_counter++;
            }
        }

        is_running = true;

        return total_sample_count / ((end_time - start_time) / 1'000'000.0f);
    }

    bool hw_good() const {
        return hw_good_flag;
    }

    int overflow_count() const {
        return overflow_counter;
    }

    void clear_overflow() {
        overflow_counter = 0;
    }

    // This routine should be called periodically from the polling timer ISR
    void polling_routine() {
        if (hw_good_flag && pending_request == Request::none && is_running) {
            request_status_async();
        }
    }

    // This routine should be called from the SPI TXRX ISR
    void spi_finish_routine() {
        chip_select.write(buddy::hw::Pin::State::high);

        if (pending_request == Request::none) {
            return;
        }

        if (HAL_SPI_GetState(spi) == HAL_SPI_STATE_ERROR) {
            hw_good_flag = false;
            finalize_async_request_chain();
            return;
        }

        // There is a slight problem with STM32 HAL SPI driver. This callback is
        // invoked from the DMA TX ISR. However, as DMA TX and RX share
        // priority, it is possible that te DMA RX ISR is invoked before the TX
        // ISR. Hence the handle of TX ISR is in busy state preventing further
        // DMA transactions. We know that all bits are shifted out, hence, it is
        // safe to clean it up manually:
        HAL_SPI_Abort(spi);
        HAL_DMA_Abort(spi->hdmarx);
        HAL_DMA_Abort(spi->hdmatx);

        if (HAL_SPI_GetState(spi) != HAL_SPI_STATE_READY) {
            bsod("Accelerometer SPI not ready");
        }

        if (pending_request == Request::status) {
            auto status = std::bit_cast<lis2dh12_status_reg_t>(dma_buffer[1]);
            if (status.zyxda) {
                request_sample_async();
            } else {
                finalize_async_request_chain();
            }

            if (status.zyxor) {
                overflow_counter++;
            }

            return;
        }

        if (pending_request == Request::sample) {
            int16_t x = (int16_t(dma_buffer[2]) << 8) | dma_buffer[1];
            int16_t y = (int16_t(dma_buffer[4]) << 8) | dma_buffer[3];
            int16_t z = (int16_t(dma_buffer[6]) << 8) | dma_buffer[5];
            if (!sample_queue.enqueue(std::make_tuple(x, y, z))) {
                overflow_counter++;
            }
            total_sample_count++;
            finalize_async_request_chain();
            return;
        }
    }
};
