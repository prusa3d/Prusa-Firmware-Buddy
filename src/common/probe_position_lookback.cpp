
#include "probe_position_lookback.hpp"
#include "timing.h"

namespace buddy {
ProbePositionLookback probePositionLookback;

ProbePositionLookback::ProbePositionLookback()
    : newest_sample_pos(NUM_SAMPLES - 1) {
}

void ProbePositionLookback::update(float z_position) {
    uint32_t time = ticks_us();
    uint32_t last_sample_time = samples[newest_sample_pos].time;
    if (time - last_sample_time < SAMPLES_REQUESTED_DIFF) {
        // last sample still fresh enough - skip for now
        return;
    }

    add_sample(time, z_position);
}

void ProbePositionLookback::add_sample(uint32_t time, float position) {
    newest_sample_pos = (newest_sample_pos + 1) % NUM_SAMPLES;
    samples[newest_sample_pos].time = time;
    samples[newest_sample_pos].position = position;
}

float ProbePositionLookback::get_position_at(uint32_t time_us, std::function<float()> latest_z_position) {
    // store position of last sample before proceeding (new sample might be added later from interrupt)
    size_t s1_pos = newest_sample_pos;

    // get current sample so we can also interpolate between newest sample and now
    PositionSample_t current_sample;
    current_sample.position = latest_z_position();
    current_sample.time = ticks_us();
    PositionSample_t *s2 = &current_sample;

    while (true) {
        auto &s1 = samples[s1_pos];

        int32_t time_diff = ticks_diff(s2->time, s1.time);
        // s1 should be older than s2, if that is not the case, we wrapped thorough whole buffer.
        if (time_diff < 0) {
            return NAN;
        }

        // check if searched time is between s1 & s2, but in a way that is fine with timer overflow
        // s1.time s1.time <= time_us && time_us <= s2->time
        if (static_cast<uint32_t>(time_diff) >= (s2->time - time_us)) {
            float time_coef = (time_us - s1.time) / (float)time_diff;
            return s1.position + ((s2->position - s1.position) * time_coef);
        }

        s2 = &samples[s1_pos];
        s1_pos = (s1_pos == 0) ? NUM_SAMPLES - 1 : s1_pos - 1;

        // we reached newest sample again - stop
        if (s1_pos == newest_sample_pos) {
            return NAN;
        }
    }
}

} // namespace buddy
