// wirring_digital - Buddy/STM32
#ifndef _WIRING_DIGITAL_H
#define _WIRING_DIGITAL_H

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void pinMode(uint32_t dwPin, uint32_t dwMode);

extern void digitalWrite(uint32_t marlinPin, uint32_t dwVal);

extern int digitalRead(uint32_t marlinPin);

#ifdef __cplusplus
}
#endif

#endif //_WIRING_DIGITAL_H
