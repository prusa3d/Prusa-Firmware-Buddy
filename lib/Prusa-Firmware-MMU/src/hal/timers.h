/// @file timers.h
#pragma once
#include <avr/io.h>

#define TIMER0 ((hal::timers::Tim8bit_TypeDef *)&TCCR0A)
namespace hal {

/// Hardware Abstraction Layer for the CPU's internal timers
namespace timers {

// void ConfigureTimer(uint8_t timer /* some config struct */);
// void StartTimer(uint8_t timer);
// void StopTimer(uint8_t timer);

struct Tim8bit_TypeDef {
    volatile uint8_t TCCRxA;
    volatile uint8_t TCCRxB;
    volatile uint8_t TCNTx;
    volatile uint8_t OCRxA;
    volatile uint8_t OCRxB;
};

static volatile uint8_t *const TIFR = &TIFR0;
static volatile uint8_t *const TIMSK = &TIMSK0;

struct Tim8bit_CTC_config {
    uint8_t cs : 3; ///clock source as per datasheet. It is not consistent between timer types
    uint8_t ocra; ///compare value for TOP
};

void Configure_CTC8(const uint8_t timer, Tim8bit_TypeDef *const htim8, Tim8bit_CTC_config *const conf);

} // namespace cpu
} // namespace hal
