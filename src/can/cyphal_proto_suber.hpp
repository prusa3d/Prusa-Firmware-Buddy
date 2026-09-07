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
class ProtoSuber : public ProtoPortList {
public:
    constexpr static size_t MAX_DATA_SIZE = 1024; ///< Size of static buffer for deserializing the data, maximal size of the data structure

    /**
     * @brief Used to group metadata of callback.
     */
    struct Meta {
        CanardMicrosecond timestamp; ///< Timestamp when this was received
        CanardNodeID remote_node_id; ///< Remote node-ID of the sender
        CanardTransferID transfer_id; ///< Transfer-ID useful to pair request and response
        CanardPriority priority; ///< Priority of the transfer
    };

    /**
     * @brief Dummy deserialize function that just gives size 0.
     * This is to be used when the incoming data are empty. In servers and clients.
     * @param out_obj deserialized output object, not used
     * @param buffer buffer to deserialize the data, not used
     * @param inout_buffer_size_bytes size of the buffer, to be set to 0
     * @return 0
     */
    static int8_t dummy_deserialize([[maybe_unused]] uint8_t *const out_obj, [[maybe_unused]] const uint8_t *buffer, size_t *const inout_buffer_size_bytes) {
        debug_assert(inout_buffer_size_bytes != nullptr);
        *inout_buffer_size_bytes = 0;
        return 0;
    }

    static constexpr CanardMicrosecond multipart_timeout_default = 1'000'000; ///< Longer timeout for multipart messages to arrive from Python
    static constexpr CanardMicrosecond multipart_timeout_short = 100'000; ///< Shorter timeout for multipart messages from non-Python devices

protected:
    CanardRxSubscription raw = {}; ///< Canard subscription object

    /// @note These (and port_id) are duplicated in raw, but in there they are managed by libcanard and we cannot change them.
    CanardTransferKind kind; ///< Message, request or response
    size_t extent; ///< Maximum size of serialized data, to be accessed by friend can::Task
    CanardMicrosecond timeout; ///< Timeout for request, this applies to multipart messages that arrive far apart

    /**
     * @brief Callback when message, request or response is received.
     * @note It is called from the CAN thread. Do not use this callback for heavy processing.
     * @param transfer received transfer, needs to be deserialized
     * @param buffer buffer to deserialize the data
     */
    virtual void raw_callback(const CanardRxTransfer &transfer, uint8_t buffer[MAX_DATA_SIZE]) = 0;

    /**
     * @brief Prototype that handles subscription.
     * @param kind_ message, request or response
     * @param port_id Cyphal message port-ID
     * @param extent_ maximum size of serialized data
     * @param timeout_ timeout for message, this applies to multipart messages that arrive far apart
     */
    ProtoSuber(CanardTransferKind kind_, CanardPortID port_id, size_t extent_, CanardMicrosecond timeout_)
        : ProtoPortList(port_id)
        , raw {}
        , kind(kind_)
        , extent(extent_)
        , timeout(timeout_) {
        raw.user_reference = nullptr;
    }

    /// Protected destructor because virtual destructor would cost too much codesize.
    ~ProtoSuber() {
        debug_assert(raw.user_reference == nullptr); // Subscription should be removed from the list before destruction
    }

public:
    /// @brief Add itself to Cyphal Task.
    void add_to_task();

    /// @brief Remove itself from Cyphal Task.
    void remove_from_task();

    /**
     * @brief Get Canard subscription object.
     * @note To be used from Task. Do not change this data manually.
     * @return Canard subscription object
     */
    [[nodiscard]] CanardRxSubscription &get_raw() {
        return raw;
    }

    /**
     * @brief Callback when message, request or response is received.
     * @note This is to be called from Task. Do not use this callback elsewhere.
     * @param subscription Canard subscription object that was the reason to call this
     * @param transfer received transfer, needs to be deserialized
     * @param buffer buffer to deserialize the data
     */
    inline static void static_callback(CanardRxSubscription *subscription, const CanardRxTransfer &transfer, uint8_t buffer[MAX_DATA_SIZE]) {
        debug_assert(subscription != nullptr);
        debug_assert(subscription->user_reference != nullptr);
        reinterpret_cast<ProtoSuber *>(subscription->user_reference)->raw_callback(transfer, buffer);
    }

    /// @return message, request or response
    [[nodiscard]] CanardTransferKind get_kind() const {
        return kind;
    }

    /// @return Maximum size of serialized data, to be accessed by friend can::Task
    [[nodiscard]] size_t get_extent() const {
        return extent;
    }

    /// @return timeout for request, this applies to multipart messages that arrive far apart
    [[nodiscard]] CanardMicrosecond get_timeout() const {
        return timeout;
    }
};

} // namespace can::cyphal
