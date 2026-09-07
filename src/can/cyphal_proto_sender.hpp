#pragma once

#include <cstddef>
#include <cstdint>

#include <canard.h>
#include "cyphal_proto_portlist.hpp"

#include <bsod/bsod.h>

namespace can::cyphal {

/**
 * @brief Tooling for Cyphal task.
 */
class ProtoSender : public ProtoPortList {
public:
    constexpr static size_t MAX_SERIALIZED_SIZE_BYTES = 515; ///< Maximal size of serialized data

    /**
     * @brief Dummy serialize function that just gives size 0.
     * This is to be used when the outgoing data are empty. In servers and clients.
     * @param obj object to serialize, not used
     * @param buffer buffer to serialize the data, not used
     * @param inout_buffer_size_bytes size of the buffer, to be set to 0
     * @return 0
     */
    static int8_t dummy_serialize([[maybe_unused]] const uint8_t *const obj, [[maybe_unused]] uint8_t *const buffer, size_t *const inout_buffer_size_bytes);

    static constexpr CanardMicrosecond send_timeout_default = 100'000; ///< Default timeout to transmit

protected:
    CanardPriority priority; ///< Cyphal priority of the message
    CanardMicrosecond timeout; ///< Timeout to transmit, discard if it gets stuck in queue for this long

    /**
     * @brief Prototype that handles periodic publication.
     * @param port_id Cyphal message port-ID
     * @param timeout_ timeout to transmit, discard if it gets stuck in queue for this long
     * @param priority_ Cyphal priority of the message
     */
    ProtoSender(CanardPortID port_id, CanardMicrosecond timeout_, CanardPriority priority_);

    /// Protected destructor because virtual destructor would cost too much codesize.
    ~ProtoSender() = default;

public:
    /**
     * @brief Get data to be sent.
     * @note To be used by Task. Do not use manually, has side effects.
     * @param[inout] timestamp current timestamp in, Tx timeout out, add lifespan to get timeout
     * @param[inout] meta CAN transfer metadata
     * @param[inout] buffer payload buffer
     * @param[inout] buffer_size size of the available buffer in, used payload size out [Bytes]
     */
    virtual void to_send(CanardMicrosecond &timestamp, CanardTransferMetadata &meta, uint8_t *buffer, size_t &buffer_size) = 0;

    /**
     * @brief Get next transfer ID.
     * @param transfer_id transfer ID to increment
     * @return next transfer ID
     */
    [[nodiscard]] static CanardTransferID increment_transfer_id(CanardTransferID transfer_id) {
        return (transfer_id + 1) % (CANARD_TRANSFER_ID_MAX + 1);
    }

    /**
     * @brief Set the Cyphal priority of the message.
     * @param priority_ the Cyphal priority to set
     */
    void set_priority(CanardPriority priority_) {
        priority = priority_;
    }

    /**
     * @brief Get the Cyphal priority of the message.
     * @return the Cyphal priority
     */
    [[nodiscard]] CanardPriority get_priority() const {
        return priority;
    }

    /// @return timeout to transmit, discard if it gets stuck in queue for this long
    [[nodiscard]] CanardMicrosecond get_timeout() const {
        return timeout;
    }
};

/**
 * @brief Tooling for Cyphal task.
 */
class ProtoSenderPeriodic : public ProtoSender {
    ProtoSenderPeriodic *next = nullptr; ///< Next sender in the list of senders

protected:
    CanardMicrosecond period; ///< Period of sending
    CanardMicrosecond last_sent = 0; ///< Last time the message was sent
    CanardTransferID last_transfer_id = 0; ///< Last transfer ID that was sent
    bool dirty = false; ///< True if data are to be sent
    bool added = false; ///< True if this sender is added to the list of senders

    /**
     * @brief Advance metadata when data are sent.
     * @param[inout] timestamp current timestamp in, Tx timeout out, add lifespan to get timeout
     * @return transfer ID for message being sent
     */
    [[nodiscard]] CanardTransferID mark_sent(CanardMicrosecond &timestamp);

    /**
     * @brief Prototype that handles periodic publication of messages.
     * @param port_id Cyphal message port-ID
     * @param period_ period of sending, use 0 to not send periodically
     * @param timeout timeout to transmit, discard if it gets stuck in queue for this long
     * @param priority Cyphal priority of the message
     */
    ProtoSenderPeriodic(CanardPortID port_id, CanardMicrosecond period_, CanardMicrosecond timeout, CanardPriority priority)
        : ProtoSender(port_id, timeout, priority)
        , period(period_) {}

    /// Protected destructor because virtual destructor would cost too much codesize.
    ~ProtoSenderPeriodic() {
        debug_assert(added == false); // Sender should be removed from the list before destruction
    }

public:
    /**
     * @brief Send data now.
     */
    void set_dirty() {
        dirty = true;
    }

    /**
     * @brief Decide if this message is to be sent.
     * @param timestamp current timestamp, usually get_timestamp_us(), parameter to speed up list iteration
     * @return true when due to send
     */
    [[nodiscard]] inline bool is_to_be_sent(const CanardMicrosecond timestamp) const {
        if (period > 0 && timestamp > (last_sent + period)) {
            return true;
        }
        return dirty;
    }

    /**
     * @brief Set publishing period.
     * @param period period of sending, use 0 to not send periodically
     */
    void set_period(CanardMicrosecond period_) {
        period = period_;
    }

    /**
     * @brief Get publishing period.
     * @return period of sending
     */
    [[nodiscard]] CanardMicrosecond get_period() const {
        return period;
    }

    /**
     * @return transfer ID that was last sent
     */
    [[nodiscard]] CanardTransferID get_last_transfer_id() const {
        return last_transfer_id;
    }

    /**
     * @brief Get next sender in the list of senders.
     * @note To be used from Task. Do not change this data manually.
     * @return next sender in the list of senders, can be nullptr if this is the end
     */
    [[nodiscard]] inline ProtoSenderPeriodic *get_next() const {
        return next;
    }

    /**
     * @brief Add this to a linked list.
     * @note To be used from Task. Do not change this data manually.
     * @param next_ next element in the list of senders, can be nullptr if this is the end
     */
    void add_next(ProtoSenderPeriodic *next_) {
        next = next_;
        added = true;
    }

    /**
     * @brief Remove this from linked list.
     */
    void remove_next() {
        next = nullptr;
        added = false;
    }

    /// @brief Add itself to Cyphal Task.
    void add_to_task();

    /// Remove itself from Cyphal Task.
    void remove_from_task();
};

} // namespace can::cyphal
