#pragma once

#include <option/has_cyphal_logging.h>
#if !HAS_CYPHAL_LOGGING()
    #error
#endif

#include <logging/log.hpp>

#include "cyphal_register.hpp"
#include "cyphal_sender_direct.hpp"
#include "cyphal_timesync.hpp"
#include "cyphal_port_list.hpp"

#include <uavcan/diagnostic/Record_1_1.h>

namespace can::cyphal {

/**
 * @brief Class providing a logging destination for Cyphal Record tooling.
 * Publishes log messages onto the Cyphal network.
 * Use Record::extern_log_event() as log_event_fn.
 */
class Record {
    /// Cyphal sender object
    SenderDirectTraited<uavcan_diagnostic_Record_1_1_Traits> log_sender;

    struct BufferLine {
        std::atomic<bool> blocked = false; ///< True if being used
        std::atomic<bool> filled = false; ///< True when ready to be send
        uavcan_diagnostic_Record_1_1 record = {}; ///< Buffer to store messages
    };

    BufferLine buffers[4]; ///< Buffer to store messages

    size_t lost_counter = 0; ///< Counter for lost messages
    TimeSync *time_sync = nullptr; ///< Time synchronization object

    /// Mutex to handle multiple loggers
    StaticSemaphore_t access_mutex_buffer = {};
    SemaphoreHandle_t access_mutex = xSemaphoreCreateMutexStatic(&access_mutex_buffer);

    static Record *instance; ///< Singleton instance

    /**
     * @brief Log a message over Cyphal network.
     * @param event the log to be sent
     */
    void member_log_event(logging::FormattedEvent *event);

    /**
     * @brief Add one character to the buffer.
     * @param character add this
     * @param arg Record pointer so this function can be static
     */
    static void add_character(char character, [[maybe_unused]] void *arg);

public:
    static constexpr TickType_t SEND_TIMEOUT = 1 * portTICK_PERIOD_MS; ///< Timeout for trying to send a message

    /**
     * @brief Initialize the Cyphal Record logging destination.
     */
    Record();

    ~Record();

    /**
     * @brief Add itself to task and to portlist.
     * @note Does not add port name and type registers (not needed for uavcan types).
     * @param port_list node's object publishing used ports
     */
    void init(PortList &port_list) {
        port_list.add(log_sender);
    }

    /**
     * @brief Set the time synchronization object.
     * Log timestamps should be in network time, use time_sync to convert.
     * @param time_sync_ the time synchronization object
     */
    void set_time_sync(TimeSync *time_sync_ = nullptr) {
        time_sync = time_sync_;
    }

    /**
     * @brief Log a message over Cyphal network.
     * @param event the log to be sent
     */
    static void extern_log_event(logging::FormattedEvent *event);

    /**
     * @brief Get the number of lost messages.
     * @return number of messages that were discarded due to full buffer
     */
    [[nodiscard]] size_t get_lost() const {
        return lost_counter;
    }

    /// @brief Get the publisher port-ID.
    [[nodiscard]] CanardPortID get_publisher_id() const {
        return log_sender.get_port_id();
    }

    /**
     * @brief Try to send a message from the buffer.
     */
    void try_send();
};

} // namespace can::cyphal

extern "C" {
/**
 * @brief Send log over Cyphal Record tooling.
 * @note Wrapper to be used from C code.
 * @warning Do not use from cyphal task or callbacks.
 * @param event the log to be sent
 */
void cyphal_log_event(logging::FormattedEvent *event);
}
