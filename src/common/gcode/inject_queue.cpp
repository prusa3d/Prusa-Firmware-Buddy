#include "inject_queue.hpp"
#include "media_prefetch/prefetch_compression.hpp"
#include <str_utils.hpp>
#include <gcode_reader_any.hpp>
#include <logging/log.hpp>
#include <str_utils.hpp>

#include <logging/log.hpp>

InjectQueue inject_queue; // instance

LOG_COMPONENT_REF(MarlinServer);

void InjectQueue::load_gcodes_from_file_callback(AsyncJobExecutionControl &control) {
    AnyGcodeFormatReader reader(inject_queue.get_buffer().data());

    // gcode_stream_buffer is used to store filepath & then the gcode stream itself
    // Filepath doesn't need separate buffer - file is opened and after that buffer is no longer needed and is overwritten here
    StringBuilder str_builder(inject_queue.get_buffer());

    // Error conditions
    if (!reader.is_open()) {
        const char *fallback = inject_queue.get_fallback();
        if (fallback != nullptr) {
            str_builder.append_string(fallback);
            log_info(MarlinServer, "InjectQueue: file not found, using fallback");
            inject_queue.change_buffer_state(InjectQueue::BufferState::ready);
            return;
        }
        inject_queue.change_buffer_state(InjectQueue::BufferState::error);
        log_error(MarlinServer, "InjectQueue: fail to open file");
        return;
    }

    while (true) {
        const auto result = reader->stream_gcode_start();
        if (result == IGcodeReader::Result_t::RESULT_OK) {
            break;
        } else if (result != IGcodeReader::Result_t::RESULT_TIMEOUT) {
            inject_queue.change_buffer_state(InjectQueue::BufferState::error);
            log_error(MarlinServer, "InjectQueue: fail to start reading");
            osDelay(1);
            return;
        }
        osDelay(1);
    }

    bool first_line = true;
    while (true) {
        if (control.is_discarded()) {
            return;
        }

        GcodeBuffer line_buff;
        const auto result = reader->stream_get_line(line_buff, IGcodeReader::Continuations::Discard);
        if (result == IGcodeReader::Result_t::RESULT_EOF) {
            break;
        } else if (result == IGcodeReader::Result_t::RESULT_TIMEOUT) {
            osDelay(1);
            continue;
        } else if (result != IGcodeReader::Result_t::RESULT_OK) {
            inject_queue.change_buffer_state(InjectQueue::BufferState::error);
            log_error(MarlinServer, "InjectQueue: fail to read from the file (stream_get_line)");
            return;
        }

        // Get rid of white spaces and comments
        const size_t line_length = media_prefetch::compact_gcode(line_buff.buffer.data());
        if (line_length == 0) {
            continue;
        }
        if (str_builder.byte_count() + line_length > InjectQueue::gcode_stream_buffer_size) {
            // File is too large to buffer
            inject_queue.change_buffer_state(InjectQueue::BufferState::error);
            log_error(MarlinServer, "InjectQueue: buffered file is too large");
            return;
        }

        if (!first_line) {
            // G-Code stream is terminated by '\0' and has '\n' separating individual G-Codes
            str_builder.append_char('\n');
        }
        str_builder.append_string(line_buff.buffer.data());
        if (!str_builder.is_ok()) {
            inject_queue.change_buffer_state(InjectQueue::BufferState::error);
            log_error(MarlinServer, "InjectQueue: fail to build up a gcode stream in the buffer");
            return;
        }
        first_line = false;
    }

    inject_queue.change_buffer_state(InjectQueue::BufferState::ready);
}

bool InjectQueue::try_push(InjectQueueRecord record) {
    return queue.try_put(record);
}

std::expected<const char *, InjectQueue::GetGCodeError> InjectQueue::get_gcode() {

    // Check if we're buffering some gcode, if yes, handle the buffer
    switch (buffer_state) {

    case BufferState::buffering:
        return std::unexpected(GetGCodeError::buffering);

    case BufferState::ready:
        change_buffer_state(BufferState::idle);
        return gcode_stream_buffer;

    case BufferState::error:
        change_buffer_state(BufferState::idle);
        return std::unexpected(GetGCodeError::loading_aborted);

    case BufferState::idle:
        break;
    }

    // If we've reached here, we need to check the queue and do something
    InjectQueueRecord item;
    if (!queue.try_get(item)) {
        return std::unexpected(GetGCodeError::empty);
    }

    // The item is a literal -> just return the literal
    if (const auto *val = std::get_if<GCodeLiteral>(&item)) {
        return val->gcode;
    }

    assert(!worker_job.is_active());

    // Otherwise, the item is a file, we need to start buffering

    // Using the gcode_stream buffer to store filepath - to avoid RAM cost of another filepath buffer
    // File is opened by async job and and then its safe to use it for buffering the file
    StringBuilder filepath(gcode_stream_buffer);
    filepath.append_string("/usb/macros/");
    std::visit([&]<typename T>(const T &v) {
        if constexpr (std::is_same_v<T, GCodeMacroButton>) {
            filepath.append_printf("btn_%hu", v.button);
        } else if constexpr (std::is_same_v<T, GCodeFilename>) {
            filepath.append_string(v.name);
            gcode_fallback = v.fallback;
        } else if constexpr (std::is_same_v<T, GCodeLiteral>) {
            assert(0); // handled earlier
        } else {
            static_assert(false);
        }
    },
        item);
    filepath.append_string(".gcode");

    if (!filepath.is_ok()) {
        return std::unexpected(GetGCodeError::loading_aborted);
    }

    change_buffer_state(BufferState::buffering);
    // Set up default async_job_callback - buffering from file
    worker_job.issue(load_gcodes_from_file_callback);
    return std::unexpected(GetGCodeError::buffering);
}
