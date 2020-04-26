// media.h
#ifndef _MEDIA_H
#define _MEDIA_H

#include <inttypes.h>

typedef enum {
    media_error_OK = 0,    // no error
    media_error_MOUNT = 1, // error - mounting media - f_mount failed
} media_error_t;

typedef enum {
    media_print_state_NONE = 0,
    media_print_state_PRINTING = 1,
    media_print_state_PAUSED = 2,
} media_print_state_t;

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern char media_print_filename[128];

extern char media_print_filepath[128];

extern uint8_t media_is_inserted(void);

extern void media_print_start(const char *filepath);

extern void media_print_stop(void);

extern void media_print_pause(void);

extern void media_print_resume(void);

extern media_print_state_t media_print_get_state(void);

extern uint32_t media_print_get_size(void);

extern uint32_t media_print_get_position(void);

extern void media_loop(void);

extern void media_set_inserted(uint8_t inserted);

extern void media_set_error(media_error_t error);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_MEDIA_H
