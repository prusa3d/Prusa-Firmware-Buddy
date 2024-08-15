#pragma once

#include "inject_queue_actions.hpp"
#include <common/circular_buffer.hpp>
#include <expected>
#include <atomic>
#include <file_list_defs.h>
#include <async_job/async_job.hpp>

class InjectQueue {
public:
    enum class BufferState {
        idle,
        buffering,
        ready,
        error,
    };

    enum GetGCodeError {
        /// There is no gcode to be injected, you can continue with processing the standard gcode
        empty,
        /// There is gcode to be injected, but it's being loaded and is not available yet
        /// Do not process standard gcode, wait till we finish loading the injected one
        buffering,
        /// Record was discarded, an error occurred during loading
        loading_aborted,

    };

    static constexpr size_t gcode_stream_buffer_size = 1024;
    static constexpr size_t queue_size = 8;

    /**
     *  Checks record validity and queue space
     *  @retval true - record was injected
     *  @retval false - record is invalid or inject_queue is full
     */
    bool try_push(InjectQueueRecord record);

    /**
     *  If a gcode is returned, the corresponding record is removed from the injection queue.
     *  @retval a gcode that is to be executed by injection or an error
     */
    std::expected<const char *, GetGCodeError> get_gcode();

    inline std::span<char> get_buffer() { return gcode_stream_buffer; }
    inline void change_buffer_state(const BufferState new_state) { buffer_state = new_state; }

private:
    static void load_gcodes_from_file_callback(AsyncJobExecutionControl &control);

    std::atomic<BufferState> buffer_state = BufferState::idle;
    char gcode_stream_buffer[gcode_stream_buffer_size]; //!< buffer for temporary storing gcode filepath & compiling gcode stream from a file
    CircularBuffer<InjectQueueRecord, queue_size> queue; //!< Queue (real size is queue_size - 1)
    AsyncJob worker_job; //!< Used for asynchronous buffering of gcode stream from a file or callback execution
};

extern InjectQueue inject_queue;
