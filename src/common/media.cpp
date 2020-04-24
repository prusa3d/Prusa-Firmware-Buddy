// media.cpp

#include "media.h"
#include "dbg.h"
#include "ff.h"
#include "usbh_core.h"
#include "../Marlin/src/gcode/queue.h"

extern USBH_HandleTypeDef hUsbHostHS; // UsbHost handle

// Re-enumerate UsbHost in case that it hangs in enumeration state (HOST_ENUMERATION,ENUM_IDLE)
// this is not solved in original UsbHost driver
// this occurs e.g. when user connects and then quickly disconnects usb flash during connection process
// state is checked every 100ms, timeout for re-enumeration is 500ms
// TODO: maybe we will change condition for states, because it can hang also in different state
void _usbhost_reenum(void) {
    static uint32_t timer = 0;     // static timer variable
    uint32_t tick = HAL_GetTick(); // read tick
    if ((tick - timer) > 100) {    // every 100ms
        // timer is valid, UsbHost is in enumeration state
        if ((timer) && (hUsbHostHS.gState == HOST_ENUMERATION) && (hUsbHostHS.EnumState == ENUM_IDLE)) {
            // longer than 500ms
            if ((tick - timer) > 500) {
                _dbg("USB host reenumerating"); // trace
                USBH_ReEnumerate(&hUsbHostHS);  // re-enumerate UsbHost
            }
        } else // otherwise update timer
            timer = tick;
    }
}

uint8_t media_inserted = 0;
media_error_t media_error = media_error_OK;
media_print_state_t media_print_state = media_print_state_NONE;
char media_print_filename[256] = { 0 };
FIL media_print_fil;
uint32_t media_current_position;
uint32_t media_current_line;
uint32_t media_queue_position[BUFSIZE];
uint32_t media_queue_line[BUFSIZE];

uint8_t media_is_inserted(void) {
    return media_inserted;
}

void media_print_start(const char *filename) {
    if (media_print_state == media_print_state_NONE) {
        strncpy(media_print_filename, filename, sizeof(media_print_filename) - 1);
        if (f_open(&media_print_fil, media_print_filename, FA_READ) == FR_OK) {
            media_current_position = 0;
            media_current_line = 0;
            media_print_state = media_print_state_PRINTING;
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
#if 0
		int length = queue.length;
		int index_r = queue.index_r + length - 1;
		if (index_r >= BUFSIZE)
			index_r -= BUFSIZE;
		while (length--)
		{
			media_current_position = media_queue_position[index_r];
			media_current_line = media_queue_line[index_r];
			_dbg("dequeue %5u %s", media_current_line, queue.command_buffer[index_r]);
			if (--index_r < 0)
				index_r = BUFSIZE - 1;
		}
#else
        int index_r = queue.index_r;
        media_current_position = media_queue_position[index_r];
        media_current_line = media_queue_line[index_r];
        _dbg("dequeue %5u %s", media_current_line, queue.command_buffer[index_r]);
#endif

        queue.clear();
        media_print_state = media_print_state_PAUSED;
    }
}

void media_print_resume(void) {
    if (media_print_state == media_print_state_PAUSED) {
        if (f_open(&media_print_fil, media_print_filename, FA_READ) == FR_OK) {
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
    return f_size(&media_print_fil);
}

uint32_t media_print_get_position(void) {
    return f_tell(&media_print_fil);
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
                        _dbg("enqueue %5u %s", media_current_line, buffer);
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

void media_set_inserted(uint8_t inserted) {
    media_inserted = inserted ? 1 : 0;
}

void media_set_error(media_error_t error) {
    media_error = error;
}

extern "C" void on_process_commad(const char *cmd) {
    _dbg("process %5u %s", media_queue_line[queue.index_r], cmd);
}
