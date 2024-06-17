#include "media_prefetch.hpp"

namespace media_prefetch {

enum class RecordType : uint8_t {
    /// RecordType + uint8_t (strlen) + non-null-terminated string
    plain_gcode,

    /// GCodeReaderStreamRestoreInfo follows
    /// Emitted in to the buffer whenever stream restore info changes.
    restore_info_update,

    /// uint32_t offset follows
    offset_update,
};

struct RecordHeader {
    RecordType record_type;
};

} // namespace media_prefetch

using namespace media_prefetch;

LOG_COMPONENT_DEF(MediaPrefetch, LOG_SEVERITY_DEBUG);

// The code is not ready for two worker routines to be executed in parallel, which could happen in multi-worker executor.
// So if this changes, we need to revise the code.
// A good solution might be to adjust the AsyncJob::issue function so that it always uses the same worker throughout discarding
static_assert(AsyncJobExecutor::worker_count() == 1);

static constexpr const char *prefetch_bsod_title = "Prefetch error";

MediaPrefetchManager::MediaPrefetchManager() {
}

MediaPrefetchManager::~MediaPrefetchManager() {
    // We're not ready for this, should never happen
    std::terminate();
}

MediaPrefetchManager::Status MediaPrefetchManager::read_command(ReadResult &result) {
    auto &s = manager_state;

    // Get the readable region boundaires
    {
        std::lock_guard mutex_guard(mutex);

        s.read_head.buffer_pos = shared_state.read_head.buffer_pos;
        s.read_tail.buffer_pos = shared_state.read_tail.buffer_pos;

        // While we have the mutex locked, update some stats variables
        s.buffer_occupancy = float((shared_state.read_tail.buffer_pos - shared_state.read_head.buffer_pos + buffer_size) % buffer_size) / buffer_size;
        s.read_tail_status = shared_state.read_tail.status;
    }

    // If we're at the buffer end, return the appropriate error
    if (s.read_head.buffer_pos == s.read_tail.buffer_pos) {
        assert(manager_state.read_tail_status != Status::ok);
        return manager_state.read_tail_status;
    }

    auto &resume_pos = s.read_head.gcode_pos;
    const GCodeReaderPosition replay_pos = resume_pos;

    // Read records from the buffer until a gcode record is read
    // Worker should put the data in the buffer in such way that there
    while (true) {
        RecordHeader header;
        read_entry(header);

        switch (header.record_type) {

        case RecordType::offset_update:
            read_entry(resume_pos.offset);
            continue;

        case RecordType::restore_info_update:
            read_entry(resume_pos.restore_info);
            continue;

        case RecordType::plain_gcode: {
            uint8_t gcode_len;
            read_entry(gcode_len);

            assert(gcode_len < result.gcode.size());
            auto gcode_buffer = result.gcode.data();
            read_entry_raw(gcode_buffer, gcode_len);

            // The gcode in the buffer was not null-terminated, so add the null here
            result.gcode[gcode_len] = '\0';
            break;
        }

        default:
            bsod(prefetch_bsod_title);
        }

        // Only gcode types in the switch above use break; to get from the switch -> that means that the loop should end
        // Metadata record use continue, so they do not reach this break
        break;
    }

    // Let the worker know that we've read stuff and it can reuse the memory
    {
        std::lock_guard mutex_guard(mutex);
        shared_state.read_head.buffer_pos = s.read_head.buffer_pos;
    }

    result.resume_pos = resume_pos;
    result.replay_pos = replay_pos;
    return Status::ok;
}

void MediaPrefetchManager::start(const char *filepath, const GCodeReaderPosition &position) {
    stop();

    log_debug(MediaPrefetch, "Media prefetch start '%s' %" PRIu32, filepath, position.offset);

    // The worker is definitely invalidated after we've called stop, so we can write into shared_data without synchronization
    assert(strlen(filepath) < shared_state.filepath.size());
    strlcpy(shared_state.filepath.data(), filepath, shared_state.filepath.size());

    assert(shared_state.worker_reset_pending);
    shared_state.read_tail.gcode_pos = position;
    shared_state.read_tail.status = Status::end_of_buffer;

    issue_fetch();
}

