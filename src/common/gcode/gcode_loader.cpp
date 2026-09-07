#include "gcode_loader.hpp"

#include "media_prefetch/prefetch_compression.hpp"

#include <logging/log.hpp>
#include <gcode_reader_any.hpp>
#include <utils/string_builder.hpp>
#include <bsod/bsod.h>

LOG_COMPONENT_REF(MarlinServer);

bool GCodeLoader::is_idle() const {
    return state == BufferState::idle;
}

bool GCodeLoader::is_buffering() const {
    return state == BufferState::buffering;
}

void GCodeLoader::load_gcode_callback(AsyncJobExecutionControl &control) {
    AnyGcodeFormatReader reader(gcode_buffer);

    // Error conditions
    if (!reader.is_open()) {
        if (gcode_fallback != nullptr) {
            StringBuilder str_builder(gcode_buffer);
            str_builder.append_string(gcode_fallback);
            state = BufferState::ready;
            return;
        }
        state = BufferState::error;
        log_error(MarlinServer, "G-Code Loader: failed to open file: %s", gcode_buffer);
        return;
    }

    log_info(MarlinServer, "G-Code Loader: Loading %s", gcode_buffer);

    StringBuilder str_builder(gcode_buffer);

    while (true) {
        const auto result = reader->stream_gcode_start();
        if (result == IGcodeReader::Result_t::RESULT_OK) {
            break;
        } else if (result != IGcodeReader::Result_t::RESULT_TIMEOUT) {
            state = BufferState::error;
            log_error(MarlinServer, "G-Code Loader: failed to start reading");
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
            state = BufferState::error;
            log_error(MarlinServer, "G-Code Loader: failed to read from file");
            return;
        }

        // Get rid of white spaces and comments
        const size_t line_length = media_prefetch::compact_gcode(line_buff.buffer.data());
        if (line_length == 0) {
            continue;
        }
        if (str_builder.byte_count() + line_length > gcode_stream_buffer_size) {
            // File is too large to buffer
            state = BufferState::error;
            log_error(MarlinServer, "G-Code Loader: buffered file is too large");
            return;
        }

        if (!first_line) {
            // G-Code stream is terminated by '\0' and has '\n' separating individual G-Codes
            str_builder.append_char('\n');
        }
        str_builder.append_string(line_buff.buffer.data());
        if (!str_builder.is_ok()) {
            state = BufferState::error;
            log_error(MarlinServer, "G-Code Loader: failed to build gcode stream in the buffer");
            return;
        }
        first_line = false;
    }

    state = BufferState::ready;
}

void GCodeLoader::load_gcode(const GCodeFile &gcode_file) {
    if (state != BufferState::idle) {
        bsod("GcodeLoader::load_gcode() while not idle");
    }

    const char *delimiter = "";
    const char *dir_to_use = "";
    if (gcode_file.directory != nullptr) {
        delimiter = "/";
        dir_to_use = gcode_file.directory;
    }

    // Abuse the buffer for building the file path before we load the gcode into it in the callback
    StringBuilder filepath(gcode_buffer);
    filepath.append_printf("/usb/macros/%s%s%s.gcode", dir_to_use, delimiter, gcode_file.filename);
    gcode_fallback = gcode_file.default_gcode;

    state = BufferState::buffering;
    worker_job.issue([&](AsyncJobExecutionControl &control) { this->load_gcode_callback(control); });
}

std::expected<char *, GCodeLoader::BufferState> GCodeLoader::get_result() {
    auto state_value = state.load();
    if (state_value == BufferState::ready) {
        return gcode_buffer;
    }

    return std::unexpected(state_value);
}

void GCodeLoader::reset() {
    state = BufferState::idle;
}

std::span<char> GCodeLoader::share_buffer() {
    return std::span<char>(gcode_buffer);
}
