#include "hx717.h"
#include "disable_interrupts.h"
#include "timing_precise.hpp"
#include <limits>
#include <algorithm>

HX717 hx717;

HX717::HX717()
    : currentChannel(static_cast<Channel>(0))
    , consecutiveSamplesCount(0)
    , channelSwitchTimestamp(0)
    , sampleRate(std::numeric_limits<float>::quiet_NaN()) {
}

/**
 * @brief Get ADC conversion result and start next conversion
 *
 * If this is called for the first time it returns result of
 * CHANNEL_A_GAIN_128 conversion if IsValueReady.
 *
 * If !IsValueReady it returns -1 or any possible garbled
 * nonsense.
 *
 * It ensures minimum SCK pulse duration of 200 ns.
 * It ensures maximum SCK high is lower than 60 us
 * by disabling interrupts for the duration of high
 * pulse of 200 ns. This avoids putting HX717 to
 * sleep by too long positive pulse.
 *
 * There is no risk of selecting wrong channel
 * and corrupting communication if this function
 * is interrupted for less than sensor sample period of 3.125 ms.
 *
 * @param nextChannel Channel and gain for next conversion
 * @return Result of previous ADC conversion
 */
int32_t HX717::ReadValue(Channel nextChannel) {
    using namespace buddy::hw;
    int32_t result = 0;
    buddy::DisableInterrupts disable_interrupts(false);
    static constexpr int32_t zero = 0;

    // Minimum time for both low and high SCK
    static constexpr int32_t minDelayCycles = timing_nanoseconds_to_cycles(200);
    // Conservative guesses + roughly checked by debugging
    static constexpr int32_t pinWriteCycles = 1;
    static constexpr int32_t pinReadCycles = 2;
    static constexpr int32_t loopCycles = 2;
    static constexpr int32_t disableEnableIrqCycles = 2;

    for (int index = 0; index < 24; index++) {
        disable_interrupts.disable();
        loadcellSck.write(Pin::State::high); //! Data are clocked out by rising edge of SCK
        timing_delay_cycles(std::max(zero, minDelayCycles - pinWriteCycles));
        loadcellSck.write(Pin::State::low);
        disable_interrupts.resume();

        timing_delay_cycles(std::max(zero, minDelayCycles - pinWriteCycles - pinReadCycles - loopCycles - disableEnableIrqCycles));
        //! Sample data in the last moment before next clock rising edge
        //! to maximize setting time
        result = (result << 1) | static_cast<int>(loadcellDout.read());
        static_assert(0 == static_cast<int>(Pin::State::low));
        static_assert(1 == static_cast<int>(Pin::State::high));
    }

    for (int index = 0; index < nextChannel; index++) {
        disable_interrupts.disable();
        loadcellSck.write(Pin::State::high);
        timing_delay_cycles(std::max(zero, minDelayCycles - pinWriteCycles));
        loadcellSck.write(Pin::State::low);
        disable_interrupts.resume();

        timing_delay_cycles(std::max(zero, minDelayCycles - pinWriteCycles - loopCycles - disableEnableIrqCycles));
    }

    sampleTimestamp = nextSampleTimestamp;
    nextSampleTimestamp = HAL_GetTick();

    if (currentChannel == nextChannel) {
        consecutiveSamplesCount++;
        if (consecutiveSamplesCount == 1) {
            channelSwitchTimestamp = nextSampleTimestamp;
        } else if (consecutiveSamplesCount > 20 && consecutiveSamplesCount < 300) {
            uint32_t durationMs = nextSampleTimestamp - channelSwitchTimestamp;
            sampleRate = static_cast<float>(durationMs) / static_cast<float>(consecutiveSamplesCount - 1);
        }
    } else {
        consecutiveSamplesCount = 0;
    }
    currentChannel = nextChannel;

    // convert 24 bit signed to 32 bit signed
    return (result >= 0x800000) ? (result | 0xFF000000) : result;
}
