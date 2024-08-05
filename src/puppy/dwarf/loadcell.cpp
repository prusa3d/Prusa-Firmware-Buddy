#include "loadcell.hpp"

#include <freertos/critical_section.hpp>
#include "hx717.hpp"
#include "timing.h" // for ticks_ms
#include <logging/log.hpp>
#include <common/circular_buffer.hpp>
#include "loadcell.hpp"
#include "bsod.h"

LOG_COMPONENT_REF(Dwarf);

namespace dwarf::loadcell {

// number of records in loadcell buffer
static constexpr size_t LOADCELL_BUFFER_RECORDS_NUM = 16;

// number of running average steps to filter the data
static constexpr size_t RUNNING_AVERAGE_STEPS { 10 };
static constexpr LoadcellSample_t MAX_DIFFERENCE { 50000 };
static constexpr size_t MAX_SKIPPED { 3 };

// circular buffer that stores loadcell samples
CircularBuffer<LoadcellRecord, LOADCELL_BUFFER_RECORDS_NUM> sample_buffer;

// indicator that sample_buffer was full
uint32_t buffer_overflown = 0;

// Running average sample value
LoadcellSample_t running_average = 0;

// Length of current skipped run
size_t skipped_samples = 0;

static bool loadcell_is_enabled = false;

void loadcell_init() {
    hx717.init(hx717.CHANNEL_A_GAIN_128);
}

void loadcell_loop() {
    static uint32_t buffer_over_reported_time = 0;
    if (buffer_overflown && (buffer_over_reported_time + 5000U) < ticks_ms()) {
        log_critical(Dwarf, "loadcell overflowed, %" PRIu32 " samples didn't fit ", buffer_overflown);
        buffer_over_reported_time = ticks_ms();
        buffer_overflown = 0;
    }
}

// HX717 sample function. Samples loadcell channel only.
void loadcell_irq() {
    if (!loadcell_is_enabled) {
        return;
    }

    uint32_t timestamp = ticks_us();

    [[maybe_unused]] bool was_initialized = hx717.IsInitialized();

    int32_t raw_value = hx717.ReadValue(hx717.CHANNEL_A_GAIN_128, timestamp);

    // we should never get an undefined value unless the read itself took too long, meaning the
    // interrupt took longer than ~1ms. If this happens, the issue is *not* here but in
    // higher-priority ISRs blocking too long!
    assert(!(was_initialized && raw_value == HX717::undefined_value));

    // always provide an increasing timestamp for all (potentially invalid) reads
    uint32_t sample_timestamp = timestamp - hx717.GetSamplingInterval();

    LoadcellRecord record;
    record.timestamp = sample_timestamp;
    record.loadcell_raw_value = raw_value;
    // No need for locking, we are the only interrupt touching the sample buffer
    if (!sample_buffer.is_full()) {
        sample_buffer.put(record);
    } else {
        buffer_overflown++;
    }
}

bool filter(const LoadcellRecord &sample) {
    bool ok = abs(sample.loadcell_raw_value - running_average) < MAX_DIFFERENCE;
    running_average = (RUNNING_AVERAGE_STEPS * running_average + sample.loadcell_raw_value) / (RUNNING_AVERAGE_STEPS + 1);
    return ok;
}

bool get_loadcell_sample(LoadcellRecord &sample) {
    // Consume samples until a good one is found
    for (;;) {
        {
            freertos::CriticalSection critical_section;
            if (sample_buffer.is_empty()) {
                return false;
            }
            sample = sample_buffer.get();
        }
        if (filter(sample)) {
            // Sample looks good
            skipped_samples = 0;
            return true;
        }
        if (skipped_samples > MAX_SKIPPED) {
            // Already skipped too many, needs to send something
            return true;
        }
        skipped_samples++;
    }
}

void loadcell_set_enable(bool enable) {
    // set status
    loadcell_is_enabled = enable;

    if (!enable) {
        // if loadcell was turned off, throw away all samples
        LoadcellRecord sample;
        while (get_loadcell_sample(sample)) {
        }
    }
}
} // namespace dwarf::loadcell
