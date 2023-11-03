// media.cpp

#include <algorithm>

#include "media.h"
#include "log.h"
#include "lfn.h"
#include "ff.h"
#include "usbh_core.h"
#include "../Marlin/src/gcode/queue.h"
#include <algorithm>
#include <sys/iosupport.h>
#include "marlin_server.hpp"
#include "gcode_filter.hpp"
#include "stdio.h"
#include <fcntl.h>
#include "timing.h"
#include "metric.h"
#include "tasks.hpp"
#include <errno.h>
#include "gcode_reader.hpp"
#include "gcode_info.hpp"
#include <ccm_thread.hpp>
#include <transfers/transfer.hpp>
#include <algorithm>

using transfers::Transfer;
using State = transfers::PartialFile::State;
using std::is_same_v;

LOG_COMPONENT_REF(USBHost);
LOG_COMPONENT_REF(MarlinServer);

#ifdef REENUMERATE_USB

extern USBH_HandleTypeDef hUsbHostHS; // UsbHost handle

static const constexpr uint8_t USBHOST_REENUM_DELAY = 100; // pool delay [ms]
static const constexpr uint16_t USBHOST_REENUM_TIMEOUT = 500; // state-hang timeout [ms]

// Re-enumerate UsbHost in case that it hangs in enumeration state (HOST_ENUMERATION,ENUM_IDLE)
// this is not solved in original UsbHost driver
// this occurs e.g. when user connects and then quickly disconnects usb flash during connection process
// state is checked every 100ms, timeout for re-enumeration is 500ms
// TODO: maybe we will change condition for states, because it can hang also in different state
static void _usbhost_reenum(void) {
    static uint32_t timer = 0; // static timer variable
    uint32_t tick = HAL_GetTick(); // read tick
    if ((tick - timer) > USBHOST_REENUM_DELAY) { // every 100ms
        // timer is valid, UsbHost is in enumeration state
        if ((timer) && (hUsbHostHS.gState == HOST_ENUMERATION) && (hUsbHostHS.EnumState == ENUM_IDLE)) {
            // longer than 500ms
            if ((tick - timer) > USBHOST_REENUM_TIMEOUT) {
                log_info(USBHost, "USB host reenumerating"); // trace
                USBH_ReEnumerate(&hUsbHostHS); // re-enumerate UsbHost
            }
        } else { // otherwise update timer
            timer = tick;
        }
    }
}
#else
static void _usbhost_reenum(void) {};
#endif

