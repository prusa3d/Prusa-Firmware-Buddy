// media.h
#pragma once

#include <inttypes.h>

static const uint16_t MEDIA_PRINT_FILENAME_SIZE = 128;
static const uint16_t MEDIA_PRINT_FILEPATH_SIZE = 128;

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
    media_print_state_PAUSING = 3,
} media_print_state_t;

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

/// Do not use this anywhere - the accessor functions are here just because marlin_server.cpp needs the allocated buffer.
/// @@TODO to be improved in the future
extern char *media_print_filename();
extern char *media_print_filepath();

extern media_state_t media_get_state(void);

/// Start printing by issuing M23 into Marlin with a file path.
/// Copies the content of sfnFilePath into marlin_vars->media_SFN_path (aka media_print_filepath)
/// Updates marlin_vars->media_LFN as a side-effect by opening the marlin_vars->media_SFN_path and reading its LFN
extern void media_print_start(const char *sfnFilePath);

extern void media_print_stop(void);

extern void media_print_pause(void);

extern void media_print_resume(void);

extern media_print_state_t media_print_get_state(void);

extern uint32_t media_print_get_size(void);

extern uint32_t media_print_get_position(void);

extern void media_print_set_position(uint32_t pos);

extern float media_print_get_percent_done(void);

extern void media_loop(void);

// callbacks from usb_host

extern void media_set_inserted(void);

extern void media_set_removed(void);

extern void media_set_error(media_error_t error);

/// Computes short file name (SFN) path from a (potentially) long file name (LFN)
/// path in filepath.
/// @param sfn output buffer to store the SFN path
/// @param filepath input LFN path, intentionally NOT const -
extern void media_get_SFN_path(char *sfn, uint32_t sfn_size, char *filepath);

#ifdef __cplusplus
}
#endif //__cplusplus
