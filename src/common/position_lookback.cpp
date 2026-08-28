
#include "position_lookback.hpp"
#include <raii/scope_guard.hpp>
#include <bsod/bsod.h>
#include "timing.h"

#ifndef UNITTESTS
    #include <module/planner.h>
#endif

namespace buddy {

void PositionLookbackBase::add_sample(Sample sample) {
    const auto new_newest_sample_pos = (newest_sample_pos + 1) % NUM_SAMPLES;
    auto &new_newest_sample = samples[new_newest_sample_pos];

    // First invalidate the position to indicate that the record is being manipulated with
    // get_position_at could interrupt this function, so it needs to know that the sample is invalid
    new_newest_sample.x = NAN;

    new_newest_sample.time = sample.time;
    // X is not missing, continue reading the code.
    new_newest_sample.y = sample.y;
    new_newest_sample.z = sample.z;
    new_newest_sample.e = sample.e;
    // The X is intentionally written last to signal that the sample is ready
    new_newest_sample.x = sample.x;

    // Update the sample as the last thing to reduce the probability of the reader hitting the value
    newest_sample_pos = new_newest_sample_pos;
}

xyze_pos_t PositionLookbackBase::get_position_at(uint32_t time_us) const {
    // store position of last sample before proceeding (new sample might be added later from interrupt)
    size_t s1_pos = newest_sample_pos;

    // get current sample so we can also interpolate between newest sample and now
    //! Important: The sample needs to be generaed AFTER first loading newest_sample_position
    // Otherwise, we might end up with s2.time < s1.time right from the start
    Sample s2 = generate_sample();
    static constexpr xyze_pos_t invalid_sample = {
        .x = NAN,
        .y = NAN,
        .z = NAN,
        .e = NAN
    };

    while (true) {
        const Sample s1 {
            .time = samples[s1_pos].time,
            .x = samples[s1_pos].x,
            .y = samples[s1_pos].y,
            .z = samples[s1_pos].z,
            .e = samples[s1_pos].e,
        };
        if (s1.time != samples[s1_pos].time.load()) {
            // The sample got updated under our hands.
            // This effectively means that we've wrapped around the buffer and can just give up
            // Note: checking time here because it is the first member read during s1 initialization
            return invalid_sample;
        }

        // If the position is NAN, it means that the sample is currently being updated (which means that it's the last sample and we can fail)
        if (std::isnan(s1.x)) {
            return invalid_sample;
        }

        const int32_t time_diff = ticks_diff(s2.time, s1.time);

        // s1 should be older than s2, if that is not the case, we wrapped through whole buffer.
        if (time_diff < 0) {
            return invalid_sample;
        }

        // check if searched time is between s1 & s2, but in a way that is fine with timer overflow
        // s1.time s1.time <= time_us && time_us <= s2->time
        if (static_cast<uint32_t>(time_diff) >= (s2.time - time_us)) {
            float time_coef = (time_us - s1.time) / (float)time_diff;
            return xyze_pos_t {
                .x = s1.x + ((s2.x - s1.x) * time_coef),
                .y = s1.y + ((s2.y - s1.y) * time_coef),
                .z = s1.z + ((s2.z - s1.z) * time_coef),
                .e = s1.e + ((s2.e - s1.e) * time_coef),
            };
        }

        s2 = s1;
        s1_pos = (s1_pos + NUM_SAMPLES - 1) % NUM_SAMPLES;

        // we reached newest sample again - stop
        if (s1_pos == newest_sample_pos) {
            return invalid_sample;
        }
    }
}

#ifndef UNITTESTS
void PositionLookback::update() {
    // Check that we are in an ISR
    debug_assert(__get_IPSR());

    const Sample sample = generate_sample();

    if (sample.time - samples[newest_sample_pos].time < SAMPLES_REQUESTED_DIFF) {
        // last sample still fresh enough - skip for now
        return;
    }

    add_sample(sample);
}

PositionLookback::Sample PositionLookback::generate_sample() const {
    return Sample {
        .time = ticks_us(),
        .x = planner.get_axis_position_mm(AxisEnum::X_AXIS),
        .y = planner.get_axis_position_mm(AxisEnum::Y_AXIS),
        .z = planner.get_axis_position_mm(AxisEnum::Z_AXIS),
        .e = planner.get_axis_position_mm(AxisEnum::E_AXIS)
    };
}

PositionLookback positionLookback;
#endif

} // namespace buddy
