#pragma once

#include "inject_queue_actions.hpp"
#include <common/circular_buffer.hpp>
#include <expected>
#include <file_list_defs.h>

class InjectQueue {
public:
    enum class GetGCodeError {
        /// There is no gcode to be injected, you can continue with processing the standard gcode
        empty,
        /// There is gcode to be injected, but it's being loaded and is not available yet
        /// Do not process standard gcode, wait till we finish loading the injected one
        buffering,
        /// Record was discarded, an error occurred during loading
        loading_aborted,

    };

    static constexpr size_t queue_size = 8;

    /// \returns true if the inject queue has nothing in the queue and is not loading anything
    bool is_empty() const;

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

    CircularBuffer<InjectQueueRecord, queue_size> queue; //!< Queue (real size is queue_size - 1)
};

extern InjectQueue inject_queue;