extern "C" {

namespace {
    volatile media_state_t media_state = media_state_REMOVED;
    volatile media_error_t media_error = media_error_OK;

    media_print_state_t media_print_state = media_print_state_NONE;
    AnyGcodeFormatReader *media_print_file; ///< File used to print
    AnyGcodeFormatReader *gcode_info_file; ///< File used to scan GcodeInfo
    uint32_t media_print_size_estimate = 0; ///< Estimated uncompressed G-code size in bytes
    uint32_t media_current_position = 0; // Current position in the file
    uint32_t media_gcode_position = 0; // Beginning of the current G-Code
    /// Cache of PrusaPackGcodeReader that allows to resume print quickly without long searches for correct block
    PrusaPackGcodeReader::stream_restore_info_t media_stream_restore_info;

    // Position where to start after pause / quick stop
    uint32_t media_reset_position = GCodeQueue::SDPOS_INVALID;

    char getByte(GCodeFilter::State *state);
    char gcode_buffer[MAX_CMD_SIZE + 1]; // + 1 for NULL char
    GCodeFilter gcode_filter(&getByte, gcode_buffer, sizeof(gcode_buffer));
    bool skip_gcode = false;

    uint32_t usbh_error_count = 0;
    uint32_t usb_host_reset_timestamp = 0; // USB Host timestamp in seconds

    metric_t usbh_error_cnt = METRIC("usbh_err_cnt", METRIC_VALUE_INTEGER, 1000, METRIC_HANDLER_ENABLE_ALL);

    // These buffers are HUGE. We need to rework the prefetcher logic
    // to be more efficient and add compression.
    constexpr size_t FILE_BUFF_SIZE = 5120;
    char __attribute__((section(".ccmram"))) prefetch_buff[2][FILE_BUFF_SIZE];
    char *file_buff;
    uint32_t file_buff_level;
    size_t back_buff_level = 0;
    uint32_t file_buff_pos;
    GCodeFilter::State prefetch_state;

    SemaphoreHandle_t prefetch_mutex_data_out = nullptr; ///< Mutex to switch buffers
    SemaphoreHandle_t prefetch_mutex_file_reader = nullptr; ///< Mutex to not close while another thread is using it

    metric_t metric_prefetched_bytes = METRIC("media_prefetched", METRIC_VALUE_INTEGER, 1000, METRIC_HANDLER_ENABLE_ALL);

    /**
     * @brief Initialize GCodeInfo.
     * @param event signal that started this, will be updated while waiting for file to be downloaded
     * @param event nullptr if not waiting for file to be downloaded
     */
    void media_gcode_info_scan(osEvent *event = nullptr) {
        assert(gcode_info_file);
        auto &gcode_info = GCodeInfo::getInstance();

        if (!gcode_info.start_load(*gcode_info_file)) {
            log_error(MarlinServer, "Media prefetch GCodeInfo: fail to open");
            return;
        }

        const bool should_load_gcode = [&] {
            // Wait for gcode to be valid
            while (!gcode_info.check_valid_for_print(*gcode_info_file)) {
                if (gcode_info.has_error()) {
                    log_error(MarlinServer, "Media prefetch GCodeInfo: not valid: %s", gcode_info.error_str());
                    return false;
                }

                if (!event) {
                    // Do not wait for file to download
                    log_error(MarlinServer, "Media prefetch GCodeInfo: cannot wait");
                    return false;
                } else {
                    // Check for signal to stop loading (for example Quit button during the Downloading screen)
                    *event = osSignalWait(PREFETCH_SIGNAL_GCODE_INFO_STOP, 500);
                    if (event->value.signals & PREFETCH_SIGNAL_GCODE_INFO_STOP) {
                        log_info(MarlinServer, "Media prefetch GCodeInfo: stopped");
                        return false;
                    }
                }
            }

            // Verify the file CRC
            if (!gcode_info.verify_file(*gcode_info_file)) {
                log_error(MarlinServer, "Media prefetch GCodeInfo: fail to verify: %s", gcode_info.error_str());
                return false;
            }

            return true;
        }();

        if (should_load_gcode) {
            log_info(MarlinServer, "Media prefetch GCodeInfo: loading");
            gcode_info.load(*gcode_info_file);
        }

        gcode_info.end_load(*gcode_info_file);
    }
} // namespace

media_state_t media_get_state(void) {
    return media_state;
}

void media_prefetch_init() {
    // Allocate these and never free
    // This is done after bootstrap when a lot of heap is freed

    prefetch_mutex_data_out = xSemaphoreCreateMutex();
    assert(prefetch_mutex_data_out);

    prefetch_mutex_file_reader = xSemaphoreCreateMutex();
    assert(prefetch_mutex_file_reader);

    media_print_file = new AnyGcodeFormatReader();
    assert(media_print_file);

    gcode_info_file = new AnyGcodeFormatReader();
    assert(gcode_info_file);
}

void media_prefetch(const void *) {
    assert(prefetch_mutex_data_out && prefetch_mutex_file_reader && media_print_file && gcode_info_file);

    metric_register(&metric_prefetched_bytes);

    TaskDeps::provide(TaskDeps::Dependency::media_prefetch_ready);
    for (;;) {
        char *back_buff = prefetch_buff[0];
        GCodeFilter::State bb_state = GCodeFilter::State::Ok;

        file_buff = prefetch_buff[1];
        prefetch_state = GCodeFilter::State::Ok;
        osEvent event;

        file_buff_level = file_buff_pos = 0;

        event = osSignalWait(PREFETCH_SIGNAL_START | PREFETCH_SIGNAL_STOP | PREFETCH_SIGNAL_FETCH | PREFETCH_SIGNAL_GCODE_INFO_INIT | PREFETCH_SIGNAL_GCODE_INFO_STOP | PREFETCH_SIGNAL_CHECK, osWaitForever);

        if (event.value.signals & PREFETCH_SIGNAL_GCODE_INFO_INIT) {
            media_gcode_info_scan(&event);
        }

        if (event.value.signals & PREFETCH_SIGNAL_CHECK) {
            GCodeInfo::getInstance().check_still_valid();
        }

        if ((event.value.signals & PREFETCH_SIGNAL_START) == 0) {
            continue;
        }

        log_info(MarlinServer, "Media prefetch: started");

        back_buff_level = FILE_BUFF_SIZE;

        IGcodeReader::Result_t first_read_res;
        do {
            log_info(MarlinServer, "Media prefetch: Prefetching first %u bytes at offset %u", FILE_BUFF_SIZE, media_current_position);
            xSemaphoreTake(prefetch_mutex_file_reader, portMAX_DELAY);
            if (media_print_file->is_open()) {
                back_buff_level = FILE_BUFF_SIZE;
                first_read_res = media_print_file->get()->stream_get_block(back_buff, back_buff_level);
            } else {
                first_read_res = IGcodeReader::Result_t::RESULT_ERROR;
            }
            xSemaphoreGive(prefetch_mutex_file_reader);
        } while (first_read_res == IGcodeReader::Result_t::RESULT_TIMEOUT);

        if ((first_read_res == IGcodeReader::Result_t::RESULT_OK || first_read_res == IGcodeReader::Result_t::RESULT_EOF) && back_buff_level > 0) { // read anything, or EOF happened
            bb_state = GCodeFilter::State::Ok;
        } else if (first_read_res == IGcodeReader::Result_t::RESULT_OUT_OF_RANGE) {
            bb_state = GCodeFilter::State::NotDownloaded;
            log_warning(MarlinServer, "Media prefetch: data not yet downloaded");
            osSignalWait(PREFETCH_SIGNAL_STOP, osWaitForever);
            continue;
        } else {
            prefetch_state = GCodeFilter::State::Error;
            log_info(MarlinServer, "Media prefetch: stopped by error");
            osSignalWait(PREFETCH_SIGNAL_STOP, osWaitForever);
            continue;
        }

        for (;;) {
            bool rerun_loop = false; // by default, loop will run once and wait for signal

            // swap back and front buffer, if its possible
            xSemaphoreTake(prefetch_mutex_data_out, portMAX_DELAY);
            if (file_buff_pos == file_buff_level) { // file buffer depleted
                prefetch_state = bb_state;
                if (back_buff_level > 0 && bb_state != GCodeFilter::State::Timeout && bb_state != GCodeFilter::State::Error) { // swap to back buffer
                    if (file_buff == prefetch_buff[0]) {
                        file_buff = prefetch_buff[1];
                        back_buff = prefetch_buff[0];
                    } else {
                        file_buff = prefetch_buff[0];
                        back_buff = prefetch_buff[1];
                    }
                    file_buff_level = back_buff_level;
                    file_buff_pos = 0;
                    back_buff_level = 0;
                }
            }
            xSemaphoreGive(prefetch_mutex_data_out);

            const bool need_fetch = back_buff_level == 0 && (bb_state != GCodeFilter::State::Eof && bb_state != GCodeFilter::State::Error && bb_state != GCodeFilter::State::NotDownloaded);
            if (need_fetch) {
                // We don't want other threads holding FS/media locks to inherit high priority
                osThreadSetPriority(osThreadGetId(), TASK_PRIORITY_MEDIA_PREFETCH_WHILE_FREAD);
                log_info(USBHost, "Media prefetch start read");
                back_buff_level = FILE_BUFF_SIZE;
                auto second_read_res = IGcodeReader::Result_t::RESULT_ERROR;

                xSemaphoreTake(prefetch_mutex_file_reader, portMAX_DELAY);
                if (media_print_file->is_open()) {
                    second_read_res = media_print_file->get()->stream_get_block(back_buff, back_buff_level);
                } else {
                    second_read_res = IGcodeReader::Result_t::RESULT_ERROR;
                }

                if (second_read_res == IGcodeReader::Result_t::RESULT_OUT_OF_RANGE) {
                    // The reader thinks it is outside of the already
                    // downloaded range. But we haven't updated our knowledge
                    // about what's downloaded in a while, so update it now and
                    // retry. If it still fails even after update, deal with it below.
                    transfers::Transfer::Path path;
                    marlin_vars()->media_SFN_path.execute_with([&](const char *value) {
                        path = transfers::Transfer::Path(value);
                    });

                    media_print_file->get()->update_validity(path);
                    back_buff_level = FILE_BUFF_SIZE;
                    second_read_res = media_print_file->get()->stream_get_block(back_buff, back_buff_level);
                }
                xSemaphoreGive(prefetch_mutex_file_reader);
                log_info(USBHost, "Media prefetch read done");
                osThreadSetPriority(osThreadGetId(), TASK_PRIORITY_MEDIA_PREFETCH);

                if (second_read_res == IGcodeReader::Result_t::RESULT_OK && back_buff_level > 0) {
                    // read ok
                    bb_state = GCodeFilter::State::Ok;
                } else if (second_read_res == IGcodeReader::Result_t::RESULT_OUT_OF_RANGE) {
                    bb_state = GCodeFilter::State::NotDownloaded;
                    log_warning(MarlinServer, "Media prefetch: data not yet downloaded");
                } else if (second_read_res == IGcodeReader::Result_t::RESULT_EOF) {
                    bb_state = GCodeFilter::State::Eof;
                    log_warning(MarlinServer, "Media prefetch: EOF");
                } else if (second_read_res == IGcodeReader::Result_t::RESULT_TIMEOUT) {
                    bb_state = GCodeFilter::State::Timeout;
                    rerun_loop = true;
                    log_warning(MarlinServer, "Media prefetch: timeout");
                } else {
                    bb_state = GCodeFilter::State::Error;
                    log_error(MarlinServer, "Media prefetch: error");
                }
            }

            if (!rerun_loop) {
                event = osSignalWait(PREFETCH_SIGNAL_FETCH | PREFETCH_SIGNAL_STOP, osWaitForever);
                if (event.value.signals & PREFETCH_SIGNAL_GCODE_INFO_INIT) {
                    media_gcode_info_scan();
                }
                if (event.value.signals & PREFETCH_SIGNAL_STOP) {
                    log_info(MarlinServer, "Media prefetch got STOP signal");
                    break;
                }
            }
        }
    }
}

void media_print_start__prepare(const char *sfnFilePath) {
    if (sfnFilePath) {
        auto lock = MarlinVarsLockGuard();
        // update media_SFN_path
        strlcpy(marlin_vars()->media_SFN_path.get_modifiable_ptr(lock), sfnFilePath, marlin_vars()->media_SFN_path.max_length());

        // set media_LFN
        get_LFN(marlin_vars()->media_LFN.get_modifiable_ptr(lock), marlin_vars()->media_LFN.max_length(), marlin_vars()->media_SFN_path.get_modifiable_ptr(lock));
    }
}

void media_print_start() {
    assert(prefetch_mutex_file_reader && media_print_file);

    if (media_print_state != media_print_state_NONE) {
        return;
    }

    xSemaphoreTake(prefetch_mutex_file_reader, portMAX_DELAY);
    media_print_file->open(marlin_vars()->media_SFN_path.get_ptr());
    if (media_print_file->is_open() && media_print_file->get()->stream_gcode_start()) {
        media_gcode_position = media_current_position = 0;
        media_print_state = media_print_state_PRINTING;
        media_print_size_estimate = media_print_file->get()->get_gcode_stream_size_estimate();
        gcode_filter.reset();
        osSignalSet(prefetch_thread_id, PREFETCH_SIGNAL_START);
    } else {
        marlin_server::set_warning(WarningType::USBFlashDiskError);
    }
    xSemaphoreGive(prefetch_mutex_file_reader);
}

inline void close_file() {
    assert(prefetch_mutex_file_reader && media_print_file);

    xSemaphoreTake(prefetch_mutex_file_reader, portMAX_DELAY);
    osSignalSet(prefetch_thread_id, PREFETCH_SIGNAL_STOP);
    media_print_file->close();
    xSemaphoreGive(prefetch_mutex_file_reader);
}

void media_print_stop(void) {
    if ((media_print_state == media_print_state_PRINTING) || (media_print_state == media_print_state_PAUSED)) {
        close_file();
        media_print_state = media_print_state_NONE;
        queue.sdpos = GCodeQueue::SDPOS_INVALID;
    }
}

void media_print_quick_stop(uint32_t pos) {
    assert(prefetch_mutex_file_reader && media_print_file);

    skip_gcode = false;
    media_print_state = media_print_state_PAUSED;
    media_reset_position = pos;
    queue.clear();

    xSemaphoreTake(prefetch_mutex_file_reader, portMAX_DELAY);
    if (auto pack = media_print_file->get_prusa_pack(); pack != nullptr) {
        media_stream_restore_info = pack->get_restore_info();
    }
    xSemaphoreGive(prefetch_mutex_file_reader);
}

void media_print_quick_stop_powerpanic() {
    assert(media_print_file);

    skip_gcode = false;
    media_print_state = media_print_state_PAUSED;
    media_reset_position = GCodeQueue::SDPOS_INVALID;
    queue.clear();

    // These two need to happen at once, from high priority ISR
    if (auto pack = media_print_file->get_prusa_pack(); pack != nullptr) {
        media_stream_restore_info = pack->get_restore_info();
    }
}

void media_print_pause(bool repeat_last = false) {
    if (media_print_state != media_print_state_PRINTING) {
        return;
    }

    media_print_quick_stop(queue.get_current_sdpos());
    close_file();

    // when pausing the current instruction is fully processed, skip it on resume
    skip_gcode = !repeat_last;
}

void media_print_resume(void) {
    assert(prefetch_mutex_file_reader && media_print_file);

    if ((media_print_state != media_print_state_PAUSED)) {
        return;
    }

    xSemaphoreTake(prefetch_mutex_file_reader, portMAX_DELAY);
    if (!media_print_file->is_open()) {
        // file was closed by media_print_pause, reopen
        media_print_file->open(marlin_vars()->media_SFN_path.get_ptr());
    }
    if (media_print_file->is_open()) {
        media_print_size_estimate = media_print_file->get()->get_gcode_stream_size_estimate();
        if (media_reset_position != GCodeQueue::SDPOS_INVALID) {
            media_print_set_position(media_reset_position);
            media_reset_position = GCodeQueue::SDPOS_INVALID;
        }
        if (media_print_file->get_prusa_pack()) {
            media_print_file->get_prusa_pack()->set_restore_info(media_get_restore_info());
        }
        // file was left open between pause/resume or re-opened successfully
        if (media_print_file->get()->stream_gcode_start(media_current_position)) {
            gcode_filter.reset();
            media_print_state = media_print_state_PRINTING;
            osSignalSet(prefetch_thread_id, PREFETCH_SIGNAL_START);
        } else {
            marlin_server::set_warning(WarningType::USBFlashDiskError);
            close_file();
        }
    } else {
        marlin_server::set_warning(WarningType::USBFlashDiskError);
    }
    xSemaphoreGive(prefetch_mutex_file_reader);
}

media_print_state_t media_print_get_state(void) {
    return media_print_state;
}

uint32_t media_print_get_size(void) {
    return media_print_size_estimate;
}

uint32_t media_print_get_position(void) {
    return media_current_position;
}

void media_print_set_position(uint32_t pos) {
    media_gcode_position = media_current_position = pos;
}

uint32_t media_print_get_pause_position(void) {
    return media_reset_position;
}

float media_print_get_percent_done(void) {
    if (media_print_size_estimate == 0) {
        return 100;
    }

    return std::min(99.0f, 100 * ((float)media_current_position / media_print_size_estimate));
}

char getByte(GCodeFilter::State *state) {
    char byte;
    uint32_t level;

    xSemaphoreTake(prefetch_mutex_data_out, portMAX_DELAY);
    if (file_buff_level - file_buff_pos > 0) {
        *state = GCodeFilter::State::Ok;
        media_current_position++;
        byte = file_buff[file_buff_pos++];
        level = file_buff_level - file_buff_pos;
        xSemaphoreGive(prefetch_mutex_data_out);
        if (level == 0) {
            osSignalSet(prefetch_thread_id, PREFETCH_SIGNAL_FETCH);
        }
        return byte;
    }

    if (prefetch_state == GCodeFilter::State::Ok) {
        // Ok makes no sense if we didn't get a byte, ground it to some safer state.
        *state = GCodeFilter::State::Timeout;
    } else {
        *state = prefetch_state;
    }
    xSemaphoreGive(prefetch_mutex_data_out);
    return '\0';
}

static size_t media_get_bytes_prefetched() {
    return (file_buff_level - file_buff_pos) + back_buff_level;
}

void media_loop(void) {
    assert(media_print_file);

    _usbhost_reenum();

    if (media_print_state != media_print_state_PRINTING) {
        if (media_print_file->get() != nullptr) { // Read pointer without mutex lock, should be safe
            // complete closing the file in the main loop (for media_print_quick_stop)
            close_file();
        }

        return;
    }

    while (queue.length < (BUFSIZE - 1)) { // Keep one free slot for serial commands
        GCodeFilter::State state;
        char *gcode = gcode_filter.nextGcode(&state);

        switch (state) {
        case GCodeFilter::State::NotDownloaded:
            // TODO: We want a specialized pause screen with error message and help link.
            // TODO: We want to auto-unpause if more data arrive.
            marlin_server::set_warning(WarningType::NotDownloaded);
            media_print_pause();
            return;
        case GCodeFilter::State::Timeout:
            // Unlock the loop
            return;
        case GCodeFilter::State::Error:
            // Pause in case of some issue
            usbh_error_count++;
            metric_record_integer(&usbh_error_cnt, usbh_error_count);
            marlin_server::set_warning(WarningType::USBFlashDiskError);
            media_print_pause();
            return;
        case GCodeFilter::State::Eof:
            // Stop print on EOF
            // TODO: this is incorrect. We need to wait until the queue is drained before we can stop
            media_print_stop();
            return;
        case GCodeFilter::State::Ok:
            if (gcode == NULL || gcode[0] == '\0') {
                // Nothing to process, continue to the next G-Code
                break;
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
            break;
        }
    }

    if (media_print_state == media_print_state_PRINTING && metric_record_is_due(&metric_prefetched_bytes)) {
        metric_record_integer(&metric_prefetched_bytes, media_get_bytes_prefetched());
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

void media_set_restore_info(PrusaPackGcodeReader::stream_restore_info_t &info) {
    media_stream_restore_info = info;
}

PrusaPackGcodeReader::stream_restore_info_t media_get_restore_info() {
    return media_stream_restore_info;
}

} // extern "C"
