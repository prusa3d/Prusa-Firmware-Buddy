//wiring_analog.h - Buddy/STM32
#ifndef _WIRING_ANALOG_H
#define _WIRING_ANALOG_H

#include <inttypes.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

/*typedef enum _eAnalogReference
{
	AR_DEFAULT,
} eAnalogReference ;*/

extern void analogWrite(uint32_t ulPin, uint32_t ulValue);

extern uint32_t analogRead(uint32_t ulPin);

inline void analogReadResolution(int res) {
    assert(12 == res);
    return;
}

#ifdef __cplusplus
}
#endif

#endif //_WIRING_ANALOG_H
