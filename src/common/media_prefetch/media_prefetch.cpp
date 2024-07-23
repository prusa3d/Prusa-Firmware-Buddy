#include "media_prefetch.hpp"

#include <cstring>
#include <cinttypes>

#include <logging/log.hpp>
#include <enum_array.hpp>

#include "prefetch_compression.hpp"

#ifndef UNITTESTS
    #include <metric.h>
    #include <scope_guard.hpp>
    #include <cmsis_os.h>
#endif

namespace media_prefetch {

enum class RecordType : uint8_t {
    /// RecordType + uint8_t (strlen) + non-null-terminated string
    plain_gcode,

    /// RecordType + uint8_t (strlen) + non-null terminated
    /// Compression is done through compress_gcode and decompress_gcode
    compressed_gcode,

    /// GCodeReaderStreamRestoreInfo follows
    /// Emitted in to the buffer whenever stream restore info changes.
    restore_info_update,

    /// uint32_t offset follows
    offset_update,

    /// uint8_t offset increase follows
    incremental_offset_update,

    /// Marks that the following command did not fit in the buffers and was cropped. No data.
    cropped_flag,
};

struct RecordHeader {
    RecordType record_type;
};

#ifndef UNITTESTS
/// Duration (in ms) of the just executed prefetch
METRIC_DEF(metric_fetch_duration, "ftch_dur", METRIC_VALUE_INTEGER, 0, METRIC_HANDLER_ENABLE_ALL);
#endif

} // namespace media_prefetch

using namespace media_prefetch;

LOG_COMPONENT_DEF(MediaPrefetch, logging::Severity::debug);

// The code is not ready for two worker routines to be executed in parallel, which could happen in multi-worker executor.
// So if this changes, we need to revise the code.
// A good solution might be to adjust the AsyncJob::issue function so that it always uses the same worker throughout discarding
static_assert(AsyncJobExecutor::worker_count() == 1);

static constexpr const char *prefetch_bsod_title = "Prefetch error";

MediaPrefetchManager::MediaPrefetchManager() {
}

MediaPrefetchManager::~MediaPrefetchManager() {
#ifdef UNITTESTS
    stop();
#else
    bsod(prefetch_bsod_title);
#endif
}

MediaPrefetchManager::Status MediaPrefetchManager::read_command(ReadResult &result) {
    std::lock_guard mutex_guard(mutex);

    auto &s = shared_state;

    // If we're at the buffer end, return the appropriate error
    if (s.read_head.buffer_pos == s.read_tail.buffer_pos) {
        assert(s.read_tail.status != Status::ok);
        return s.read_tail.status;
    }

    auto &resume_pos = manager_state.read_head.gcode_pos;
    result.cropped = false;
    result.replay_pos = resume_pos;

    // Read records from the buffer until a gcode record is read
    // Worker should put the data in the buffer in such way that when there is data available to read, it ends with a gcode record.
    while (true) {
        RecordHeader header;
        read_entry(header);

        switch (header.record_type) {

        case RecordType::offset_update:
            read_entry(resume_pos.offset);
            continue;

        case RecordType::incremental_offset_update: {
            uint8_t offset_diff;
            read_entry(offset_diff);
            resume_pos.offset += offset_diff;
            continue;
        }

        case RecordType::restore_info_update:
            read_entry(resume_pos.restore_info);
            continue;

        case RecordType::cropped_flag:
            result.cropped = true;
            continue;

        case RecordType::plain_gcode: {
            uint8_t gcode_len;
            read_entry(gcode_len);

            assert(gcode_len < result.gcode.size());
            read_entry_raw(result.gcode.data(), gcode_len);

            // The gcode in the buffer was not null-terminated, so add the null here
            result.gcode[gcode_len] = '\0';
            break;
        }

        case RecordType::compressed_gcode: {
            uint8_t compressed_len;
            std::array<uint8_t, MAX_CMD_SIZE> compressed_data;

            read_entry<uint8_t>(compressed_len);
            assert(compressed_len < compressed_data.size());
            read_entry_raw(compressed_data.data(), compressed_len);

            decompress_gcode(compressed_data.data(), compressed_len, result.gcode);
            break;
        }

        default:
            bsod(prefetch_bsod_title);
        }

        // Only gcode types in the switch above use break; to get from the switch -> that means that the loop should end
        // Metadata record use continue, so they do not reach this break
        break;
    }

    result.resume_pos = resume_pos;

    // Let the worker know that we've read stuff and it can reuse the memory
    assert(s.commands_in_buffer > 0);
    s.commands_in_buffer--;
    return Status::ok;
}

