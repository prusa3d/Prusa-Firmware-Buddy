// media.h
#ifndef _MEDIA_H
#define _MEDIA_H

#include <inttypes.h>

#define MEDIA_PRINT_FILENAME_SIZE 128
#define MEDIA_PRINT_FILEPATH_SIZE 128

typedef enum {
    media_state_REMOVED = 0,  // media is inserted
    media_state_INSERTED = 1, // media is removed
    media_state_ERROR = 2,    // media is in error state (TODO)
} media_state_t;

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

extern char media_print_filename[MEDIA_PRINT_FILENAME_SIZE];

extern char media_print_filepath[MEDIA_PRINT_FILEPATH_SIZE];

extern media_state_t media_get_state(void);

extern void media_print_start(const char *filepath);

extern void media_print_stop(void);

extern void media_print_pause(void);

extern void media_print_resume(void);

extern media_print_state_t media_print_get_state(void);

extern uint32_t media_print_get_size(void);

extern uint32_t media_print_get_position(void);

extern float media_print_get_percent_done(void);

extern void media_loop(void);

// callbacks from usb_host

extern void media_set_inserted(void);

extern void media_set_removed(void);

extern void media_set_error(media_error_t error);

// extern void media_get_sfn_path(char *sfn, const char *filepath, char *aname);
extern void media_get_sfn_path(char *sfn, const char *filepath);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_MEDIA_H