void MediaPrefetchManager::stop() {
    log_debug(MediaPrefetch, "Media prefetch stop");

    // Discard any pending jobs we wanted to do
    worker_job.discard();

    // Reset us to the initial state
    std::lock_guard mutex_guard(mutex);
    shared_state = {};
    manager_state = {};
}

bool MediaPrefetchManager::check_buffer_empty() const {
    std::lock_guard mutex_guard(mutex);
    return (shared_state.read_head.buffer_pos == shared_state.read_tail.buffer_pos) && (shared_state.read_tail.status == Status::end_of_buffer);
}

void MediaPrefetchManager::issue_fetch(bool force) {
    if (!force && buffer_occupancy() > 0.6f) {
        return;
    }

    // Some fetch is already running -> let it finish
    if (worker_job.is_active()) {
        return;
    }

    log_debug(MediaPrefetch, "Media prefetch issue fetch");
    worker_job.issue([this](AsyncJobExecutionControl &control) { fetch_routine(control); });
}

void MediaPrefetchManager::fetch_routine(AsyncJobExecutionControl &control) {
    auto &s = worker_state;

    log_debug(MediaPrefetch, "Fetch start");

    // (Re)initialize the reader if necessary
    {
        bool reader_needs_initialization = !s.gcode_reader.is_open();

        // Hold some variables on the stack so that we can do the AnyGcodeFormatReader initialization outside of the mutex
        decltype(shared_state.filepath) filepath;
        GCodeReaderStreamRestoreInfo stream_restore_info;

        {
            std::lock_guard mutex_guard(mutex);

            if (control.is_discarded()) {
                return;
            }

            filepath = shared_state.filepath;

            if (shared_state.worker_reset_pending) {
                shared_state.worker_reset_pending = false;

                reader_needs_initialization = true;
                stream_restore_info = shared_state.read_tail.gcode_pos.restore_info;

                // Reset the worker state to the read_tail
                s = {};
                s.gcode_reader_pos = shared_state.read_tail.gcode_pos.offset;
                s.write_tail.buffer_pos = shared_state.read_tail.buffer_pos;
            }

            // Fetch read head so that we know the end of the region where we can write to
            s.read_head.buffer_pos = shared_state.read_head.buffer_pos;
        }

        if (reader_needs_initialization) {
            s.gcode_reader = AnyGcodeFormatReader(filepath.data());
            if (!s.gcode_reader.is_open()) {
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
                // Close the reader so that stream_gcode_start is attempted on the next fetch
                s.gcode_reader = {};

                fetch_report_error(control, stream_start_result);
                return;
            }

            // Update file size estimate
            const auto file_size_estimate = s.gcode_reader->get_gcode_stream_size_estimate();

            {
                std::lock_guard mutex_guard(mutex);

                if (control.is_discarded()) {
                    return;
                }

                shared_state.file_size_estimate = file_size_estimate;
            }

        } else {
            transfers::Transfer::Path path(filepath.data());
            s.gcode_reader->update_validity(path);
        }
    }

    // Keep fetching until we run out of space
    while (true) {
        // If the command buffer contains a full command, flush it
        // If we failed, it means that there is not enough space in the buffer and we need to continue flushing in the next fetch
        if (s.command_buffer.flush_pending && !fetch_flush_command(control)) {
            log_debug(MediaPrefetch, "Buffer full");
            break;
        }

        // Read next command from the stream
        while (!s.command_buffer.flush_pending) {
            fetch_command(control);
        }
    }

    log_debug(MediaPrefetch, "Fetch finished");
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
        if (!can_write_entry<RecordHeader, decltype(offset)>()) {
            return false;
        }

        write_entry<RecordHeader>({ .record_type = RecordType::offset_update });
        write_entry(offset);

        s.write_tail.gcode_pos.offset = offset;
    }

    // Now flush the gcode itself in the form of plain gcode
    // TODO: Add compression: comment removing, space removing, special records for G1 and such
    {
        const auto command_len = strlen(s.command_buffer_data.data());
        if (!can_write_entry_raw(sizeof(RecordHeader) + sizeof(uint8_t) + command_len)) {
            return false;
        }

        write_entry<RecordHeader>({ .record_type = RecordType::plain_gcode });
        write_entry<uint8_t>(command_len);
        write_entry_raw(s.command_buffer_data.data(), command_len);

        // Update read_tail
        {
            std::lock_guard mutex_guard(mutex);

            if (control.is_discarded()) {
                return false;
            }

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
    }

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
        fetch_report_error(control, getc_result);
        return false;

    } else {
        // We've fetched one byte, increase the offset in the file
        s.gcode_reader_pos++;
    }

    if (ch == '\n') {
        if (buf_pos == s.command_buffer_data.size()) {
            // TODO: propagate overflow warning to the UI

            log_warning(MediaPrefetch, "Warning: gcode didn't fit in the command buffer, cropped");
            buf_pos--;
        }

        s.command_buffer_data[buf_pos] = '\0';
        s.command_buffer.flush_pending = true;

    } else if (buf_pos < s.command_buffer_data.size()) {
        // If we get a gcode that's longer than our buffer, we do best-effort: crop it and try to execute it anyway (but show a warning)
        s.command_buffer_data[buf_pos++] = ch;
    }

    return true;
}

