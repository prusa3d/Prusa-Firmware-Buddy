// media.cpp

#include "media.h"
#include "log.h"
#include "lfn.h"
#include "ff.h"
#include "usbh_core.h"
#include "../Marlin/src/gcode/queue.h"
#include <algorithm>
#include <sys/stat.h>
#include <sys/iosupport.h>
#include "marlin_server.hpp"
#include "gcode_filter.hpp"
#include "stdio.h"
#include <fcntl.h>

#ifdef REENUMERATE_USB

extern USBH_HandleTypeDef hUsbHostHS; // UsbHost handle

static const constexpr uint8_t USBHOST_REENUM_DELAY = 100;    // pool delay [ms]
static const constexpr uint16_t USBHOST_REENUM_TIMEOUT = 500; // state-hang timeout [ms]

// Re-enumerate UsbHost in case that it hangs in enumeration state (HOST_ENUMERATION,ENUM_IDLE)
// this is not solved in original UsbHost driver
// this occurs e.g. when user connects and then quickly disconnects usb flash during connection process
// state is checked every 100ms, timeout for re-enumeration is 500ms
// TODO: maybe we will change condition for states, because it can hang also in different state
static void _usbhost_reenum(void) {
    static uint32_t timer = 0;                   // static timer variable
    uint32_t tick = HAL_GetTick();               // read tick
    if ((tick - timer) > USBHOST_REENUM_DELAY) { // every 100ms
        // timer is valid, UsbHost is in enumeration state
        if ((timer) && (hUsbHostHS.gState == HOST_ENUMERATION) && (hUsbHostHS.EnumState == ENUM_IDLE)) {
            // longer than 500ms
            if ((tick - timer) > USBHOST_REENUM_TIMEOUT) {
                log_info(USBHost, "USB host reenumerating"); // trace
                USBH_ReEnumerate(&hUsbHostHS);               // re-enumerate UsbHost
            }
        } else // otherwise update timer
            timer = tick;
    }
}
#else
static void _usbhost_reenum(void) {};
#endif

