#include "log.h"
#include "accelerometer.hpp"
#include "SparkFunLIS2DH.h"
#include "timing.h"
#include <sys/_stdint.h>
#include <sys/types.h>
#include "circle_buffer.hpp"

using namespace dwarf::accelerometer;

static constexpr size_t ACCELEROMETER_BUFFER_RECORDS_NUM = 30;
static CircleBuffer<AccelerometerRecord, ACCELEROMETER_BUFFER_RECORDS_NUM> sample_buffer;
static bool buffer_overflown = false;
static bool accelerometer_enabled = false;
static bool high_sample_rate = true;
namespace {
enum {
    sample_rate_low = 400,
    sample_rate_high = 1344,
};
} // end anonymous namespace

LOG_COMPONENT_DEF(Accelerometer, LOG_SEVERITY_WARNING);

static LIS2DH driver(10);

static void accelerometer_init() {
    if (high_sample_rate) {
        driver.m_settings.accelSampleRate = sample_rate_high;
    } else {
        driver.m_settings.accelSampleRate = sample_rate_low;
    }

    driver.m_settings.fifoEnabled = true;
    driver.m_settings.fifoMode = 1;
    driver.m_settings.fifoThreshold = 30;
    driver.m_settings.tempEnabled = false;
    driver.m_settings.adcEnabled = false;

    driver.begin();

    driver.fifoBegin();
    driver.fifoClear();
    driver.fifoStartRec();
}

void dwarf::accelerometer::accelerometer_irq() {
    if (!driver.isSetupDone()) {
        return;
    }

    static uint32_t last_report = 0;
    static uint32_t sample_count = 0;
    static uint32_t max_at_once = 0;

    uint32_t samples_read = 0;
    static float dummy = 0;

    auto all_ones_before = driver.m_allOnesCounter;
    auto nonsuccess_before = driver.m_nonSuccessCounter;

    while ((driver.fifoGetStatus() & 0x1F)) { // not empty
        AccelerometerRecord record;
        record.timestamp = ticks_us();
        record.x = driver.readRawAccelX();
        record.y = driver.readRawAccelY();
        record.z = driver.readRawAccelZ();
        if (all_ones_before != driver.m_allOnesCounter || nonsuccess_before != driver.m_nonSuccessCounter) {
            break;
        }
        bool write_buffer_success = sample_buffer.push_back_DontRewrite(record);
        if (!write_buffer_success) {
            buffer_overflown = true;
        }
        samples_read += 1;
        sample_count += 1;
    }

    if (samples_read > max_at_once) {
        max_at_once = samples_read;
    }

    if (ticks_diff(ticks_ms(), last_report) > 1000) {
        log_info(Accelerometer, "Read %u samples (max %u), %f", sample_count, max_at_once, (double)dummy);
        last_report = ticks_ms();
        sample_count = 0;
        max_at_once = 0;
    }
}

void dwarf::accelerometer::accelerometer_loop() {
    static uint32_t buffer_over_reported_time = 0;
    if (buffer_overflown && (buffer_over_reported_time + 5000U) < ticks_ms()) {
        log_critical(Accelerometer, "accelerometer overflowed, %d samples didn't fit ", buffer_overflown);
        buffer_over_reported_time = ticks_ms();
        buffer_overflown = 0;
    }
}

bool dwarf::accelerometer::accelerometer_get_sample(AccelerometerRecord &sample) {
    return sample_buffer.ConsumeFirst(sample);
}

size_t dwarf::accelerometer::get_num_samples() {
    return sample_buffer.Count();
}

void dwarf::accelerometer::set_enable(bool enabled) {
    if (accelerometer_enabled == enabled)
        return;
    if (enabled) {
        accelerometer_init();
    } else {
        driver.end();
    }
    accelerometer_enabled = enabled;
}

bool dwarf::accelerometer::is_enabled() {
    return accelerometer_enabled;
}
void dwarf::accelerometer::set_high_sample_rate(bool high) {
    if (high_sample_rate == high)
        return;
    high_sample_rate = high;
    if (accelerometer_enabled) {
        accelerometer_init();
    }
}
bool dwarf::accelerometer::is_high_sample_rate() {
    return high_sample_rate;
}
