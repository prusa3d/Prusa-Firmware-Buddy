/// @file adc.cpp
#include "adc.h"
#include <avr/io.h>

namespace hal {
namespace adc {

void Init() {
    ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
    ADMUX |= (1 << REFS0);
    ADCSRA |= (1 << ADEN);
}

uint16_t ReadADC(uint8_t channel) {
    uint8_t admux = ADMUX;
    admux &= ~0x1F;
    admux |= channel & 0x1F;
    ADMUX = admux;

    uint8_t adcsrb = ADCSRB;
    adcsrb &= ~(1 << MUX5);
    adcsrb |= channel & 0x20;
    ADCSRB = adcsrb;

    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC))
        ;

    return ADC;
}

} // namespace adc
} // namespace hal
