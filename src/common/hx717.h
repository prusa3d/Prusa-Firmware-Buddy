#pragma once

#include "config_buddy_2209_02.h"
#ifdef LOADCELL_HX717

    #include "hwio_pindef.h"
    #include <inttypes.h>

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
        return (Pin::State::low == loadcellDout.read()) ? true : false;
    }
    float GetSampleRate() const {
        return sampleRate;
    }

private:
    Channel currentChannel;
    int consecutiveSamplesCount;
    uint32_t channelSwitchTimestamp;
    float sampleRate;
};

extern HX717 hx717;

#endif //LOADCELL_HX717