void MediaPrefetchManager::start(const char *filepath, const GCodeReaderPosition &position) {
    stop();

    log_debug(MediaPrefetch, "Media prefetch start '%s' %" PRIu32, filepath, position.offset);

    // Set up for the new file
    {
        std::lock_guard mutex_guard(mutex);

        assert(strlen(filepath) < shared_state.filepath.size());
        strlcpy(shared_state.filepath.data(), filepath, shared_state.filepath.size());

        manager_state.read_head.gcode_pos = position;
        shared_state.worker_reset_pending = true;
        shared_state.read_tail.gcode_pos = position;
        shared_state.read_tail.status = Status::end_of_buffer;
    }
}

void MediaPrefetchManager::stop() {
    log_debug(MediaPrefetch, "Media prefetch stop");

    // Discard any pending jobs we wanted to do
    worker_job.discard();

    // Reset us to the initial state
    {
        std::lock_guard mutex_guard(mutex);
        shared_state = {};
        manager_state = {};
    }

    // Issue a "fetch" (with fetch_requested = false) that closes the file handle
    worker_job.issue([this](AsyncJobExecutionControl &control) { fetch_routine(control); });
}

bool MediaPrefetchManager::check_buffer_empty() const {
    std::lock_guard mutex_guard(mutex);
    return (shared_state.read_head.buffer_pos == shared_state.read_tail.buffer_pos) && (shared_state.read_tail.status == Status::end_of_buffer);
}

bool MediaPrefetchManager::check_ready_to_start_print() const {
    const auto metrics = get_metrics();
    return metrics.buffer_occupancy_percent > 90 || metrics.tail_status == Status::end_of_file;
}

void MediaPrefetchManager::issue_fetch() {
    {
        std::lock_guard mutex_guard(mutex);

        // Some fetch is already running -> let it finish
        if (worker_job.is_active() && shared_state.fetch_requested) {
            return;
        }

        shared_state.fetch_requested = true;
    }

    log_debug(MediaPrefetch, "Media prefetch issue fetch");
    worker_job.issue([this](AsyncJobExecutionControl &control) { fetch_routine(control); });
}

MediaPrefetchManager::Metrics MediaPrefetchManager::get_metrics() const {
    std::lock_guard mutex_guard(mutex);
    auto &s = shared_state;
    return Metrics {
        .commands_in_buffer = s.commands_in_buffer,
        .stream_size_estimate = s.stream_size_estimate,
        .buffer_occupancy_percent = static_cast<uint8_t>(((s.read_tail.buffer_pos - s.read_head.buffer_pos + buffer_size) % buffer_size * 100) / buffer_size),
        .tail_status = s.read_tail.status,
    };
}