extern "C" {

/// File name (Long-File-Name) of the file being printed
static char media_print_LFN[FILE_NAME_BUFFER_LEN] = { 0 };

/// Absolute path to the file being printed.
/// MUST be in Short-File-Name (DOS 8.3) notation, since
/// the transfer buffer is ~120B long (LFN paths would run out of space easily)
static char media_print_SFN_path[FILE_PATH_BUFFER_LEN] = { 0 };

char *media_print_filename() {
    return media_print_LFN;
}

char *media_print_filepath() {
    return media_print_SFN_path;
}

static media_state_t media_state = media_state_REMOVED;
static media_error_t media_error = media_error_OK;
static media_print_state_t media_print_state = media_print_state_NONE;
static FILE *media_print_file;
static uint32_t media_print_size = 0;
static uint32_t media_current_position = 0; // Current position in the file
static uint32_t media_gcode_position = 0;   // Beginning of the current G-Code
static uint32_t media_queue_position[BUFSIZE];

char getByte(GCodeFilter::State *state);
static char gcode_buffer[MAX_CMD_SIZE + 1]; // + 1 for NULL char
static GCodeFilter gcode_filter(&getByte, gcode_buffer, sizeof(gcode_buffer));
static uint32_t media_loop_read = 0;
static const constexpr uint32_t MEDIA_LOOP_MAX_READ = 4096;

media_state_t media_get_state(void) {
    return media_state;
}

void media_print_start(const char *sfnFilePath) {
    if (media_print_state != media_print_state_NONE) {
        return;
    }

    if (sfnFilePath) { // null sfnFilePath means use current filename media_print_SFN_path
        strlcpy(media_print_SFN_path, sfnFilePath, sizeof(media_print_SFN_path));
        get_LFN(media_print_LFN, sizeof(media_print_LFN), media_print_SFN_path);
    }
    struct stat info = { 0 };
    int result = stat(sfnFilePath, &info);

    if (result != 0) {
        return;
    }

    media_print_size = info.st_size;

    if ((media_print_file = fopen(sfnFilePath, "rb")) != nullptr) {
        media_gcode_position = media_current_position = 0;
        media_print_state = media_print_state_PRINTING;
    } else {
        set_warning(WarningType::USBFlashDiskError);
    }
}

inline void close_file() {
    fclose(media_print_file);
    media_print_file = nullptr;
    gcode_filter.reset();
}

void media_print_stop(void) {
    if ((media_print_state == media_print_state_PRINTING) || (media_print_state == media_print_state_PAUSED)) {
        close_file();
        media_print_state = media_print_state_NONE;
    }
}

void media_print_pause(void) {
    if (media_print_state == media_print_state_PRINTING) {
        media_print_state = media_print_state_PAUSING;
    }
}

void media_print_resume(void) {
    if (media_print_state == media_print_state_PAUSED) {
        if ((media_print_file = fopen(media_print_SFN_path, "rb")) != nullptr) {
            if (fseek(media_print_file, media_current_position, SEEK_SET) == 0)
                media_print_state = media_print_state_PRINTING;
            else {
                set_warning(WarningType::USBFlashDiskError);
                fclose(media_print_file);
                media_print_file = nullptr;
            }
        }
    }
}

media_print_state_t media_print_get_state(void) {
    return media_print_state;
}

uint32_t media_print_get_size(void) {
    return media_print_size;
}

uint32_t media_print_get_position(void) {
    return media_current_position;
}

void media_print_set_position(uint32_t pos) {
    if (pos < media_print_size) {
        media_gcode_position = media_current_position = pos;
    }
}

float media_print_get_percent_done(void) {
    if (media_print_size)
        return 100 * ((float)media_current_position / media_print_size);
    return 0;
}

char getByte(GCodeFilter::State *state) {
    char byte = '\0';

    if (media_loop_read == MEDIA_LOOP_MAX_READ) {
        // Don't read too many data at once
        *state = GCodeFilter::State::Skip;
        return byte;
    }

    UINT bytes_read = 0;
    bytes_read = fread(&byte, sizeof(byte), 1, media_print_file);

    if (bytes_read == 1) {
        *state = GCodeFilter::State::Ok;
        media_current_position++;
        media_loop_read++;
    } else if (feof(media_print_file)) {
        *state = GCodeFilter::State::Eof;
    } else {
        *state = GCodeFilter::State::Error;
    }

    return byte;
}

void media_loop(void) {
    if (media_print_state == media_print_state_PAUSING) {
        fclose(media_print_file);
        int index_r = queue.index_r;
        media_gcode_position = media_current_position = media_queue_position[index_r];
        queue.clear();
        media_print_state = media_print_state_PAUSED;
        return;
    }

    _usbhost_reenum();

    if (media_print_state != media_print_state_PRINTING) {
        return;
    }

    media_loop_read = 0;

    while (queue.length < (BUFSIZE - 1)) { // Keep one free slot for serial commands
        GCodeFilter::State state;
        char *gcode = gcode_filter.nextGcode(&state);

        if (state == GCodeFilter::State::Skip) {
            // Unlock the loop
            return;
        }
        if (state == GCodeFilter::State::Error) {
            // Pause in case of some issue
            set_warning(WarningType::USBFlashDiskError);
            media_print_pause();
            return;
        }

        if (gcode == NULL || gcode[0] == '\0') {
            if (state == GCodeFilter::State::Eof) {
                // Stop print on EOF
                media_print_stop();
                return;
            }
            // Nothing to process, continue to the next G-Code
            continue;
        }

        queue.enqueue_one(gcode, false);

        // Calculate index_w because it is private
        int index_w = queue.index_r + queue.length - 1;
        if (index_w >= BUFSIZE)
            index_w -= BUFSIZE;

        // Save current position and line
        media_queue_position[index_w] = media_gcode_position;

        if (state == GCodeFilter::State::Eof) {
            // Stop print on EOF, no need to update media_gcode_position
            media_print_stop();
            return;
        }

        // Current position can be after ';' char or after new line.  We need
        // to store the position before a semicolon. Position before a new line
        // char is also safe, therefore decrement the position.
        media_gcode_position = media_current_position - 1;
    }
}

void media_set_removed(void) {
    media_state = media_state_REMOVED;
    media_error = media_error_OK;
}

void media_set_inserted(void) {
    media_state = media_state_INSERTED;
    media_error = media_error_OK;
}

void media_set_error(media_error_t error) {
    media_state = media_state_ERROR;
    media_error = error;
}

} //extern "C"
