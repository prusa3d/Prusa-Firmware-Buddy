// app.h
#ifndef _APP_H
#define _APP_H

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern void app_setup(void);

extern void app_run(void);

extern void app_error(void);

extern void app_assert(uint8_t *file, uint32_t line);

extern void app_cdc_rx(uint8_t *buffer, uint32_t length);

extern void app_tim6_tick(void);

extern void app_tim14_tick(void);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // _APP_H
