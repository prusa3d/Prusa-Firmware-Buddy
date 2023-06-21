// wiring_time.h - Buddy/STM32
#ifndef _WIRING_TIME_H
#define _WIRING_TIME_H

#ifdef __cplusplus
extern "C" {
#endif

extern uint32_t millis(void);

extern uint32_t micros(void);

extern void delay(uint32_t dwMs);

extern void delayMicroseconds(uint32_t usec);

#ifdef __cplusplus
}
#endif

#endif //_WIRING_TIME_H
