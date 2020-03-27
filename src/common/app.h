// app.h
#ifndef _APP_H
#define _APP_H

#include <inttypes.h>

#define APP_MEDIA_ERROR_MOUNT 1 // error - mounting media - f_mount failed

#define APP_FILEPRINT_NONE    0
#define APP_FILEPRINT_RUNNING 1
#define APP_FILEPRINT_PAUSED  2

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

extern void app_usbhost_reenum(void);

extern void app_media_set_inserted(uint8_t inserted);

extern uint8_t app_media_is_inserted(void);

extern void app_media_error(uint8_t media_error);

extern void app_fileprint_start(const char *filename);

extern void app_fileprint_stop(void);

extern void app_fileprint_pause(void);

extern void app_fileprint_resume(void);

extern uint8_t app_fileprint_get_state(void);

extern uint32_t app_fileprint_get_size(void);

extern uint32_t app_fileprint_get_position(void);

extern void app_fileprint_loop(void);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // _APP_H
