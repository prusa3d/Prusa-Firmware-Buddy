#include "adc.h"
#include "stub_adc.h"
#include <vector>

namespace hal {
namespace adc {

static TADCData values2Return[6];
static TADCData::const_iterator rdptr[6] = { values2Return[0].cbegin(), values2Return[1].cbegin() };
static uint8_t oversampleFactor = 1;
static uint8_t oversample = 1; ///< current count of oversampled values returned from the ADC - will get filled with oversampleFactor once it reaches zero

void ReinitADC(uint8_t channel, TADCData &&d, uint8_t ovsmpl) {
    values2Return[channel] = std::move(d);
    oversampleFactor = ovsmpl - 1;
    oversample = ovsmpl;
    rdptr[channel] = values2Return[channel].cbegin();
}

uint16_t CurrentADC(uint8_t adc) {
    return rdptr[adc] != values2Return[adc].end() ? *rdptr[adc] : values2Return[adc].back();
}

/// ADC access routines
uint16_t ReadADC(uint8_t adc) {
    if (!oversample) {
        if (rdptr[adc] != values2Return[adc].end())
            ++rdptr[adc];
        oversample = oversampleFactor;
    } else {
        --oversample;
    }
    return CurrentADC(adc);
}

void SetADC(uint8_t channel, uint16_t value) {
    ReinitADC(channel, TADCData({ value }), 1);
}

} // namespace adc
} // namespace hal
