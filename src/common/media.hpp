// media.h
#pragma once

#include <inttypes.h>
#include <gcode/gcode_reader_restore_info.hpp>

#define PREFETCH_SIGNAL_START           1
#define PREFETCH_SIGNAL_STOP            2
#define PREFETCH_SIGNAL_FETCH           4
#define PREFETCH_SIGNAL_GCODE_INFO_INIT 8
#define PREFETCH_SIGNAL_GCODE_INFO_STOP 16
#define PREFETCH_SIGNAL_CHECK           32 ///< Re-checks that the file is still valid

/// Determines how full should the gcode queue be kept when fetching from media
/// You need at least one free slot for commands from serial (and UI)
#define MEDIA_FETCH_GCODE_QUEUE_FILL_TARGET (BUFSIZE - 1)

typedef enum {
    media_state_REMOVED = 0, // media is inserted
    media_state_INSERTED = 1, // media is removed
    media_state_ERROR = 2, // media is in error state (TODO)
} media_state_t;

typedef enum {
    media_error_OK = 0, // no error
    media_error_MOUNT = 1, // error - mounting media - f_mount failed
} media_error_t;

typedef enum {
    media_print_state_NONE = 0,
    media_print_state_PRINTING = 1,
    media_print_state_PAUSED = 2,
} media_print_state_t;

extern media_state_t media_get_state(void);

extern osThreadId prefetch_thread_id;
void media_prefetch(const void *);

/// Copies the content of sfnFilePath into marlin_vars->media_SFN_path
/// Updates marlin_vars->media_LFN as a side-effect by opening the marlin_vars->media_SFN_path and reading its LFN
extern void media_print_start__prepare(const char *sfnFilePath);

/// Start printing by issuing M23 into Marlin with a file path set by media_print_start__prepare
extern void media_print_start();

extern void media_print_stop(void);
/// \brief Pauses print.
/// \param repeat_last Repeat last command after print resume
extern void media_print_pause(bool repeat_last);
extern void media_print_resume(void);
void media_print_reopen();

/// Stop adding new commands immediately and pause the reading
/// \param pos position in the file where the print should be resumed
extern void media_print_quick_stop(uint32_t pos);

/**
 * @brief Stop adding new commands immediately and pause the reading.
 * This function is not thread safe and can only be used from powerpanic.
 */
extern void media_print_quick_stop_powerpanic();

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

extern void media_set_restore_info(const GCodeReaderStreamRestoreInfo &info);
extern GCodeReaderStreamRestoreInfo media_get_restore_info();
