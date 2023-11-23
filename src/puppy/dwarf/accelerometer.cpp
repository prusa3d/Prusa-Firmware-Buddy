#include "log.h"
#include "accelerometer.hpp"
#include "SparkFunLIS2DH.h"
#include "timing.h"
#include <sys/_stdint.h>
#include <sys/types.h>
#include "circle_buffer.hpp"
#include <atomic>

using namespace dwarf::accelerometer;

namespace {
static constexpr size_t ACCELEROMETER_BUFFER_RECORDS_NUM = 60;
CircleBuffer<AccelerometerRecord, ACCELEROMETER_BUFFER_RECORDS_NUM> sample_buffer;
size_t overflown_count = 0;
size_t overflown_logged_count = 0;
bool accelerometer_enabled = false;
bool high_sample_rate = true;

enum {
    sample_rate_low = 400,
    sample_rate_high = 1344,
};

std::atomic<bool> accelerometer_irq_enabled = true;

class PreventInterrupt {
public:
    [[nodiscard]] PreventInterrupt() {
        accelerometer_irq_enabled = false;
    }

    ~PreventInterrupt() {
        accelerometer_irq_enabled = true;
    }
};

LOG_COMPONENT_DEF(Accelerometer, LOG_SEVERITY_WARNING);

LIS2DH driver(10);
} // end anonymous namespace

static void clear() {
    sample_buffer.clear();
    overflown_count = 0;
    overflown_logged_count = 0;
}

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

    clear();

    driver.fifoStartRec();
}

void dwarf::accelerometer::accelerometer_irq() {
    if (!driver.isSetupDone() || !accelerometer_irq_enabled) {
        return;
    }

    static uint32_t last_report = 0;
    static uint32_t sample_count = 0;
    static uint32_t max_at_once = 0;

    uint32_t samples_read = 0;
    static float dummy = 0;

    auto all_ones_before = driver.m_allOnesCounter;
    auto nonsuccess_before = driver.m_nonSuccessCounter;

    // TODO: Optimize read by reading sample count instead of reading empty flag over and over again.
    // TODO: Implement FIFO full interrupt and read all samples at once using a burst transfer.
    while ((driver.fifoGetStatus() & 0x1F)) { // not empty
        AccelerometerRecord record;
        record.timestamp = ticks_us();
        record.x = driver.readRawAccelX();
        record.y = driver.readRawAccelY();
        record.z = driver.readRawAccelZ();
        record.corrupted = overflown_count > 0;
        if (all_ones_before != driver.m_allOnesCounter || nonsuccess_before != driver.m_nonSuccessCounter) {
            break;
        }
        bool write_buffer_success = sample_buffer.push_back_DontRewrite(record);
        if (!write_buffer_success) {
            overflown_count++;
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
    if (overflown_count != overflown_logged_count && (buffer_over_reported_time + 5000U) < ticks_ms()) {
        log_critical(Accelerometer, "accelerometer overflowed, %d samples didn't fit ", overflown_count);
        buffer_over_reported_time = ticks_ms();
        overflown_logged_count = overflown_count;
    }
}

bool dwarf::accelerometer::accelerometer_get_sample(AccelerometerRecord &sample) {
    bool ret = sample_buffer.ConsumeFirst(sample);
    // Mark all outgoing packets as corrupted when there is an overflow
    sample.corrupted = overflown_count > 0;
    return ret;
}

size_t dwarf::accelerometer::get_num_samples() {
    return sample_buffer.Count();
}

void dwarf::accelerometer::set_enable(bool enabled) {
    PreventInterrupt irq_disabled;
    clear();
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
    PreventInterrupt irq_disabled;
    clear();
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
