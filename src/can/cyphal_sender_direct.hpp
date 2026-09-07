#pragma once

#include <cstddef>
#include <cstdint>

#include <canard.h>
#include "cyphal_proto_sender.hpp"
#include "cyphal_task.hpp"

#include <FreeRTOS.h>
#include <semphr.h>
#include <bsod/bsod.h>

namespace can::cyphal {

/**
 * @brief Untemplated direct sender to save on codesize.
 * @note This uses void to send data, so it is dangerous. Do not use without the templated face.
 */
class SenderDirectVoid : public ProtoSender {
protected:
    CanardNodeID remote_node_id; ///< Remote node-ID, used for requests
    CanardTransferID next_transfer_id = 0; ///< Next transfer ID that will be sent

    const void *data = nullptr; ///< Data to be sent, valid only during send_data() function

public:
    /**
     * @brief Get data to be sent.
     * @note To be used by Task. Do not use manually, has side effects.
     * @param[inout] timestamp current timestamp in, Tx timeout out, add lifespan to get timeout
     * @param[inout] meta CAN transfer metadata
     * @param[inout] buffer payload buffer
     * @param[inout] buffer_size size of the available buffer in, used payload size out [Bytes]
     * @return return from serialization function - 0 on success, negative on error
     */
    void to_send(CanardMicrosecond &timestamp, CanardTransferMetadata &meta, uint8_t *buffer, size_t &buffer_size) override {
        debug_assert(data != nullptr); // Data should be set before sending
        debug_assert(buffer != nullptr);
        [[maybe_unused]] int8_t ret_serialize = serialize(data, buffer, &buffer_size); // Serialize data
        debug_assert(ret_serialize == 0); // Serialization should never fail

        meta.priority = priority;
        meta.transfer_kind = kind;
        meta.port_id = port_id;
        meta.remote_node_id = remote_node_id;
        meta.transfer_id = next_transfer_id;

        next_transfer_id = increment_transfer_id(next_transfer_id); // Prepare incremented ID for next message
        timestamp += timeout; // Add lifespan to get timeout

        data = nullptr; // Data are no longer needed
    }

protected:
    /**
     * @brief Send data to a specific remote node-ID.
     * @note This uses void to send data, so it is dangerous. Do not use without the templated face.
     * @param data_ data to be sent
     * @param remote_node_id_ remote node-ID
     *    @warning remote_node_id_ is only to be used for CanardTransferKindRequest or CanardTransferKindResponse.
     * @param timeout timeout to wait for mutex lock
     * @return true if put to tx_buffer, false if failed
     * @note This can fail by timeout or when in anonymous mode or if buffer cannot be taken (trying to send 2 things from cyphal callback).
     */
    [[nodiscard]] bool send_data_void(const void *data_, std::optional<CanardNodeID> remote_node_id_, TickType_t timeout) {
        if (remote_node_id_.has_value()) {
            set_remote_node_id(remote_node_id_.value());
        }

        data = data_; // Store pointer to data
        return cyphal_task.directly_send(*this, timeout);
    }

    /// @brief Serialize data to buffer.
    virtual int8_t serialize(const void *data, uint8_t *buffer, size_t *inout_buffer_size_bytes) = 0;

    /**
     * @brief Object that directly sends message, request or response.
     * @note No need to call add_to_task(), this is not registered with Cyphal Task.
     *
     * @param port_id Cyphal message port-ID
     * @param kind message, request or response
     * @param remote_node_id remote node-ID, used for requests and responses
     *
     * @param timeout timeout to transmit, discard if it gets stuck in queue for this long
     * @param priority Cyphal priority of the message
     */
    SenderDirectVoid(CanardPortID port_id,
        CanardTransferKind kind_, CanardNodeID remote_node_id_,
        CanardMicrosecond timeout, CanardPriority priority)
        : ProtoSender(port_id, timeout, priority)
        , remote_node_id(remote_node_id_)
        , kind(kind_) {
        // Remote node-ID should be set only for requests and responses
        debug_assert(kind == CanardTransferKindRequest || kind == CanardTransferKindResponse || remote_node_id == CANARD_NODE_ID_UNSET);
    }

public:
    const CanardTransferKind kind; ///< Message, request or response

