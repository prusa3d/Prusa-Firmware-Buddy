#include "inject_queue.hpp"
#include "media_prefetch/prefetch_compression.hpp"

#include <gcode_reader_any.hpp>
#include <logging/log.hpp>
#include <str_utils.hpp>

#include <logging/log.hpp>

InjectQueue inject_queue; // instance

LOG_COMPONENT_REF(MarlinServer);

void InjectQueue::load_gcodes_from_file_callback(const char *filepath, AsyncJobExecutionControl &control) {
    AnyGcodeFormatReader reader(filepath);
    // Error conditions
    if (!reader.is_open() || reader->get_gcode_stream_size_estimate() >= InjectQueue::gcode_stream_buffer_size) {
        inject_queue.buffer_state = BufferState::error;
        log_error(MarlinServer, "InjectQueue: fail to open file");
        return;
    }

    while (true) {
        const auto result = reader->stream_gcode_start();
        if (result == IGcodeReader::Result_t::RESULT_OK) {
            break;
        } else if (result != IGcodeReader::Result_t::RESULT_TIMEOUT) {
            inject_queue.buffer_state = BufferState::error;
            log_error(MarlinServer, "InjectQueue: fail to start reading");
            osDelay(1);
            return;
        }
    }

    // gcode_stream_buffer is used to store filepath & then the gcode stream itself
    // Filepath doesn't need separate buffer - file is opened and after that buffer is no longer needed and is overwritten here
    StringBuilder str_builder(inject_queue.gcode_stream_buffer);
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
            inject_queue.buffer_state = BufferState::error;
            log_error(MarlinServer, "InjectQueue: fail to read from the file (stream_get_line)");
            return;
        }

        // Get rid of white spaces and comments
        const size_t line_length = media_prefetch::compact_gcode(line_buff.buffer.data());
        if (line_length == 0) {
            continue;
        }

        if (!first_line) {
            // G-Code stream is terminated by '\0' and has '\n' separating individual G-Codes
            str_builder.append_char('\n');
        }
        str_builder.append_string(line_buff.buffer.data());
        if (!str_builder.is_ok()) {
            inject_queue.buffer_state = BufferState::error;
            log_error(MarlinServer, "InjectQueue: fail to build up a gcode stream in the buffer");
            return;
        }
        first_line = false;
    }

    inject_queue.buffer_state = BufferState::ready;
}

bool InjectQueue::try_push(InjectQueueRecord record) {
    if (!queue.is_full()) {
        queue.put(record);
        return true;
    }
    return false;
}

std::expected<const char *, InjectQueue::GetGCodeError> InjectQueue::get_gcode() {

    // Check if we're buffering some gcode, if yes, handle the buffer
    switch (buffer_state) {

    case BufferState::buffering:
        return std::unexpected(GetGCodeError::buffering);

    case BufferState::ready:
        buffer_state = BufferState::idle;
        return gcode_stream_buffer;

    case BufferState::error:
        buffer_state = BufferState::idle;
        return std::unexpected(GetGCodeError::loading_aborted);

    case BufferState::idle:
        break;
    }

    // If we've reached here, we need to check the queue and do something
    if (queue.is_empty()) {
        return std::unexpected(GetGCodeError::empty);
    }

    const auto item = queue.get();

    // The item is a literal -> just return the literal
    if (const auto *val = std::get_if<GCodeLiteral>(&item)) {
        return val->gcode;
    }

    // Otherwise, the item is a file, we need to start buffering

    // Using the gcode_stream buffer to store filepath - to avoid RAM cost of another filepath buffer
    // File is opened by async job and and then its safe to use it for buffering the file
    StringBuilder filepath(gcode_stream_buffer);
    filepath.append_string("/usb/macros/");
    std::visit([&]<typename T>(const T &v) {
        if constexpr (std::is_same_v<T, GCodeMacroButton>) {
            filepath.append_printf("btn_%hu", v.button);

        } else if constexpr (std::is_same_v<T, GCodePresetMacro>) {
            filepath.append_string(gcode_macro_preset_filanames[v]);

        } else if constexpr (std::is_same_v<T, GCodeLiteral>) {
            assert(0);

        } else {
            static_assert(false);
        }
    },
        item);
    filepath.append_string(".gcode");

    if (!filepath.is_ok()) {
        return std::unexpected(GetGCodeError::loading_aborted);
    }

    assert(!worker_job.is_active());
    buffer_state = BufferState::buffering;
    worker_job.issue([this](AsyncJobExecutionControl &control) {
        InjectQueue::load_gcodes_from_file_callback(gcode_stream_buffer, control);
    });

    return std::unexpected(GetGCodeError::buffering);
}
