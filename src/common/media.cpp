// media.cpp

#include <algorithm>

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
#include "timing.h"
#include "metric.h"

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

static volatile media_state_t media_state = media_state_REMOVED;
static volatile media_error_t media_error = media_error_OK;

static media_print_state_t media_print_state = media_print_state_NONE;
static FILE *media_print_file = nullptr;
static uint32_t media_print_size = 0;
static uint32_t media_current_position = 0; // Current position in the file
static uint32_t media_gcode_position = 0;   // Beginning of the current G-Code
// Position where to start after pause / quick stop
static uint32_t media_reset_position = MEDIA_PRINT_UNDEF_POSITION;

char getByte(GCodeFilter::State *state);
static char gcode_buffer[MAX_CMD_SIZE + 1]; // + 1 for NULL char
static GCodeFilter gcode_filter(&getByte, gcode_buffer, sizeof(gcode_buffer));
static uint32_t media_loop_read = 0;
static const constexpr uint32_t MEDIA_LOOP_MAX_READ = 4096;
static bool skip_gcode = false;

static uint32_t usbh_error_count = 0;
uint32_t usb_host_reset_timestamp = 0; // USB Host timestamp in seconds

static uint32_t usb_host_power_cycle_delay = 1; // USB Host pulse delay in seconds

typedef enum {
    USB_host_recovery_start = 0,
    USB_host_recovery_end = 1,
} USB_host_recovery_state_t;

static USB_host_recovery_state_t USB_host_recovery_state = USB_host_recovery_state_t::USB_host_recovery_start;

static metric_t usbh_error_cnt = METRIC("usbh_err_cnt", METRIC_VALUE_INTEGER, 1000, METRIC_HANDLER_ENABLE_ALL);

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
        gcode_filter.reset();
    } else {
        set_warning(WarningType::USBFlashDiskError);
    }
}

inline void close_file() {
    fclose(media_print_file);
    media_print_file = nullptr;
}

void media_print_stop(void) {
    if ((media_print_state == media_print_state_PRINTING) || (media_print_state == media_print_state_PAUSED)) {
        close_file();
        media_print_state = media_print_state_NONE;
        queue.sdpos = MEDIA_PRINT_UNDEF_POSITION;
    }
}

void media_print_quick_stop(uint32_t pos) {
    skip_gcode = false;
    media_print_state = media_print_state_PAUSED;
    media_reset_position = pos;
    queue.clear();
}

void media_print_pause(bool repeat_last = false) {
    if (media_print_state != media_print_state_PRINTING)
        return;

    media_print_quick_stop(queue.get_current_sdpos());
    close_file();

    // when pausing the current instruction is fully processed, skip it on resume
    skip_gcode = !repeat_last;
}

void media_print_resume(void) {
    if ((media_print_state != media_print_state_PAUSED) && (media_print_state != media_print_state_DRAINING))
        return;

    if (media_reset_position != MEDIA_PRINT_UNDEF_POSITION) {
        media_print_set_position(media_reset_position);
        media_reset_position = MEDIA_PRINT_UNDEF_POSITION;
    }

    if (media_print_state == media_print_state_PAUSED || media_print_state == media_print_state_DRAINING) {
        if (!media_print_file) {
            // file was closed by media_print_pause, reopen
            media_print_file = fopen(media_print_SFN_path, "rb");
        }
        if (media_print_file != nullptr) {
            // file was left open between pause/resume or re-opened successfully
            if (fseek(media_print_file, media_current_position, SEEK_SET) == 0) {
                gcode_filter.reset();
                media_print_state = media_print_state_PRINTING;
            } else {
                set_warning(WarningType::USBFlashDiskError);
                close_file();
            }
        }
    }
}

void media_print_drain() {
    media_reset_position = queue.get_current_sdpos();
    media_print_state = media_print_state_DRAINING;
    close_file();
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

uint32_t media_print_get_pause_position(void) {
    return media_reset_position;
}

float media_print_get_percent_done(void) {
    if (media_print_size == 0)
        return 100;

    return 100 * ((float)media_current_position / media_print_size);
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
    _usbhost_reenum();

    if (media_print_state != media_print_state_PRINTING) {
        if (media_print_file) {
            // complete closing the file in the main loop (for media_print_quick_stop)
            close_file();
        }
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
            usbh_error_count++;
            if (media_state == media_state_INSERTED) {
                metric_record_integer(&usbh_error_cnt, usbh_error_count);
                media_print_drain();
            } else {
                set_warning(WarningType::USBFlashDiskError);
                media_print_pause();
            }

            return;
        }

        if (gcode == NULL || gcode[0] == '\0') {
            if (state == GCodeFilter::State::Eof) {
                // Stop print on EOF
                // TODO: this is incorrect. We need to wait until the queue is drained before we can stop
                media_print_stop();
                return;
            }
            // Nothing to process, continue to the next G-Code
            continue;
        }

        if (media_print_state == media_print_state_PAUSED
            || media_print_state == media_print_state_NONE) {
            // Exit from the loop if aborted early
            // TODO: this is incorrect. We need to wait until the queue is drained before we can stop
            return;
        }

        if (skip_gcode) {
            skip_gcode = false;
        } else {
            // update the gcode position for the queue
            queue.sdpos = media_gcode_position;
            // FIXME: what if the gcode is not enqueued
            // use 'enqueue_one_now' instead
            queue.enqueue_one(gcode, false);
        }

        // Current position can be after ';' char or after new line.  We need
        // to store the position before a semicolon. Position before a new line
        // char is also safe, therefore decrement the position.
        media_gcode_position = media_current_position - 1;
    }
}

// callback from usb_host
void media_set_removed(void) {
    media_state = media_state_REMOVED;
    media_error = media_error_OK;
}

// callback from usb_host
void media_set_inserted(void) {
    media_state = media_state_INSERTED;
    media_error = media_error_OK;
}

// callback from usb_host
void media_set_error(media_error_t error) {
    media_error = error;
    media_state = media_state_ERROR;
}

void media_reset_usbh_error() {
    usbh_error_count = 0;
}

void media_reset_USB_host() {

    switch (USB_host_recovery_state) {
    case USB_host_recovery_state_t::USB_host_recovery_start:
        log_error(USBHost, "Start recovering from USB Host error");
        buddy::hw::hsUSBEnable.write(buddy::hw::Pin::State::high); // power off USB Host
        usb_host_reset_timestamp = ticks_s();
        USB_host_recovery_state = USB_host_recovery_state_t::USB_host_recovery_end;
        break;
    case USB_host_recovery_state_t::USB_host_recovery_end:
        if (ticks_diff(ticks_s(), usb_host_reset_timestamp) > (int32_t)usb_host_power_cycle_delay) {
            buddy::hw::hsUSBEnable.write(buddy::hw::Pin::State::low); //power on USB Host
            if (media_get_state() == media_state_t::media_state_INSERTED) {
                media_print_resume();
                log_error(USBHost, "Recovery from USB Host error is done");
                USB_host_recovery_state = USB_host_recovery_state_t::USB_host_recovery_start;
            }
        }
        break;
    }
}

} //extern "C"