    /**
     * @brief Set remote node-ID for requests or responses.
     * @warning This is only to be used for CanardTransferKindRequest or CanardTransferKindResponse.
     * @param remote_node_id_ remote node-ID
     */
    void set_remote_node_id(CanardNodeID remote_node_id_) {
        debug_assert(kind == CanardTransferKindRequest || kind == CanardTransferKindResponse);
        remote_node_id = remote_node_id_;
    }

    /**
     * @brief Get remote node-ID for requests or responses.
     * @return remote node-ID, has to be CANARD_NODE_ID_UNSET for messages
     */
    [[nodiscard]] CanardNodeID get_remote_node_id() const {
        return remote_node_id;
    }

    /**
     * @brief Set transfer ID of the next message to be sent.
     * @warning This is only to be used for CanardTransferKindResponse.
     * @param transfer_id_ transfer ID to set
     */
    void set_transfer_id(CanardTransferID transfer_id_) {
        debug_assert(kind == CanardTransferKindResponse);
        next_transfer_id = transfer_id_;
    }

    /**
     * @return transfer ID of the next message to be sent
     */
    [[nodiscard]] CanardTransferID get_next_transfer_id() const {
        return next_transfer_id;
    }
};

/**
 * @brief We cannot use multiple inheritance, so this takes a lambda instead.
 * @note This is to be used only for embedding in Client and Server.
 */
class SenderDirectLambda final : public SenderDirectVoid {
    /// Function to serialize the data
    using SerializeVoid = std::function<int8_t(const void *const data, uint8_t *const buffer, size_t *const size)>;
    SerializeVoid serialize_fn;

    /// Inherited from SenderDirectVoid
    int8_t serialize(const void *data, uint8_t *buffer, size_t *inout_buffer_size_bytes) override {
        return serialize_fn(data, buffer, inout_buffer_size_bytes);
    }

public:
    SenderDirectLambda(const SerializeVoid serialize_fn_, CanardPortID port_id,
        CanardTransferKind kind_, CanardNodeID remote_node_id_,
        CanardMicrosecond timeout, CanardPriority priority)
        : SenderDirectVoid(port_id, kind_, remote_node_id_, timeout, priority)
        , serialize_fn(serialize_fn_) {
    }

    /// We want to use this function directly from Server
    [[nodiscard]] inline bool send_data_void(const void *data, std::optional<CanardNodeID> remote_node_id, TickType_t timeout) {
        return SenderDirectVoid::send_data_void(data, remote_node_id, timeout);
    }
};

/**
 * @brief Sender that serializes and sends message, request or response.
 * All parameters are transpiled from DSDL and found inside the same generated file.
 * @tparam T data type, looks like "module_submodule_MessageType_1_0"
 * @tparam SIZE size of the buffer to hold serialized data, looks "like module_submodule_MessageType_1_0_SERIALIZATION_BUFFER_SIZE_BYTES_"
 * @note Constructor parameter "fn_": function to serialize the data, looks like "module_submodule_MessageType_1_0_serialize_"
 */
template <typename T, size_t SIZE>
class SenderDirect : public SenderDirectVoid {
public:
    static_assert(MAX_SERIALIZED_SIZE_BYTES >= SIZE, "Increase size of buffer for the serialized data!");

    /// Function to serialize the data
    using SerializeFn = int8_t(const T *const obj, uint8_t *const buffer, size_t *const inout_buffer_size_bytes);

