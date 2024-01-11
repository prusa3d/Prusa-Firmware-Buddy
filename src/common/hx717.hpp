#pragma once
#include "timing_precise.hpp"
#include <inttypes.h>

// do not expose pin definitions unless necessary
#include <option/has_loadcell_hx717.h>
#if HAS_LOADCELL_HX717()
    #include "hwio_pindef.h"
#endif

class HX717 {

public:
    static constexpr int32_t undefined_value = std::numeric_limits<int32_t>::min();
    static constexpr int32_t min_value = 0xFF80'0000;
    static constexpr int32_t max_value = 0x007F'FFFF;
    static constexpr float sample_rate = 320.f; // Samples per second (Hz) for any channel

    enum Channel {
        CHANNEL_A_GAIN_128 = 1,
        CHANNEL_A_GAIN_64 = 3,
        CHANNEL_B_GAIN_32 = 2,
        CHANNEL_B_GAIN_8 = 4,
        CHANNEL_NOT_SET = -1,
    };

    HX717();

    void init(Channel configureChannel);

    /**
     * @brief Read the current sample when IsValueReady() has been triggered.
     * @param configureChannel Channel configuration for the next sample
     * @param readyTimestamp Timestamp (us) of DOUT for protocol synchronization.
     * @return Sample value, or undefined_value on error.
     */
    int32_t ReadValue(Channel configureChannel, uint32_t readyTimestamp);

    bool IsInitialized() {
        return initialized;
    }

#if HAS_LOADCELL_HX717()
    bool IsValueReady() {
        using namespace buddy::hw;
        return (Pin::State::low == hx717Dout.read()) ? true : false;
    }
#endif

    // Return the HAL ticks (us) required for a sample conversion:
    // NOTE: this is fixed in hw at the nominal frequency of sample_rate
    uint32_t GetSamplingInterval() const {
        return static_cast<uint32_t>(1e6f / sample_rate);
    }

    // Return the HAL ticks timestamp (us) of the last read value
    uint32_t GetSampleTimestamp() const {
        return sampleTimestamp;
    }

    // Return the HAL ticks timestamp (us) of the last read value
    Channel GetCurrentChannel() const {
        return currentChannel;
    }

private:
    // Minimum time for both low and high SCK
    // 200ns is minimum from datasheet. Added 10% margin to avoid being at
    // the edge and compensate for rounding errors while converting to cycles.
    static constexpr int32_t minDelayCycles = timing_nanoseconds_to_cycles(220);

    bool initialized;
    Channel currentChannel;
    uint32_t sampleTimestamp;
};

extern HX717 hx717;
