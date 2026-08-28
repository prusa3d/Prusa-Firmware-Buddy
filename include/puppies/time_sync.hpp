#pragma once

#include <cstdint>
#include <limits>
#include <array>

#include <filters/kalman.hpp>
#include "puppies/PuppyModbus.hpp"

namespace buddy::puppies {

class TimeSync {
public:
    TimeSync(const uint8_t id);

    void init();
    void sync(const uint32_t puppy_time_us, const RequestTiming timing);
    uint32_t buddy_time_us(const uint32_t puppy_time_us) const;
    bool is_time_sync_valid() const;

private:
    static constexpr auto RESPONSE_DURATION_EMA_WINDOW = 10;

    // Threshold for considering a sync response duration anomalous - inaccurate
    static constexpr auto RESPONSE_JITTER_THRESHOLD = 1.05f;

    const uint8_t id;

    uint32_t last_sync_us;
    int32_t last_offset_us;
    int32_t average_drift_ppb;
    int32_t average_puppy_offset_us;
    uint32_t average_response_us;

    KalmanFilterCallback drift_filter;
    KalmanFilterCallback offset_filter;

    int32_t correct_offset(const int32_t offset_us, const uint32_t now_us) const;
};

} // namespace buddy::puppies