void MediaPrefetchManager::fetch_routine(AsyncJobExecutionControl &control) {
#ifndef UNITTESTS
    // Increase the priority during the fetch routine - we want to win fights over USB with Connect
    const auto thread_id = osThreadGetId();
    const auto original_priority = osThreadGetPriority(thread_id);
    osThreadSetPriority(thread_id, TASK_PRIORITY_MEDIA_PREFETCH);

    ScopeGuard exit_guard = [start = ticks_ms(), thread_id, original_priority] {
        osThreadSetPriority(thread_id, original_priority);

        const int32_t prefetch_duration = ticks_diff(ticks_ms(), start);
        log_debug(MediaPrefetch, "Media prefetch took %" PRIi32 " ms", prefetch_duration);
        metric_record_integer(&metric_fetch_duration, prefetch_duration);
    };
#endif

    auto &s = worker_state;

    // (Re)initialize the reader if necessary
    {
        bool reader_needs_initialization = !s.gcode_reader.is_open();
        bool fetch_requested;

        // Hold some variables on the stack so that we can do the AnyGcodeFormatReader initialization outside of the mutex
        decltype(shared_state.filepath) filepath;
        GCodeReaderStreamRestoreInfo stream_restore_info;

        {
            std::lock_guard mutex_guard(mutex);

            if (control.is_discarded()) {
                return;
            }

            filepath = shared_state.filepath;
            fetch_requested = shared_state.fetch_requested;

            if (shared_state.worker_reset_pending) {
                shared_state.worker_reset_pending = false;

                reader_needs_initialization = true;
                stream_restore_info = shared_state.read_tail.gcode_pos.restore_info;

                // Reset the worker state to the read_tail
                s = {};
                s.gcode_reader_pos = shared_state.read_tail.gcode_pos.offset;
                s.write_tail.buffer_pos = shared_state.read_tail.buffer_pos;
                s.write_tail.gcode_pos = shared_state.read_tail.gcode_pos;
            }

            // Fetch read head so that we know the end of the region where we can write to
            s.read_head.buffer_pos = shared_state.read_head.buffer_pos;
        }

        log_debug(MediaPrefetch, "Fetch start '%s' from %" PRIu32, filepath.data(), s.gcode_reader_pos);

        if (reader_needs_initialization) {
            // First destroy, then create, to prevent having two readers at the same time
            s.gcode_reader = {};

            if (filepath[0] == '\0') {
                // No file, just close the reader and exit
                return;
            }

            s.gcode_reader = AnyGcodeFormatReader(filepath.data());

            if (!s.gcode_reader.is_open()) {
                log_debug(MediaPrefetch, "Fetch open failed");

                std::lock_guard mutex_guard(mutex);

                if (control.is_discarded()) {
                    return;
                }

                shared_state.read_tail.status = Status::usb_error;
                return;
            }

            s.gcode_reader->set_restore_info(stream_restore_info);
            const auto stream_start_result = s.gcode_reader->stream_gcode_start(s.gcode_reader_pos);

            if (stream_start_result != IGcodeReader::Result_t::RESULT_OK) {
                log_debug(MediaPrefetch, "Fetch start stream fail: %i", static_cast<int>(stream_start_result));
                fetch_handle_error(control, stream_start_result);
                return;
            }

            log_debug(MediaPrefetch, "Fetch reader (re)opened");

            // Update file size estimate
            const auto stream_size_estimate = s.gcode_reader->get_gcode_stream_size_estimate();

            {
                std::lock_guard mutex_guard(mutex);

                if (control.is_discarded()) {
                    return;
                }

                shared_state.stream_size_estimate = stream_size_estimate;
            }

        } else {
            s.gcode_reader->update_validity(filepath.data());
        }

        // Fetch not requested - we just do the file handle thing and exit
        if (!fetch_requested) {
            return;
        }
    }

    [[maybe_unused]] const auto initial_gcode_pos = s.gcode_reader_pos;

    // Keep fetching until we run out of space
    while (true) {
        // If the command buffer contains a full command, flush it
        // If we failed, it means that there is not enough space in the buffer and we need to continue flushing in the next fetch
        if (s.command_buffer.flush_pending && !fetch_flush_command(control)) {
            log_debug(MediaPrefetch, "Flush command stopped: %" PRIu16 " %" PRIu16, s.write_tail.buffer_pos, s.read_head.buffer_pos);
            break;
        }

        if (!fetch_command(control)) {
            break;
        }
    }

    log_debug(MediaPrefetch, "Fetch stop at %" PRIu32 ", fetched %" PRIu32, s.gcode_reader_pos, s.gcode_reader_pos - initial_gcode_pos);
}

