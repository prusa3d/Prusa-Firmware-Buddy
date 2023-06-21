#include "loadcell.hpp"
#include "hx717.h"
#include "timing.h" // for ticks_ms
#include "log.h"
#include "circle_buffer.hpp"
#include "loadcell_shared.hpp"
#include "loadcell.hpp"

LOG_COMPONENT_REF(Dwarf);

namespace dwarf::loadcell {

// number of records in loadcell buffer
static constexpr size_t LOADCELL_BUFFER_RECORDS_NUM = 14;

// circular buffer that stores loadcell samples
CircleBuffer<LoadcellRecord, LOADCELL_BUFFER_RECORDS_NUM> sample_buffer;

// indicator that sample_buffer was full
uint32_t buffer_overflown = 0;

static bool loadcell_is_enabled = false;

void loadcell_init() {
    // read one value to unstuck the loadcell converter
    hx717.ReadValue(hx717.CHANNEL_A_GAIN_128);
}

void loadcell_loop() {
    static uint32_t buffer_over_reported_time = 0;
    if (buffer_overflown && (buffer_over_reported_time + 5000U) < ticks_ms()) {
        log_critical(Dwarf, "loadcell overflowed, %d samples didn't fit ", buffer_overflown);
        buffer_over_reported_time = ticks_ms();
        buffer_overflown = 0;
    }
}

// HX717 sample function. Sample both HX channels
void loadcell_irq() {
    if (!loadcell_is_enabled || !hx717.IsValueReady())
        return;

    int32_t raw_value = hx717.ReadValue(hx717.CHANNEL_A_GAIN_128);

    LoadcellRecord record;
    record.timestamp = ticks_us();
    record.loadcell_raw_value = raw_value;
    bool write_buffer_success = sample_buffer.push_back_DontRewrite(record);
    if (!write_buffer_success) {
        buffer_overflown++;
    }
}

bool get_loadcell_sample(LoadcellRecord &sample) {
    return sample_buffer.ConsumeFirst(sample);
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
}
