#pragma once

#include <async_job/async_job.hpp>

#include <atomic>
#include <expected>

class GCodeLoader {
public:
    enum class BufferState {
        idle,
        buffering,
        ready,
        error,
    };

    static constexpr size_t gcode_stream_buffer_size = 1024;

    void load_gcode(const char *filename, const char *fallback = nullptr);

    std::expected<char *, BufferState> get_result();

    void reset();

private:
    void load_gcode_callback(AsyncJobExecutionControl &control);

    std::atomic<BufferState> state = BufferState::idle;
    char gcode_buffer[gcode_stream_buffer_size]; //!< buffer for temporary storing gcode filepath & compiling gcode stream from a file
    const char *gcode_fallback { nullptr }; //!< fallback gcode to use in case the file is not found
    AsyncJob worker_job; //!< Used for asynchronous buffering of gcode stream from a file or callback execution
};
