#pragma once

#include <stdint.h>
#include <stddef.h>
#include <atomic>
#include <cmath>
#include <functional>

namespace buddy {

class ProbePositionLookback {
public:
    ProbePositionLookback();

    float get_position_at(uint32_t time_us, std::function<float()> latest_z_position);

    void update(float latest_z_position);

    void add_sample(uint32_t time, float position);

private:
    struct PositionSample_t {
        uint32_t time;
        float position;

        PositionSample_t()
            : time(0)
            , position(NAN) {}
    };

    static constexpr size_t NUM_SAMPLES = 16;
    static constexpr size_t SAMPLES_REQUESTED_DIFF = 1900;

    PositionSample_t samples[NUM_SAMPLES];
    std::atomic<size_t> newest_sample_pos;
};

extern ProbePositionLookback probePositionLookback;

} // namespace buddy
