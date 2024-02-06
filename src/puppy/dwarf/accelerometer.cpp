#include "log.h"
#include "accelerometer.hpp"
#include "timing.h"
#include <cstdint>
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
stmdev_ctx_t dev_ctx;
std::atomic<bool> initialized = false;

LOG_COMPONENT_DEF(Accel, LOG_SEVERITY_INFO);

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

void init() {
    assert(!initialized);

    // Disable interrupt
    buddy::hw::lis2dh12_data.disableIRQ();

    // Initialize driver
    dev_ctx.write_reg = write_reg;
    dev_ctx.read_reg = read_reg;
    dev_ctx.handle = &SPI_HANDLE_FOR(accelerometer);

    // Soft reboot device
    lis2dh12_boot_set(&dev_ctx, PROPERTY_ENABLE);
    delay_ms(5);

    // Check device ID
    uint8_t who_am_i;
    lis2dh12_device_id_get(&dev_ctx, &who_am_i);
    if (who_am_i != LIS2DH12_ID) {
        log_critical(Accel, "Invalid device ID: 0x%x", who_am_i);
        return;
    }

    // Clear filter block by reading reference register
    uint8_t dummy;
    lis2dh12_filter_reference_get(&dev_ctx, &dummy);

    // Configure device
    lis2dh12_block_data_update_set(&dev_ctx, PROPERTY_ENABLE);
    lis2dh12_ctrl_reg3_t reg3 {};
    reg3.i1_zyxda = true;
    lis2dh12_pin_int1_config_set(&dev_ctx, &reg3);
    lis2dh12_full_scale_set(&dev_ctx, LIS2DH12_2g);
    lis2dh12_operating_mode_set(&dev_ctx, LIS2DH12_NM_10bit);
    lis2dh12_data_rate_set(&dev_ctx, LIS2DH12_ODR_5kHz376_LP_1kHz344_NM_HP);

    // Wait for sample and throw it away to make sure interrupt is not already pending
    lis2dh12_status_reg_t status {};
    while (!status.zyxda) {
        lis2dh12_status_get(&dev_ctx, &status);
    }
    int16_t dummy_accel[3];
    lis2dh12_acceleration_raw_get(&dev_ctx, dummy_accel);

    // Clear local data structures
    clear();

    // Mark initialized before enabling the IRQ
    initialized = true;

    // Enable new sample interrupt
    lis2dh12_data.enableIRQ();
}

void deinit() {
    assert(initialized);
    buddy::hw::lis2dh12_data.disableIRQ();
    initialized = false;
}

void on_new_samples(uint32_t count) {
    if (count == 0) {
        return;
    }

    uint32_t now = ticks_us();
    if (first_sample_timestamp == 0) {
        first_sample_timestamp = now;
        samples_extracted = 0;
    }
    last_sample_timestamp = now;
    samples_extracted += count;
}

} // end anonymous namespace

void dwarf::accelerometer::irq() {
    assert(initialized);
    if (!initialized) {
        return;
    }

    // Check status for overrun
    lis2dh12_status_reg_t status;
    lis2dh12_status_get(&dev_ctx, &status);

    // Get sample and store sample
    on_new_samples(1);
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

void dwarf::accelerometer::set_enable(bool enabled) {
    clear();

    if (initialized == enabled) {
        return;
    }

    if (enabled) {
        init();
    } else {
        deinit();
    }
}
float dwarf::accelerometer::measured_sampling_rate() {
    uint32_t duration = ticks_diff(last_sample_timestamp, first_sample_timestamp);
    if (duration == 0 || samples_extracted == 0) {
        return 0;
    }
    return (samples_extracted - 1) * 1000000.f / duration;
}
