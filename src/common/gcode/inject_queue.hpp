#pragma once

#include "inject_queue_actions.hpp"
#include <common/circular_buffer.hpp>
#include <async_job/async_job.hpp>
#include <span>
#include <str_utils.hpp>
#include <expected>
#include <atomic>
#include <file_list_defs.h>

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

    inline bool is_empty() const { return queue.is_empty(); }
    inline bool is_full() const { return queue.is_full(); }

private:
    /**
     *  Buffering a file. This callback is executed in a different thread through AsyncJob worker_job
     */
    static void load_gcodes_from_file_callback(const char *filepath, AsyncJobExecutionControl &control);

    std::atomic<BufferState> buffer_state = BufferState::idle;
    char gcode_stream_buffer[gcode_stream_buffer_size]; //!< buffer for compiling gcode stream from a file
    char filepath_buffer[FILE_PATH_BUFFER_LEN]; //!< buffer for macro gcode filepath
    CircularBuffer<InjectQueueRecord, queue_size> queue; //!< Queue (real size is queue_size - 1)
    AsyncJob worker_job; //!< Used for asynchronous buffering of gcode stream from a file
};

extern InjectQueue inject_queue;
