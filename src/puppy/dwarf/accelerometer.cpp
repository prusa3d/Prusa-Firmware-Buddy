#include "log.h"
#include "accelerometer.hpp"
#include "timing.h"
#include <cstdint>
#include "circle_buffer.hpp"
#include <atomic>
#include <lis2dh12_reg.h>
#include "hwio_pindef.h"

// #define DEBUG_SAMPLERATE

using namespace dwarf::accelerometer;
using namespace buddy::hw;

namespace {
static constexpr size_t ACCELEROMETER_BUFFER_RECORDS_NUM = 60;
static constexpr lis2dh12_odr_t low_sample_rate = LIS2DH12_ODR_400Hz;
static constexpr lis2dh12_odr_t high_sample_rate = LIS2DH12_ODR_5kHz376_LP_1kHz344_NM_HP;

CircleBuffer<AccelerometerRecord, ACCELEROMETER_BUFFER_RECORDS_NUM> sample_buffer;
size_t overflown_count = 0;
size_t overflown_logged_count = 0;
lis2dh12_odr_t current_sample_rate = low_sample_rate;
stmdev_ctx_t dev_ctx;
std::atomic<bool> initialized = false;

LOG_COMPONENT_DEF(Accel, LOG_SEVERITY_INFO);

#ifdef DEBUG_SAMPLERATE
class SampleRateDebug {
public:
    SampleRateDebug() {
        clear();
    }

    void clear() {
        last_timestamp_us = 0;
        last_milestone_timestamp_us = 0;
        max_interval = 0;
        min_interval = std::numeric_limits<uint32_t>::max();
        sample_counter = 0;
        last_milestone_time_us = 0;
        hist.fill(0);
    }

    void sample(uint32_t timestamp_us) {
        if (last_milestone_timestamp_us != 0 && sample_counter % milestone_interval == 0) {
            last_milestone_time_us = timestamp_us - last_milestone_timestamp_us;
            log_info(Accel, "%d samples in %d us", milestone_interval, last_milestone_time_us);
            last_milestone_timestamp_us = timestamp_us;
        }

        volatile const uint32_t diff = timestamp_us - last_timestamp_us;
        if (last_timestamp_us != 0 && diff < std::numeric_limits<uint32_t>::max() / 2) {
            if (max_interval < diff) {
                max_interval = diff;
            }
            if (min_interval > diff) {
                min_interval = diff;
            }

            assert(diff < hist.size() * hist_step_us);
            hist[diff / hist_step_us]++;
        }

        sample_counter++;
        last_timestamp_us = timestamp_us;
    }

    void report() {
        log_info(Accel, "Sample interval: min: %d us, max: %d us", min_interval, max_interval);

        size_t pos = 0;
        static char buff[8 * hist_buckets];
        for (size_t &val : hist) {
            pos += snprintf(buff + pos, sizeof(buff) - pos, "%d,", val);
        }
        log_info(Accel, buff);
    }

private:
    static constexpr size_t hist_step_us = 10;
    static constexpr uint32_t milestone_interval = 1000;
    static constexpr size_t hist_buckets = 200;
    std::array<size_t, hist_buckets> hist {};

    uint32_t last_timestamp_us = 0;
    uint32_t max_interval = 0;
    uint32_t min_interval = std::numeric_limits<uint32_t>::max();
    size_t sample_counter = 0;
    uint32_t last_milestone_timestamp_us = 0;
    uint32_t last_milestone_time_us = 0;
};
#else
class SampleRateDebug {
public:
    void clear() {}
    void sample([[maybe_unused]] uint32_t timestamp_us) {}
    void report() {}
};
#endif

SampleRateDebug debug;

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
    debug.clear();

    // Clear buffer
    sample_buffer.clear();
    overflown_count = 0;
    overflown_logged_count = 0;

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
    lis2dh12_data_rate_set(&dev_ctx, current_sample_rate);

    // Wait for sample and throw it away to make sure interrupt is not already pending
    lis2dh12_status_reg_t status {};
    while (!status.zyxda) {
        lis2dh12_status_get(&dev_ctx, &status);
    }
    int16_t dummy_accel[3];
    lis2dh12_acceleration_raw_get(&dev_ctx, dummy_accel);

    // Mark initialized before enabling the IRQ
    initialized = true;

    // Enable new sample interrupt
    lis2dh12_data.enableIRQ();
}

void deinit() {
    assert(initialized);
    buddy::hw::lis2dh12_data.disableIRQ();
    initialized = false;
    debug.report();
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
    AccelerometerRecord record;
    record.timestamp = ticks_us();
    record.buffer_overflow = overflown_count > 0;
    record.sample_overrun = status.zyxor;
    lis2dh12_acceleration_raw_get(&dev_ctx, record.raw);
    if (!sample_buffer.push_back_DontRewrite(record)) {
        overflown_count++;
    }

    debug.sample(record.timestamp);
}

void dwarf::accelerometer::accelerometer_loop() {
    static uint32_t buffer_over_reported_time = 0;
    if (overflown_count != overflown_logged_count && (buffer_over_reported_time + 5000U) < ticks_ms()) {
        log_critical(Accel, "accelerometer overflowed, %d samples didn't fit ", overflown_count);
        buffer_over_reported_time = ticks_ms();
        overflown_logged_count = overflown_count;
    }
}

bool dwarf::accelerometer::accelerometer_get_sample(AccelerometerRecord &sample) {
    bool ret = sample_buffer.ConsumeFirst(sample);
    // Mark all outgoing packets as corrupted when there is an overflow
    sample.buffer_overflow = overflown_count > 0;
    return ret;
}

size_t dwarf::accelerometer::get_num_samples() {
    return sample_buffer.Count();
}

void dwarf::accelerometer::set_enable(bool enabled) {
    if (initialized == enabled) {
        return;
    }

    if (enabled) {
        init();
    } else {
        deinit();
    }
}

bool dwarf::accelerometer::is_enabled() {
    return initialized;
}

void dwarf::accelerometer::set_high_sample_rate(bool high) {
    current_sample_rate = high ? high_sample_rate : low_sample_rate;

    if (!initialized) {
        return;
    }

    lis2dh12_data.disableIRQ();
    lis2dh12_data_rate_set(&dev_ctx, current_sample_rate);
    lis2dh12_data.enableIRQ();
}

bool dwarf::accelerometer::is_high_sample_rate() {
    return current_sample_rate == high_sample_rate;
}
