#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>

#include <canard.h>
#include "cyphal_proto_suber.hpp"
#include "cyphal_task.hpp"

#include <FreeRTOS.h>
#include <semphr.h>
#include <bsod/bsod.h>

namespace can::cyphal {

/**
 * @brief Untemplated suber call to save on codesize.
 * @note This uses void to send data, so it is dangerous. Do not use without the templated face.
 */
class SuberCallVoid : public ProtoSuber {
    /**
     * @brief Callback when message, request or response is received.
     * @param transfer received transfer, needs to be deserialized
     * @param buffer buffer to deserialize the data
     */
    void raw_callback(const CanardRxTransfer &transfer, uint8_t buffer[MAX_DATA_SIZE]) override {
        size_t buffer_size_bytes = transfer.payload_size;
        if (deserialize(buffer, reinterpret_cast<uint8_t *>(transfer.payload), &buffer_size_bytes) == 0) {
            void_callback(buffer,
                {
                    .timestamp = transfer.timestamp_usec,
                    .remote_node_id = transfer.metadata.remote_node_id,
                    .transfer_id = transfer.metadata.transfer_id,
                    .priority = transfer.metadata.priority,
                });
        }
    }

protected:
    SuberCallVoid(CanardPortID port_id, size_t extent, CanardTransferKind kind, CanardMicrosecond timeout)
        : ProtoSuber(kind, port_id, extent, timeout) {}

    /**
     * @brief Callback when message, request or response is received.
     * @param data received data
     * @param meta metadata of the received transfer
     */
    virtual void void_callback(const void *const data, const Meta &meta) = 0;

    /// @brief Deserialize data from buffer.
    virtual int8_t deserialize(void *const data, const uint8_t *buffer, size_t *const inout_buffer_size_bytes) = 0;
};

/**
 * @brief Subscription for a message, request or response. This one uses callback and doesn't store the data.
 * All parameters are transpiled from DSDL and found inside the same generated file.
 * @tparam T data type, looks like "module_submodule_MessageType_1_0"
 * @tparam EXTENT size of the buffer to receive serialized data, looks "like module_submodule_MessageType_1_0_EXTENT_BYTES_"
 * @note Constructor parameter "fn_": function to deserialize the data, looks like "module_submodule_MessageType_1_0_deserialize_"
 */
template <typename T, size_t EXTENT>
class SuberCall : public SuberCallVoid {
public:
    static_assert(MAX_DATA_SIZE >= sizeof(T), "Increase size of buffer for the data structure!");

    /// Function to deserialize the data
    using DeserializeFn = int8_t(T *const obj, const uint8_t *buffer, size_t *const inout_buffer_size_bytes);

    /**
     * @brief Callback type when message, request or response is received.
     * @note It is called from the CAN thread. Do not use this callback for heavy processing.
     * @param data received data
     * @param meta metadata of the received transfer
     */
    using Callback = std::function<void(const T &data, const Meta &meta)>;

private:
    DeserializeFn &deserialize_fn; ///< Function to deserialize the data
    Callback callback; ///< Callback to be called when data is received

    /// Called when message, request or response is received.
    void void_callback(const void *const data, const Meta &meta) {
        callback(*reinterpret_cast<const T *>(data), meta);
    }

    /// Function to deserialize the data
    int8_t deserialize(void *const data, const uint8_t *buffer, size_t *const inout_buffer_size_bytes) {
        return deserialize_fn(reinterpret_cast<T *>(data), buffer, inout_buffer_size_bytes);
    }

public:
    /**
     * @brief Object that can be subscribed and handles deserialization.
     * @note After creation, you must call add_to_task() to add itself to Cyphal Task.
     *
     * @param deserialize_fn_ function to deserialize the data, looks like "module_submodule_MessageType_1_0_deserialize_"
     * @param callback_ callback to be called when data is received
     *    @note Callback is called from the CAN thread. Do not use this callback for heavy processing.
     *    @note You can use only one SenderDirect::send_data() during the callback.
     *
     * @param port_id Cyphal message port-ID
     * @param kind_ message, request or response
     * @param timeout timeout for message, this applies to multipart messages that arrive far apart
     */
    SuberCall(DeserializeFn &deserialize_fn_, CanardPortID port_id, const Callback callback_,
        CanardTransferKind kind = CanardTransferKindMessage, CanardMicrosecond timeout = ProtoSuber::multipart_timeout_default)
        : SuberCallVoid(port_id, EXTENT, kind, timeout)
        , deserialize_fn(deserialize_fn_)
        , callback(callback_) {
        debug_assert(callback);
    }
};

template <typename Traits>
using SuberCallTraitedBase = SuberCall<typename Traits::Type, Traits::extent_bytes>;

template <typename Traits, CanardPortID port_id = Traits::fixed_port_id>
class SuberCallTraited : public SuberCallTraitedBase<Traits> {

public:
    SuberCallTraited(const SuberCall<typename Traits::Type, Traits::extent_bytes>::Callback callback_,
        CanardTransferKind kind = CanardTransferKindMessage, CanardMicrosecond timeout = ProtoSuber::multipart_timeout_default)
        : SuberCallTraitedBase<Traits>(*Traits::deserialize, port_id, callback_, kind, timeout) {
        if constexpr (Traits::has_fixed_port_id) {
            static_assert(port_id == Traits::fixed_port_id);
        }
    }
};

} // namespace can::cyphal
