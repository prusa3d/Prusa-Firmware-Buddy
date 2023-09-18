#pragma once
#include "hwio_pindef.h"
#include <inttypes.h>
#include "timing_precise.hpp"

class HX717 {

public:
    enum Channel {
        CHANNEL_A_GAIN_128 = 1,
        CHANNEL_A_GAIN_64 = 3,
        CHANNEL_B_GAIN_32 = 2,
        CHANNEL_B_GAIN_8 = 4
    };

    HX717();

    int32_t ReadValue(Channel configureChannel);

    bool IsValueReady() {
        using namespace buddy::hw;

        // For unknown reason hx717 does not run correctly after start with st490abdr rs422 drivers (xBuddy bom 39)
        // making it sleep and wake up fixes it
        static bool first_call = true;
        if (first_call) {
            first_call = false;

            // sleep
            loadcellSck.write(Pin::State::high);
            delay_us_precise<61>();
            // wake up
            loadcellSck.write(Pin::State::low);
            delay_ns_precise<100>();
        }

        return (Pin::State::low == loadcellDout.read()) ? true : false;
    }

    float GetSampleRate() const {
        return sampleRate;
    }

    // Return the HAL ticks timestamp (ms) of the last read value
    uint32_t GetSampleTimestamp() const {
        return sampleTimestamp;
    }

private:
    Channel currentChannel;
    int consecutiveSamplesCount;
    uint32_t channelSwitchTimestamp;
    uint32_t sampleTimestamp;
    uint32_t nextSampleTimestamp;
    float sampleRate;
};

extern HX717 hx717;
