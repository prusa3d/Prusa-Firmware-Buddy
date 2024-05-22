// Loadcell and FilamentSensor muxer
#pragma once
#include "hx717.hpp"

namespace buddy::hw {

class HX717Mux {
public:
    void init();

    float get_sample_interval() const { return sample_interval; }
    HX717::Channel get_current_channel() const { return hx717.GetCurrentChannel(); }

protected:
    uint32_t ready_ts;
    inline void handler();
    friend void hx717_irq();
    friend void hx717_soft();

private:
    // channel switching sample counter
    uint8_t sample_counter;
    static const uint32_t sample_switch_count = 13;

    // current sample interval data
    uint32_t channel_switch_samples;
    uint32_t channel_switch_timestamp;
    float sample_interval;
};

extern HX717Mux hx717mux;

} // namespace buddy::hw
