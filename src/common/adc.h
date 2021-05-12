// adc.h
#pragma once

#include <inttypes.h>
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern uint32_t adc_val[ADC_CHAN_CNT];

extern void adc_init(void);

extern void adc_cycle(void);

#ifdef __cplusplus
}
#endif //__cplusplus
