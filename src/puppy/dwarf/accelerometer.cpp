#include "accelerometer.hpp"
#include "timing.h"
#include <cstdint>
#include <common/bsod.h>
#include <common/circular_buffer.hpp>
#include <atomic>
#include <cassert>
#include <lis2dh12_reg.h>
#include "hwio_pindef.h"

using namespace dwarf::accelerometer;
using namespace buddy::hw;

namespace {
static constexpr size_t ACCELEROMETER_BUFFER_RECORDS_NUM = 128;

CircularBuffer<AccelerometerRecord, ACCELEROMETER_BUFFER_RECORDS_NUM> sample_buffer;
uint32_t first_sample_timestamp = 0;
uint32_t last_sample_timestamp = 0;
size_t samples_extracted = 0;
size_t overflown_count = 0;

enum class State : uint8_t {
    uninitialized = 0,
    disabled,
    enabled,
};
State state = State::uninitialized;

void clear() {
    taskENTER_CRITICAL();
    sample_buffer.clear();
    taskEXIT_CRITICAL();
    overflown_count = 0;
    first_sample_timestamp = 0;
    last_sample_timestamp = 0;
    samples_extracted = 0;
}

/*
 * @brief  Write generic device register
 *
 * @param  handle    SPI handle
 * @param  reg       register to write
 * @param  bufp      pointer to data to write in register reg
 * @param  len       number of consecutive register to write
 * @return           0 if ok, -1 if error
 */
int32_t write_reg(void *handle, uint8_t reg, const uint8_t *bufp, uint16_t len) {
    reg &= ~0b10000000; // Clear RW bit to force write command
    reg |= 0b01000000; // Set MS bit to force multiple command
    acellCs.write(Pin::State::low);
    HAL_StatusTypeDef reg_stat = HAL_SPI_Transmit(static_cast<SPI_HandleTypeDef *>(handle), &reg, 1, 1000);
    HAL_StatusTypeDef data_stat = HAL_SPI_Transmit(static_cast<SPI_HandleTypeDef *>(handle), (uint8_t *)bufp, len, 1000);
    acellCs.write(Pin::State::high);
    return reg_stat == HAL_OK && data_stat == HAL_OK ? 0 : -1;
}

/*
 * @brief  Read generic device register
 *
 * @param  handle    SPI handle
 * @param  reg       register to read
 * @param  bufp      pointer to buffer that store the data read
 * @param  len       number of consecutive register to read
 * @return           0 if ok, -1 if error
 */
int32_t read_reg(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len) {
    reg |= 0b11000000; // Set RW and MS bit to force read command and multiple command
    acellCs.write(Pin::State::low);
    HAL_StatusTypeDef reg_stat = HAL_SPI_Transmit(static_cast<SPI_HandleTypeDef *>(handle), &reg, 1, 1000);
    HAL_StatusTypeDef data_stat = HAL_SPI_Receive(static_cast<SPI_HandleTypeDef *>(handle), bufp, len, 1000);
    acellCs.write(Pin::State::high);
    return reg_stat == HAL_OK && data_stat == HAL_OK ? 0 : -1;
}

stmdev_ctx_t dev_ctx {
    .write_reg = write_reg,
    .read_reg = read_reg,
    .mdelay = nullptr,
    .handle = &SPI_HANDLE_FOR(accelerometer),
};

void check_device_id() {
    uint8_t who_am_i;
    lis2dh12_device_id_get(&dev_ctx, &who_am_i);
    if (who_am_i != LIS2DH12_ID) {
        bsod("Invalid device ID: 0x%x", who_am_i);
    }
}

void configure_interrupt() {
    lis2dh12_ctrl_reg3_t reg3 {};
    reg3.i1_zyxda = true;
    lis2dh12_pin_int1_config_set(&dev_ctx, &reg3);
}

void enable_sampling() {
    constexpr lis2dh12_ctrl_reg1_t ctrl_reg1 {
        .xen = 1, // enable x axis
        .yen = 1, // enable y axis
        .zen = 1, // enable z axis
        .lpen = 0, // disable low power mode
        .odr = 0x9, // enable output
    };
    write_reg(dev_ctx.handle, LIS2DH12_CTRL_REG1, (const uint8_t *)&ctrl_reg1, 1);
}

void disable_sampling() {
    constexpr lis2dh12_ctrl_reg1_t ctrl_reg1 {
        .xen = 1, // enable x axis
        .yen = 1, // enable y axis
        .zen = 1, // enable z axis
        .lpen = 0, // disable low power mode
        .odr = 0x0, // disable output
    };
    write_reg(dev_ctx.handle, LIS2DH12_CTRL_REG1, (const uint8_t *)&ctrl_reg1, 1);
}

void throwaway_sample() {
    int16_t dummy_accel[3];
    lis2dh12_acceleration_raw_get(&dev_ctx, dummy_accel);
}

} // end anonymous namespace

void dwarf::accelerometer::enable() {
    switch (state) {
    case State::uninitialized:
        check_device_id();
        configure_interrupt();
        [[fallthrough]];
    case State::disabled:
        clear();
        enable_sampling();
        state = State::enabled;
        return;
    case State::enabled:
        return;
    }
}

void dwarf::accelerometer::disable() {
    switch (state) {
    case State::uninitialized:
        return;
    case State::disabled:
        return;
    case State::enabled:
        state = State::disabled;
        disable_sampling();
        throwaway_sample();
        clear();
        return;
    }
}

void dwarf::accelerometer::irq() {
    switch (state) {
    case State::uninitialized:
        return;
    case State::disabled:
        return;
    case State::enabled:
        break;
    }

    // Check status for overrun
    lis2dh12_status_reg_t status;
    lis2dh12_status_get(&dev_ctx, &status);

    // Get sample and store sample
    uint32_t now = ticks_us();
    if (first_sample_timestamp == 0) {
        first_sample_timestamp = now;
        samples_extracted = 0;
    }
    last_sample_timestamp = now;
    ++samples_extracted;

    AccelerometerRecord record;
    record.buffer_overflow = overflown_count > 0;
    record.sample_overrun = status.zyxor;
    lis2dh12_acceleration_raw_get(&dev_ctx, record.raw);
    // No need for locking, we are the only interrupt touching the sample buffer
    if (!sample_buffer.try_put(record)) {
        overflown_count++;
    }
}

bool dwarf::accelerometer::accelerometer_get_sample(AccelerometerRecord &sample) {
    taskENTER_CRITICAL();
    const bool ret = sample_buffer.try_get(sample);
    taskEXIT_CRITICAL();
    // Mark all outgoing packets as corrupted when there is an overflow
    sample.buffer_overflow = overflown_count > 0;
    return ret;
}

size_t dwarf::accelerometer::get_num_samples() {
    taskENTER_CRITICAL();
    const size_t size = sample_buffer.size();
    taskEXIT_CRITICAL();
    return size;
}

float dwarf::accelerometer::measured_sampling_rate() {
    uint32_t duration = ticks_diff(last_sample_timestamp, first_sample_timestamp);
    if (duration == 0 || samples_extracted == 0) {
        return 0;
    }
    return (samples_extracted - 1) * 1000000.f / duration;
}
