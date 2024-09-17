#include "puppies/time_sync.hpp"

#include <cassert>
#include <math.h>

#include "timing.h"
#include "metric.h"

// #define TIME_SYNC_DEBUG

using namespace buddy::puppies;

#ifdef TIME_SYNC_DEBUG
METRIC_DEF(metric_buddy_time_us, "buddy_t", METRIC_VALUE_INTEGER, 0, METRIC_ENABLED);
METRIC_DEF(metric_puppy_time_us, "puppy_t", METRIC_VALUE_CUSTOM, 0, METRIC_ENABLED);
METRIC_DEF(metric_sync_roundtrip_us, "sync_rt", METRIC_VALUE_CUSTOM, 0, METRIC_ENABLED);
METRIC_DEF(metric_puppy_offset_us, "puppy_off", METRIC_VALUE_CUSTOM, 0, METRIC_ENABLED);
METRIC_DEF(metric_puppy_drift_ppb, "puppy_drift", METRIC_VALUE_CUSTOM, 0, METRIC_ENABLED);
METRIC_DEF(metric_puppy_average_offset_us, "puppy_aoff", METRIC_VALUE_CUSTOM, 0, METRIC_ENABLED);
METRIC_DEF(metric_puppy_average_drift_ppb, "puppy_adrif", METRIC_VALUE_CUSTOM, 0, METRIC_ENABLED);
#endif

TimeSync::TimeSync(const uint8_t id)
    : id(id)
    , drift_filter(120000, 120000, 0.01)
    , offset_filter(100, 100, 0.1, [this](double value, uint32_t now_us) { return correct_offset(value, now_us); }) {
    init();
}

void TimeSync::init() {
    // Reset time sync
    last_sync_us = std::numeric_limits<uint32_t>::max();
    last_offset_us = std::numeric_limits<int32_t>::max();
    average_drift_ppb = std::numeric_limits<int32_t>::max();
    average_puppy_offset_us = std::numeric_limits<int32_t>::max();
    average_roundtrip_us = std::numeric_limits<uint32_t>::max();
}

void TimeSync::sync(const uint32_t puppy_time_us, const RequestTiming timing) {
    // Discard sync that take unusually long time as it is inaccurate
    const uint32_t roundtrip_us = timing.end_us - timing.begin_us;
    if (average_roundtrip_us == std::numeric_limits<uint32_t>::max()) {
        average_roundtrip_us = roundtrip_us;
    } else {
        average_roundtrip_us = ((ROUNDTRIP_DURATION_SAMPLES - 1) * average_roundtrip_us + roundtrip_us) / ROUNDTRIP_DURATION_SAMPLES;
    }
    if (roundtrip_us > average_roundtrip_us * LONG_ROUNDTRIP_THRESHOLD) {
        // Skip sync, this sample is bad
        return;
    }

    // This assumes response_send_time == request_receive_time and puppy processing time == 0
    const uint32_t buddy_time_us = timing.begin_us + static_cast<uint32_t>(timing.end_us - timing.begin_us) / 2;

    // Compute raw offset info
    const int32_t puppy_offset_us = puppy_time_us - buddy_time_us;
    const uint32_t sync_interval_us = buddy_time_us - last_sync_us;

    // Compute filtered offset and drift
    if (last_offset_us != std::numeric_limits<int32_t>::max()) {
        average_puppy_offset_us = offset_filter.filter(puppy_offset_us, buddy_time_us);
        const int32_t puppy_drift_ppb = 1'000'000'000 * static_cast<int64_t>(puppy_offset_us - last_offset_us) / sync_interval_us;
        average_drift_ppb = drift_filter.filter(puppy_drift_ppb, buddy_time_us);

#ifdef TIME_SYNC_DEBUG
        metric_record_custom(&metric_sync_roundtrip_us, ",n=%d v=%d", id, roundtrip_us);
        metric_record_custom(&metric_puppy_time_us, ",n=%d v=%d", id, puppy_time_us);
        metric_record_integer(&metric_buddy_time_us, buddy_time_us);
        metric_record_custom(&metric_puppy_offset_us, ",n=%d v=%d", id, puppy_offset_us);
        metric_record_custom(&metric_puppy_average_offset_us, ",n=%d v=%d", id, average_puppy_offset_us);
        metric_record_custom(&metric_puppy_drift_ppb, ",n=%d v=%d", id, puppy_drift_ppb);
        metric_record_custom(&metric_puppy_average_drift_ppb, ",n=%d v=%d", id, average_drift_ppb);
#endif
    }

    last_offset_us = puppy_offset_us;
    last_sync_us = buddy_time_us;
}

int32_t TimeSync::correct_offset(const int32_t offset_us, const uint32_t now_us) const {
    uint32_t diff = now_us - last_sync_us;
    if (!diff) {
        return offset_us;
    }
    const int32_t correction_us = diff * static_cast<int64_t>(average_drift_ppb) / 1'000'000'000;
    return offset_us + correction_us;
}

bool TimeSync::is_time_sync_valid() const {
    return average_puppy_offset_us != std::numeric_limits<int32_t>::max() && average_drift_ppb != std::numeric_limits<int32_t>::max();
}

uint32_t TimeSync::buddy_time_us(const uint32_t puppy_time_us) const {
    assert(is_time_sync_valid());
    return puppy_time_us - correct_offset(average_puppy_offset_us, ticks_us());
}
