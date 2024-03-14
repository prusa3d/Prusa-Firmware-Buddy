#include "hx717.hpp"
#include "disable_interrupts.h"
#include "bsod.h"
#include "timing.h"
#include <limits>
#include <algorithm>

using buddy::hw::hx717Dout;
using buddy::hw::hx717Sck;
using buddy::hw::Pin;

HX717 hx717;

HX717::HX717()
    : initialized(false)
    , currentChannel(CHANNEL_NOT_SET) {
}

void HX717::init(Channel configureChannel) {
    assert(configureChannel != CHANNEL_NOT_SET);

    initialized = false;
    currentChannel = configureChannel;

    // Force a power-down state to enforce a new conversion cycle on resume
    hx717Sck.write(Pin::State::high);
    delay_us_precise<100>(); // At least 80uS required according to DS
    hx717Sck.write(Pin::State::low);

    // Automatically reconfigure the IC on the next conversion cycle
    hx717Dout.clearIT();
    hx717Dout.enableIRQ();
}

/**
 * @brief Get ADC conversion result and start next conversion
 *
 * If this is called for the first time it returns result of CHANNEL_A_GAIN_128 conversion if
 * IsValueReady, which should be monitor through an interrupt as the read *must* be completed within
 * the current conversion cycle.
 *
 * If !IsValueReady or if the read isn't completed during the current version cycle the return value
 * and next channel selection is undefined until the next successfull read is completed.
 *
 * It ensures minimum SCK pulse duration of 200 ns.
 * It ensures maximum SCK high is lower than 60 us
 * by disabling interrupts for the duration of high
 * pulse of 200 ns. This avoids putting HX717 to
 * sleep by too long positive pulse.
 *
 * @param nextChannel Channel and gain for next conversion
 * @return Result of previous ADC conversion
 */
int32_t HX717::ReadValue(Channel nextChannel, uint32_t readyTimestamp) {
    // ensure init() has been called
    assert(currentChannel != CHANNEL_NOT_SET);
    assert(nextChannel != CHANNEL_NOT_SET);

    int32_t result = 0;
    buddy::DisableInterrupts disable_interrupts(false);
    static constexpr int32_t zero = 0;

    // Conservative guesses + roughly checked by debugging
    static constexpr int32_t pinWriteCycles = 1;
    static constexpr int32_t pinReadCycles = 2;
    static constexpr int32_t loopCycles = 2;
    static constexpr int32_t disableEnableIrqCycles = 2;

    if (!IsValueReady()) {
        goto reset;
    }

    // Disable the IRQ before it starts to trigger due to the data read
    hx717Dout.disableIRQ();

    // Read the value
    for (int index = 0; index < 24; index++) {
        disable_interrupts.disable();
        hx717Sck.write(Pin::State::high); //! Data are clocked out by rising edge of SCK
        timing_delay_cycles(std::max(zero, minDelayCycles - pinWriteCycles));
        hx717Sck.write(Pin::State::low);
        disable_interrupts.resume();

        timing_delay_cycles(std::max(zero, minDelayCycles - pinWriteCycles - pinReadCycles - loopCycles - disableEnableIrqCycles));
        //! Sample data in the last moment before next clock rising edge
        //! to maximize setting time
        result = (result << 1) | static_cast<int32_t>(hx717Dout.read());
        static_assert(0 == static_cast<int>(Pin::State::low));
        static_assert(1 == static_cast<int>(Pin::State::high));
    }

    // Clear the flag and re-arm the interrupt immediately
    hx717Dout.clearIT();
    hx717Dout.enableIRQ();

    // Configure the next read
    for (int index = 0; index < nextChannel; index++) {
        disable_interrupts.disable();
        hx717Sck.write(Pin::State::high);
        timing_delay_cycles(std::max(zero, minDelayCycles - pinWriteCycles));
        hx717Sck.write(Pin::State::low);
        disable_interrupts.resume();

        timing_delay_cycles(std::max(zero, minDelayCycles - pinWriteCycles - loopCycles - disableEnableIrqCycles));
    }

    // Perform an additional check to ensure no higher-priority ISR delayed this read
    if (ticks_diff(ticks_us(), readyTimestamp) > static_cast<int32_t>(1.f / HX717::sample_rate / 2.f * 1e6f)) {
        // We exceeded the maximum conversion cycle. Even though the state might be appear to be
        // consistent, we have no way to verify. Assume the IC can be in any random state.
        goto reset;
    }

    // extend 2's complement to 32bits
    if (result >= 0x800000) {
        result |= 0xFF000000;
    }

    if (IsValueReady() || result < -0x7FFFFF || result > 0x7FFFFF) {
        // DOUT should automatically switch off after a correct readout. If that happens, and/or if
        // the final result is out of spec, this means we have a communication issue.
        goto reset;
    }

    // At this point the read has been validated, update the internal configuration state
    sampleTimestamp = readyTimestamp - GetSamplingInterval();
    currentChannel = nextChannel;

    if (!initialized) {
        // The first successfull read also configures the IC to a known state. Mark as initialized,
        // but drop the value which is performed with an unknown configuration.
        initialized = true;
        return undefined_value;
    }

    return result;

reset:
    initialized = false;
    return undefined_value;
}