bool MediaPrefetchManager::fetch_flush_command(AsyncJobExecutionControl &control) {
    auto &s = worker_state;

    // If the restore info changed, publish a relevant record
    if (const auto ri = s.gcode_reader->get_restore_info(); ri != s.write_tail.gcode_pos.restore_info) {
        if (!can_write_entry<RecordHeader, GCodeReaderStreamRestoreInfo>()) {
            return false;
        }

        write_entry<RecordHeader>({ .record_type = RecordType::restore_info_update });
        write_entry(ri);

        s.write_tail.gcode_pos.restore_info = ri;
    }

    // If the gcode pos changed, publish a relevant record
    if (const auto offset = s.gcode_reader_pos; offset != s.write_tail.gcode_pos.offset) {
        assert(offset >= s.write_tail.gcode_pos.offset);
        const auto offset_diff = offset - s.write_tail.gcode_pos.offset;

        if (offset_diff < 256) {
            // Offset diff fits into one byte -> we can do incremental offset
            if (!can_write_entry<RecordHeader, uint8_t>()) {
                return false;
            }

            write_entry<RecordHeader>({ .record_type = RecordType::incremental_offset_update });
            write_entry<uint8_t>(offset_diff);

        } else {
            // Otherwise, emit full 4 byte offsetđ
            if (!can_write_entry<RecordHeader, decltype(offset)>()) {
                return false;
            }

            write_entry<RecordHeader>({ .record_type = RecordType::offset_update });
            write_entry(offset);
        }

        s.write_tail.gcode_pos.offset = offset;
    }

    /// If the cropped flag is set, send a message notifying that
    if (s.command_buffer.cropped) {
        if (!can_write_entry<RecordHeader>()) {
            return false;
        }

        write_entry<RecordHeader>({ .record_type = RecordType::cropped_flag });
        s.command_buffer.cropped = false;
    }

    // We're using one byte to encode length, cannot go longer.
    static_assert(std::tuple_size_v<decltype(s.command_buffer_data)> <= 256);

    // Try compressing the gcode
    std::array<uint8_t, MAX_CMD_SIZE> compressed_data;
    const auto command_len = s.command_buffer.write_pos;
    const auto compressed_len = compress_gcode(s.command_buffer_data.data(), compressed_data).value_or(command_len);

    if (compressed_len < command_len) {
        // If the gcode is compressable, store it compressed
        if (!can_write_entry_raw(sizeof(RecordHeader) + sizeof(uint8_t) + compressed_len)) {
            return false;
        }

        write_entry<RecordHeader>({ .record_type = RecordType::compressed_gcode });
        write_entry<uint8_t>(compressed_len);
        write_entry_raw(compressed_data.data(), compressed_len);

    } else {
        // Otherwise store it plain
        if (!can_write_entry_raw(sizeof(RecordHeader) + sizeof(uint8_t) + command_len)) {
            return false;
        }

        write_entry<RecordHeader>({ .record_type = RecordType::plain_gcode });
        write_entry<uint8_t>(command_len);
        write_entry_raw(s.command_buffer_data.data(), command_len);
    }

    // Update read_tail
    {
        std::lock_guard mutex_guard(mutex);

        if (control.is_discarded()) {
            return false;
        }

        shared_state.commands_in_buffer++;

        // Only do this now that a gcode is the last record in the buffer.
        // The read_command function needs to be able to finish, once it starts reading
        shared_state.read_tail = {
            .gcode_pos = s.write_tail.gcode_pos,
            .buffer_pos = s.write_tail.buffer_pos,
            .status = Status::end_of_buffer,
        };

        // Fetch read head while we're at it, in case the client read some commands, so what we get more space for writing
        s.read_head.buffer_pos = shared_state.read_head.buffer_pos;
    }

    // Reset the command buffer state
    s.command_buffer = {};

    return true;
}

bool MediaPrefetchManager::fetch_command(AsyncJobExecutionControl &control) {
    using SR = IGcodeReader::Result_t;

    auto &s = worker_state;
    auto &buf_pos = s.command_buffer.write_pos;

    char ch;
    const auto getc_result = s.gcode_reader->stream_getc(ch);

    if (getc_result == SR::RESULT_EOF && buf_pos > 0) {
        // EOF, but there is something in the buffer -> first flush the remaining command
        ch = '\n';

    } else if (getc_result != SR::RESULT_OK) {
        fetch_handle_error(control, getc_result);
        return false;

    } else {
        // We've fetched one byte, increase the offset in the stream
        s.gcode_reader_pos++;
    }

    if (ch == '\n') {
        if (buf_pos == s.command_buffer_data.size()) {
            log_warning(MediaPrefetch, "Warning: gcode didn't fit in the command buffer, cropped");
            buf_pos--;
            s.command_buffer.cropped = true;
        }

        s.command_buffer_data[buf_pos] = '\0';

        if (s.command_buffer.cropped) {
            log_warning(MediaPrefetch, "Cropped gcode: %s", s.command_buffer_data.data());
        }

        buf_pos = compact_gcode(s.command_buffer_data.data());

        // Empty line -> just reset the buffer and keep on reading
        if (buf_pos == 0) {
            s.command_buffer = {};
            return true;
        }

        s.command_buffer.flush_pending = true;

    } else if (buf_pos == 0 && ch == ';') {
        // If the line starts with a comment, skip it
        s.command_buffer.skip_rest_of_line = true;

    } else if (buf_pos == 0 && isspace(ch)) {
        // Skip whitespaces at the beginning of the line

    } else if (!s.command_buffer.skip_rest_of_line && buf_pos < s.command_buffer_data.size()) {
        // If we get a gcode that's longer than our buffer, we do best-effort: crop it and try to execute it anyway (but show a warning)
        s.command_buffer_data[buf_pos++] = ch;
    }

    return true;
}