void MediaPrefetchManager::fetch_report_error(AsyncJobExecutionControl &control, IGcodeReader::Result_t error) {
    using SR = IGcodeReader::Result_t;

    assert(error != SR::RESULT_OK);

    // If the read errored, update the tail error status and wait for another fetch
    std::lock_guard mutex_guard(mutex);

    // Early return if the job was discarded
    // Do this after locking the mutex, to ensure proper synchronization (stop() calls discard() before locking the mutex)
    if (control.is_discarded()) {
        return;
    }

    static constexpr EnumArray<SR, Status, IGcodeReader::result_t_cnt> status_map {
        { SR::RESULT_OK, Status::end_of_buffer },
        { SR::RESULT_EOF, Status::end_of_file },
        { SR::RESULT_TIMEOUT, Status::end_of_buffer },
        { SR::RESULT_ERROR, Status::usb_error },
        { SR::RESULT_OUT_OF_RANGE, Status::end_of_buffer },
        { SR::RESULT_CORRUPT, Status::usb_error },
    };
    const auto status = status_map[error];
    shared_state.read_tail.status = status;

    // If we get in an error, trying to just keep reading likely wouldn't work -> reset the worker, re-try to continue from read_tail
    if (is_error(status)) {
        shared_state.worker_reset_pending = true;
    }
}

void MediaPrefetchManager::read_entry_raw(void *target, size_t bytes) {
    size_t &read_pos = manager_state.read_head.buffer_pos;
    const auto read_tail = manager_state.read_tail.buffer_pos;

    // If the reading would get out of the buffer bounds, wrap
    if (read_pos + bytes > buffer_size) {
        read_pos = 0;
    }

    // Check that we haven't got over read_data_end_pos - that should never happen
    // read_entry should only be called when we know there is the right data
    // We can catch up to the read_tail, but we cannot cross it
    if ((read_pos <= read_tail) != (read_pos + bytes <= read_tail)) {
        bsod(prefetch_bsod_title);
    }

    memcpy(target, &buffer[read_pos], bytes);
    read_pos += bytes;
}

bool MediaPrefetchManager::can_write_entry_raw(size_t bytes) const {
    auto write_pos = worker_state.write_tail.buffer_pos;
    const auto read_head = worker_state.read_head.buffer_pos;

    // If the writing would get out of the buffer bounds, wrap
    if (write_pos + bytes > buffer_size) {
        // If the wrapping would result in crossing the stop_mark, we cannot do that
        if (read_head > write_pos) {
            return false;
        }

        write_pos = 0;
    }

    // We can not catch up to the read_head - that would be interpreted as write_pos having no data
    return (write_pos < read_head) == (write_pos + bytes < read_head);
}

void MediaPrefetchManager::write_entry_raw(const void *data, size_t bytes) {
    if (!can_write_entry_raw(bytes)) {
        bsod(prefetch_bsod_title);
    }

    auto &write_pos = worker_state.write_tail.buffer_pos;

    // If the writing would get out of the buffer bounds, wrap
    if (write_pos + bytes > buffer_size) {
        write_pos = 0;
    }

    memcpy(&buffer[write_pos], data, bytes);
    write_pos += bytes;
}
