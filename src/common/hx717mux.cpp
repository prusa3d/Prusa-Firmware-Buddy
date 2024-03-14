#include "hx717mux.hpp"

#include "filament_sensors_handler.hpp"
#include "loadcell.hpp"

// Ensure coherence between loadcell and lower-level types/values without creating a circular header
// dependency between Loadcell and HX717Mux
static_assert(HX717::undefined_value == Loadcell::undefined_value);
static_assert(std::is_same_v<decltype(HX717::undefined_value), decltype(Loadcell::undefined_value)>);

namespace buddy::hw {

void HX717Mux::init() {
    // reset counters
    ready_ts = 0;
    sample_counter = 0;
    sample_interval = std::numeric_limits<float>::quiet_NaN();
    channel_switch_samples = 0;
    channel_switch_timestamp = 0;

    // init HX717
    hx717.init(hx717.CHANNEL_A_GAIN_128);
    hx717Soft.clearIT();
    hx717Soft.enableIRQ();
}

void HX717Mux::handler() {
    // Currently set channel
    HX717::Channel current_channel = hx717.GetCurrentChannel();

    // Configure the channel for the next read
    sample_counter = (sample_counter + 1) % sample_switch_count;
    HX717::Channel next_channel;
    if (!(loadcell.IsHighPrecisionEnabled()) && !sample_counter) {
        next_channel = hx717.CHANNEL_B_GAIN_8; // fs
    } else {
        next_channel = hx717.CHANNEL_A_GAIN_128; // loadcell
    }

    int32_t raw_value;

    int32_t read_delay = ticks_diff(ticks_us(), ready_ts);
    if (read_delay > static_cast<int32_t>(1.f / HX717::sample_rate / 32.f * 1e6f)) {
        // too much delay between trigger and read, skip the sample and wait for next cycle
        raw_value = HX717::undefined_value;
    } else {
        [[maybe_unused]] bool was_initialized = hx717.IsInitialized();

        // attempt to read the value
        raw_value = hx717.ReadValue(next_channel, ready_ts);

        // we already account for delayed reads above: we should never get an undefined value unless
        // the read itself took too long, meaning the interrupt took longer than ~1ms. If this
        // happens, the issue is *not* here but in higher-priority ISRs blocking too long!
        assert(!(was_initialized && raw_value == HX717::undefined_value));
    }

    // always provide an increasing timestamp for all (potentially invalid) reads
    uint32_t sample_timestamp = ready_ts - hx717.GetSamplingInterval();

    if (raw_value == HX717::undefined_value) {
        // invalid read: invalidate
        channel_switch_samples = 0;
        sample_interval = std::numeric_limits<float>::quiet_NaN();
    } else {
        if (current_channel != next_channel) {
            // switching channel: reset
            channel_switch_samples = 0;
            sample_interval = std::numeric_limits<float>::quiet_NaN();
        } else {
            // continous read on the same channel, estimate interval
            if (!channel_switch_samples) {
                channel_switch_timestamp = sample_timestamp;
            } else if (channel_switch_samples > 64 && channel_switch_samples < 320) {
                int32_t duration_ms = ticks_diff(sample_timestamp, channel_switch_timestamp) / 1000;
                sample_interval = static_cast<float>(duration_ms) / static_cast<float>(channel_switch_samples);
            }
            ++channel_switch_samples;
        }
    }

    // always forward the sample to the correct subsystem
    if (current_channel == hx717.CHANNEL_A_GAIN_128) {
        if (!std::isnan(sample_interval)) {
            loadcell.analysis.SetSamplingIntervalMs(sample_interval);
        }
        loadcell.ProcessSample(raw_value, sample_timestamp);
    } else {
        fs_process_sample(raw_value, 0);
    }
}

HX717Mux hx717mux;

void hx717_irq() {
    hx717mux.ready_ts = ticks_us();
    hx717Soft.triggerIT();
}

void hx717_soft() {
    hx717mux.handler();
}
} // namespace buddy::hw
