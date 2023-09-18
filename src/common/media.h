// media.h
#pragma once

#include <inttypes.h>
#include <gcode/gcode_reader.hpp> // for PrusaPackGcodeReader::stream_restore_info_t

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

extern media_state_t media_get_state(void);

/// Copies the content of sfnFilePath into marlin_vars->media_SFN_path
/// Updates marlin_vars->media_LFN as a side-effect by opening the marlin_vars->media_SFN_path and reading its LFN
extern void media_print_start__prepare(const char *sfnFilePath);

/// Start printing by issuing M23 into Marlin with a file path set by media_print_start__prepare
extern void media_print_start(const bool prefetch_start);

extern void media_print_stop(void);
/// \brief Pauses print.
/// \param repeat_last Repeat last command after print resume
extern void media_print_pause(bool repeat_last);
extern void media_print_resume(void);

/// Stop adding new commands immediately and pause the reading
/// \param pos position in the file where the print should be resumed
/// media_print_quick_stop is safe to use within an ISR
extern void media_print_quick_stop(uint32_t pos);

extern media_print_state_t media_print_get_state(void);

extern uint32_t media_print_get_size(void);

extern uint32_t media_print_get_position(void);

extern void media_print_set_position(uint32_t pos);

extern uint32_t media_print_get_pause_position(void);

extern float media_print_get_percent_done(void);

extern void media_loop(void);

// callbacks from usb_host
extern void media_set_inserted(void);
extern void media_set_removed(void);
extern void media_set_error(media_error_t error);

extern void media_reset_usbh_error();

extern void media_set_restore_info(PrusaPackGcodeReader::stream_restore_info_t &info);
extern PrusaPackGcodeReader::stream_restore_info_t media_get_restore_info();

#ifdef __cplusplus
}
#endif //__cplusplus
