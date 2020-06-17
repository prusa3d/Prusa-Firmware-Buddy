// media.cpp

#include "media.h"
#include "dbg.h"
#include "ff.h"
#include "usbh_core.h"
#include "../Marlin/src/gcode/queue.h"
#include <algorithm>

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

/// File name (Long-File-Name) of the file being printed
static char media_print_LFN[MEDIA_PRINT_FILENAME_SIZE] = { 0 };

/// Absolute path to the file being printed.
/// MUST be in Short-File-Name (DOS 8.3) notation, since
/// the transfer buffer is ~120B long (LFN paths would run out of space easily)
static char media_print_SFN_path[MEDIA_PRINT_FILEPATH_SIZE] = { 0 };

char *media_print_filename() {
    return media_print_LFN;
}

char *media_print_filepath() {
    return media_print_SFN_path;
}

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

void media_get_SFN_path(char *sfn, uint32_t sfn_size, char *filepath) {
    *sfn = 0; // init the output buffer
    if (!*filepath)
        return; // empty path received -> don't bother

    // skip initial '/'
    while (*filepath == '/')
        ++filepath;

    // first walk over filepath and replace all '/' with \x00
    // that will allow for incremental traversal through the path without the need for additional buffer
    // the '/' will be replaced back, so in fact, filepath will not be changed, but must be in RAM!
    // Moreover, fpend will get the address of the end of filepath with this cycle
    char *fpend = filepath;
    for (; *fpend; ++fpend) {
        if (*fpend == '/')
            *fpend = 0;
    }
    char *tmpEnd = filepath + strlen(filepath); // up until the first \x00

    // normally, I'd do this by hijacking FATfs's follow_path(), which in fact does the same
    // but it is not in FATfs's public interface ...
    while (filepath < fpend) {
        *sfn = '/'; // prepare an output slash
        FILINFO fi;
        // LFN MUST BE TURNED ON (1||2)
        // unfortunately, this does follow_path internally all over again
        if (f_stat(filepath, &fi) == FR_OK) {
            // we got folder || end file info -> process
            // FATFS flag for valid 8.3 fname - used instead of altname
            const char *fname = (fi.altname[0] == 0 && fi.fname[0] != 0) ? fi.fname : fi.altname;
            _dbg(fname);
            uint32_t chars_added = strlcpy(sfn, fname, std::min(sfn_size, uint32_t(12)));
            sfn += chars_added;
            sfn_size -= chars_added;
        }
        *tmpEnd = '/';                    // return the '/' back into filepath
        tmpEnd = tmpEnd + strlen(tmpEnd); // iterate to the next \x00
    }
}

/// This is a workaround for a nasty edge case of FATfs's f_stat,
/// which doesn't return a LFN when given a full SFN path.
/// @@TODO may be an updated FATfs solves this
/// The complexity of this code is comparable to original f_stat, may be a bit faster
/// It just opens a directory, finds the right SFN and fills the fno structure with the LFN, which is what we want.
/// Refactored from LazyDirView::F_DIR_RAII_Find_One
class f_stat_LFN {
public:
    /// beware, this assumes a full path which includes at least one slash.
    /// Also, the sfnPath must not be a const ptr, since we are abusing the complete path
    /// to break it IN PLACE into the separate path and the separate filename.
    /// This is fixed at the end of the function, so the sfnPath doesn't change from the outside perspective.
    f_stat_LFN(char *sfnPath) {
        char *sfn = strrchr(sfnPath, '/');
        char *returnSlash = sfn;
        *returnSlash = 0; //
        ++sfn;
        result = FR_NO_FILE;
        if ((result = f_opendir(&dp, sfnPath)) == FR_OK) {
            for (;;) {
                result = f_readdir(&dp, &fno); // get a directory item
                if (result != FR_OK || !fno.fname[0]) {
                    result = FR_NO_FILE; // make sure we report some meaningful error (unlike FATfs)
                    break;
                }
                // select appropriate file name
                const char *fname = fno.altname[0] ? fno.altname : fno.fname;
                if (!strcmp(sfn, fname)) {
                    break; // found the SFN searched for
                }
            }
        }
        *returnSlash = '/';
    }
    ~f_stat_LFN() {
        f_closedir(&dp);
    }
    /// @returns true if the search for the filename was successfull
    inline bool Success() const { return result == FR_OK; }
    /// @returns the file size of the searched for. It is only valid if the search was successful
    inline unsigned int FSize() const { return fno.fsize; }
    /// @returns pointer to the LFN of the file searched for.
    /// It is only valid if the search was successful and while the f_stat_LFN exists.
    inline const char *LFName() const { return fno.fname; }

private:
    DIR dp;
    FILINFO fno;
    int result;
};

void media_print_start(const char *sfnFilePath) {
    if (media_print_state == media_print_state_NONE) {
        strlcpy(media_print_SFN_path, sfnFilePath, sizeof(media_print_SFN_path));
        // Beware - f_stat returns a SFN filename, when the input path is SFN
        // which is a nasty surprise. Therefore there is an alternative way of looking
        // for the file, which has the same results (and a bit lower code complexity)
        // An updated version of FATfs may solve the problem, therefore the original line of code is left here as a comment
        // if (f_stat(media_print_SFN_path, &filinfo) == FR_OK) {
        f_stat_LFN fo(media_print_SFN_path);
        if (fo.Success()) {
            strlcpy(media_print_LFN, fo.LFName(), sizeof(media_print_LFN));
            media_print_size = fo.FSize(); //filinfo.fsize;
            if (f_open(&media_print_fil, media_print_SFN_path, FA_READ) == FR_OK) {
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
        if (f_open(&media_print_fil, media_print_SFN_path, FA_READ) == FR_OK) {
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
