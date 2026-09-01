#include "puppies/time_sync.hpp"

#include <math.h>

#include "timing.h"
#include "metric.h"
#include <bsod/bsod.h>

// #define TIME_SYNC_DEBUG

using namespace buddy::puppies;

#ifdef TIME_SYNC_DEBUG
// #error dead code found by automatic analyses (see BFW-5461)
METRIC_DEF(metric_buddy_time_us, "buddy_t", METRIC_VALUE_INTEGER, 0, METRIC_ENABLED);
METRIC_DEF(metric_puppy_time_us, "puppy_t", METRIC_VALUE_CUSTOM, 0, METRIC_ENABLED);
METRIC_DEF(metric_sync_response_us, "sync_resp", METRIC_VALUE_CUSTOM, 0, METRIC_ENABLED);
METRIC_DEF(metric_puppy_offset_us, "puppy_off", METRIC_VALUE_CUSTOM, 0, METRIC_ENABLED);
METRIC_DEF(metric_puppy_drift_ppb, "puppy_drift", METRIC_VALUE_CUSTOM, 0, METRIC_ENABLED);
METRIC_DEF(metric_puppy_average_offset_us, "puppy_aoff", METRIC_VALUE_CUSTOM, 0, METRIC_ENABLED);
METRIC_DEF(metric_puppy_average_drift_ppb, "puppy_adrif", METRIC_VALUE_CUSTOM, 0, METRIC_ENABLED);
#endif

TimeSync::TimeSync(const uint8_t id)
    : id(id)
    , drift_filter(120000, 0.01, 120000, 0, [](double value, uint32_t) { return value; })
    , offset_filter(100, 0.1, 100, 0, [this](double value, uint32_t now_us) { return correct_offset(static_cast<int32_t>(value), now_us); }) {
    init();
}

void TimeSync::init() {
    // Reset time sync
    last_sync_us = std::numeric_limits<uint32_t>::max();
    last_offset_us = std::numeric_limits<int32_t>::max();
    average_drift_ppb = std::numeric_limits<int32_t>::max();
    average_puppy_offset_us = std::numeric_limits<int32_t>::max();
    average_response_us = std::numeric_limits<uint32_t>::max();
}

void TimeSync::sync(const uint32_t puppy_time_us, const RequestTiming timing) {
    const uint32_t response_us = timing.response_end_us - timing.request_end_us;
    if (average_response_us == std::numeric_limits<uint32_t>::max()) {
        average_response_us = response_us;
    } else {
        average_response_us = ((RESPONSE_DURATION_EMA_WINDOW - 1) * average_response_us + response_us) / RESPONSE_DURATION_EMA_WINDOW;
    }
    if (response_us > average_response_us + RESPONSE_JITTER_TOLERANCE_US
        || response_us + RESPONSE_JITTER_TOLERANCE_US < average_response_us) {
        // Puppy processing and response frame duration are constant per request type,
        // so a scheduling delay shows up as a deviation of the response duration.
        // Discard such syncs as inaccurate.
        return;
    }

    // The puppy samples its clock right after receiving the request, so the end
    // of the request frame is the closest wire event the buddy can timestamp.
    // The residual error (puppy-side frame detection and parsing) biases
    // converted timestamps slightly to the past.
    const uint32_t buddy_time_us = timing.request_end_us;

    // Compute raw offset info
    const int32_t puppy_offset_us = puppy_time_us - buddy_time_us;
    const uint32_t sync_interval_us = buddy_time_us - last_sync_us;

    // Compute filtered offset and drift
    if (last_offset_us != std::numeric_limits<int32_t>::max()) {
        average_puppy_offset_us = static_cast<int32_t>(offset_filter.filter(puppy_offset_us, buddy_time_us));
        const int32_t puppy_drift_ppb = 1'000'000'000 * static_cast<int64_t>(puppy_offset_us - last_offset_us) / sync_interval_us;
        average_drift_ppb = static_cast<int32_t>(drift_filter.filter(puppy_drift_ppb, 0));

#ifdef TIME_SYNC_DEBUG
        // #error dead code found by automatic analyses (see BFW-5461)
        metric_record_custom(&metric_sync_response_us, ",n=%d v=%d", id, response_us);
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
    debug_assert(is_time_sync_valid());
    const uint32_t now_us = ticks_us();
    const uint32_t converted_us = puppy_time_us - correct_offset(average_puppy_offset_us, now_us);
    // The converted event has already happened, so a time ahead of now is residual error
    return ticks_diff(converted_us, now_us) > 0 ? now_us : converted_us;
}
