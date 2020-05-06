// media.cpp

#include "media.h"
#include "dbg.h"
#include "ff.h"
#include "usbh_core.h"
#include "../Marlin/src/gcode/queue.h"

extern USBH_HandleTypeDef hUsbHostHS; // UsbHost handle

#define USBHOST_REENUM_DELAY   100 // pool delay [ms]
#define USBHOST_REENUM_TIMEOUT 500 // state-hang timeout [ms]

extern "C" {

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
                _dbg("USB host reenumerating"); // trace
                USBH_ReEnumerate(&hUsbHostHS);  // re-enumerate UsbHost
            }
        } else // otherwise update timer
            timer = tick;
    }
}

char media_print_filename[MEDIA_PRINT_FILENAME_SIZE] = { 0 };
char media_print_filepath[MEDIA_PRINT_FILEPATH_SIZE] = { 0 };

static media_state_t media_state = media_state_REMOVED;
static media_error_t media_error = media_error_OK;
static media_print_state_t media_print_state = media_print_state_NONE;
static FIL media_print_fil;
static uint32_t media_print_size = 0;
static uint32_t media_current_position = 0;
static uint32_t media_current_line = 0;
static uint32_t media_queue_position[BUFSIZE];
static uint32_t media_queue_line[BUFSIZE];

media_state_t media_get_state(void) {
    return media_state;
}

void media_get_sfn_path(char *sfn, const char *filepath) {
    uint i, j, k;
    i = j = k = 0;
    uint sl = strlen(filepath); // length of filepath
    FILINFO fi;
    char tmpPath[sl] = { 0 };
    while (i <= sl) {
        // folder || endfile found -> begin
        if (filepath[i] == '/' || i == sl) {
            // file info struct with fname & altname
            strlcpy(tmpPath, filepath, i + 1);
            FRESULT fRes = f_stat(tmpPath, &fi);
            if (fRes == FR_OK) {
                // we got folder || end file info -> process
                const char *tmpDir = fi.altname; // LFN MUST BE TURNED ON (1||2)
                _dbg(tmpDir);
                // FATFS flag for valid 8.3 fname - used instead of altname
                if (tmpDir[0] == 0 && fi.fname[0] != 0) {
                    tmpDir = fi.fname;
                }
                // save SFN part
                for (j = 0; j < 12; j++) {
                    if (tmpDir[j] == 0) {
                        break;
                    }
                    sfn[k] = tmpDir[j];
                    k++;
                }
                // add folder slash
                if (i != sl) {
                    sfn[k] = '/';
                    k++;
                }
                // SFN part of path saved
            }
        }
        i++;
    }
}

void media_print_start(const char *filepath) {
    FILINFO filinfo;
    if (media_print_state == media_print_state_NONE) {
        // get SFN path
        media_get_sfn_path(media_print_filepath, filepath);
        if (f_stat(media_print_filepath, &filinfo) == FR_OK) {
            strlcpy(media_print_filename, filinfo.fname, sizeof(media_print_filename) - 1);
            media_print_size = filinfo.fsize;
            if (f_open(&media_print_fil, media_print_filepath, FA_READ) == FR_OK) {
                media_current_position = 0;
                media_current_line = 0;
                media_print_state = media_print_state_PRINTING;
            }
        }
    }
}

void media_print_stop(void) {
    if ((media_print_state == media_print_state_PRINTING) || (media_print_state == media_print_state_PAUSED)) {
        f_close(&media_print_fil);
        queue.clear();
        media_print_state = media_print_state_NONE;
    }
}

void media_print_pause(void) {
    if (media_print_state == media_print_state_PRINTING) {
        f_close(&media_print_fil);
        int index_r = queue.index_r;
        media_current_position = media_queue_position[index_r];
        media_current_line = media_queue_line[index_r];
        queue.clear();
        media_print_state = media_print_state_PAUSED;
    }
}

void media_print_resume(void) {
    if (media_print_state == media_print_state_PAUSED) {
        if (f_open(&media_print_fil, media_print_filepath, FA_READ) == FR_OK) {
            if (f_lseek(&media_print_fil, media_current_position) == FR_OK)
                media_print_state = media_print_state_PRINTING;
            else
                f_close(&media_print_fil);
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

float media_print_get_percent_done(void) {
    if (media_print_size)
        return 100 * ((float)media_current_position / media_print_size);
    return 0;
}

void media_loop(void) {
    _usbhost_reenum();
    if (media_print_state == media_print_state_PRINTING) {
        char buffer[MAX_CMD_SIZE + 1];
        char *pch;
        if (!f_eof(&media_print_fil))              //check eof
            while (queue.length < (BUFSIZE - 1)) { //keep one free slot for serial commands
                if (f_gets(buffer, MAX_CMD_SIZE, &media_print_fil)) {
                    pch = strchr(buffer, '\r');
                    if (pch)
                        *pch = 0; //replace CR with 0
                    pch = strchr(buffer, '\n');
                    if (pch)
                        *pch = 0; //replace LF with 0
                    pch = strchr(buffer, ';');
                    if (pch)
                        *pch = 0;         //replace ; with 0 (cut comment)
                    if (strlen(buffer)) { //enqueue only not empty lines
                        queue.enqueue_one(buffer, false);
                        int index_w = queue.index_r + queue.length - 1; //calculate index_w because it is private
                        if (index_w >= BUFSIZE)
                            index_w -= BUFSIZE;                                 //..
                        media_queue_position[index_w] = media_current_position; //save current position
                        media_queue_line[index_w] = media_current_line;         //save current line
                    }
                    media_current_position = f_tell(&media_print_fil); //update current position
                    media_current_line++;                              //update current line
                } else {
                    if (f_eof(&media_print_fil)) //we need check eof also after read operation
                        media_print_stop();      //stop on eof
                    else
                        media_print_pause(); //pause in other case (read error - media removed)
                    break;
                }
            }
        else
            media_print_stop(); //stop on eof
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