void MediaPrefetchManager::fetch_handle_error(AsyncJobExecutionControl &control, IGcodeReader::Result_t error) {
    using SR = IGcodeReader::Result_t;

    assert(error != SR::RESULT_OK);
    log_debug(MediaPrefetch, "Read error: %i", static_cast<int>(error));

    static constexpr EnumArray<SR, Status, static_cast<int>(SR::_RESULT_LAST) + 1> status_map {
        { SR::RESULT_OK, Status::end_of_buffer },
        { SR::RESULT_EOF, Status::end_of_file },
        { SR::RESULT_TIMEOUT, Status::end_of_buffer },
        { SR::RESULT_ERROR, Status::usb_error },
        { SR::RESULT_OUT_OF_RANGE, Status::not_downloaded },
        { SR::RESULT_CORRUPT, Status::corruption },
    };
    const auto status = status_map[error];
    const bool is_error = this->is_error(status);

    // Close the reader now, release the handle as soon as possible
    if (status != Status::end_of_buffer) {
        worker_state.gcode_reader = {};
    }

    // If the read errored, update the tail error status and wait for another fetch
    std::lock_guard mutex_guard(mutex);

    // Early return if the job was discarded
    // Do this after locking the mutex, to ensure proper synchronization (stop() calls discard() before locking the mutex)
    if (control.is_discarded()) {
        return;
    }

    shared_state.read_tail.status = status;

    // If we get in an error, trying to just keep reading likely wouldn't work -> reset the worker, re-try to continue from read_tail
    shared_state.worker_reset_pending |= is_error;
}

bool MediaPrefetchManager::can_read_entry_raw(size_t bytes) const {
    assert(bytes < buffer_size);

    const size_t read_pos = shared_state.read_head.buffer_pos;
    const size_t read_tail = shared_state.read_tail.buffer_pos;
    const size_t new_read_pos = (read_pos + bytes) % buffer_size;

    // Check that we don't cross the read tail.
    // If we wrapped around the buffer, we check for the opposite.
    const bool does_wrap = (new_read_pos < read_pos);

    // This is basically the only place where can_read_entry_raw differs from can_write_entry_raw
    // And it's correct - for reading, we can catch up to the tail. But in writing, catching up would mean wrapping.
    const bool does_cross_tail = ((read_pos <= read_tail) != (new_read_pos <= read_tail));

    return (does_cross_tail == does_wrap);
}

void MediaPrefetchManager::read_entry_raw(void *target, size_t bytes) {
    assert(bytes < buffer_size);

    if (!can_read_entry_raw(bytes)) {
        bsod(prefetch_bsod_title);
    }

    size_t &read_pos = shared_state.read_head.buffer_pos;

    // Possibly split into two memcpy calls, if the data wraps around the buffer end
    const auto nonwrapped_bytes = std::min(bytes, buffer_size - read_pos);
    memcpy(target, &buffer[read_pos], bytes);
    memcpy(reinterpret_cast<uint8_t *>(target) + nonwrapped_bytes, &buffer[0], bytes - nonwrapped_bytes);

    read_pos = (read_pos + bytes) % buffer_size;
}

bool MediaPrefetchManager::can_write_entry_raw(size_t bytes) const {
    assert(bytes < buffer_size);

    const size_t write_pos = worker_state.write_tail.buffer_pos;
    const size_t new_write_pos = (write_pos + bytes) % buffer_size;
    const size_t read_head = worker_state.read_head.buffer_pos;

    // Check that we don't catch up the read head  - that would be interpreted as write_pos having no data.
    // If we wrapped around the buffer, we check for the opposite.
    const bool does_wrap = (new_write_pos < write_pos);

    // This is basically the only place where can_read_entry_raw differs from can_write_entry_raw
    // And it's correct - for reading, we can catch up to the tail. But in writing, catching up would mean wrapping.
    const bool does_catch_up_read_head = ((write_pos < read_head) != (new_write_pos < read_head));

    return (does_catch_up_read_head == does_wrap);
}

void MediaPrefetchManager::write_entry_raw(const void *data, size_t bytes) {
    if (!can_write_entry_raw(bytes)) {
        bsod(prefetch_bsod_title);
    }

    size_t &write_pos = worker_state.write_tail.buffer_pos;

    // Possibly split into two memcpy calls, if the data wraps around the buffer end
    const size_t nonwrapped_bytes = std::min(bytes, buffer_size - write_pos);
    memcpy(&buffer[write_pos], data, nonwrapped_bytes);
    memcpy(&buffer[0], reinterpret_cast<const uint8_t *>(data) + nonwrapped_bytes, bytes - nonwrapped_bytes);

    write_pos = (write_pos + bytes) % buffer_size;
}