    /**
     * @brief Object that directly sends message, request or response.
     * @note No need to call add_to_task(), this is not registered with Cyphal Task.
     *
     * @param serialize_fn_ function to serialize the data, looks like "module_submodule_MessageType_1_0_serialize_"
     * @param port_id Cyphal message port-ID
     * @param kind message, request or response
     * @param remote_node_id remote node-ID, used for requests and responses
     *
     * @param timeout timeout to transmit, discard if it gets stuck in queue for this long
     * @param priority Cyphal priority of the message
     */
    SenderDirect(SerializeFn &serialize_fn_, CanardPortID port_id,
        CanardTransferKind kind = CanardTransferKindMessage, CanardNodeID remote_node_id = CANARD_NODE_ID_UNSET,
        CanardMicrosecond timeout = ProtoSender::send_timeout_default, CanardPriority priority = CanardPriorityNominal)
        : SenderDirectVoid(port_id, kind, remote_node_id, timeout, priority)
        , serialize_fn(serialize_fn_) {
    }

    /**
     * @brief Send data to a specific remote node-ID.
     * @param data data to be sent
     * @param remote_node_id remote node-ID
     *    @warning remote_node_id_ is only to be used for CanardTransferKindRequest or CanardTransferKindResponse.
     * @param timeout timeout to wait for mutex lock
     * @return true if put to tx_buffer, false if failed
     * @note This can fail by timeout or when in anonymous mode or if buffer cannot be taken (trying to send 2 things from cyphal callback).
     */
    [[nodiscard]] bool send_data(const T &data, std::optional<CanardNodeID> remote_node_id, TickType_t timeout) {
        return send_data_void(reinterpret_cast<const void *>(&data), remote_node_id, timeout);
    }

    /**
     * @brief Send data to a specific remote node-ID.
     * @param data data to be sent
     * @param remote_node_id remote node-ID
     *    @warning remote_node_id_ is only to be used for CanardTransferKindRequest or CanardTransferKindResponse.
     * @note Asserts when in anonymous mode or if buffer cannot be taken (trying to send 2 things from cyphal callback).
     */
    void send_data(const T &data, std::optional<CanardNodeID> remote_node_id = std::nullopt) {
        [[maybe_unused]] bool ret = send_data_void(reinterpret_cast<const void *>(&data), remote_node_id, portMAX_DELAY);
        debug_assert(ret);
    }

private:
    SerializeFn &serialize_fn; ///< Function to serialize the data

    /// Inherited from SenderDirectVoid, call typed function
    int8_t serialize(const void *data, uint8_t *buffer, size_t *inout_buffer_size_bytes) override {
        return serialize_fn(reinterpret_cast<const T *>(data), buffer, inout_buffer_size_bytes);
    }
};

template <typename Traits>
using SenderDirectTraitedBase = SenderDirect<typename Traits::Type, Traits::serialization_buffer_size_bytes>;

template <typename Traits, CanardPortID port_id = Traits::fixed_port_id>
class SenderDirectTraited final : public SenderDirectTraitedBase<Traits> {

public:
    /**
     * @brief Object that directly sends message, request or response.
     * @note No need to call add_to_task(), this is not registered with Cyphal Task.
     *
     * @param kind message, request or response
     * @param remote_node_id remote node-ID, used for requests and responses
     *
     * @param timeout timeout to transmit, discard if it gets stuck in queue for this long
     * @param priority Cyphal priority of the message
     */
    SenderDirectTraited(CanardTransferKind kind = CanardTransferKindMessage, CanardNodeID remote_node_id = CANARD_NODE_ID_UNSET,
        CanardMicrosecond timeout = ProtoSender::send_timeout_default, CanardPriority priority = CanardPriorityNominal)
        : SenderDirectTraitedBase<Traits>(*Traits::serialize, port_id, kind, remote_node_id, timeout, priority) {
        if constexpr (Traits::has_fixed_port_id) {
            static_assert(port_id == Traits::fixed_port_id);
        }
    }
};

} // namespace can::cyphal
