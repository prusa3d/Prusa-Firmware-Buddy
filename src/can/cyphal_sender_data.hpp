#pragma once

#include <cstddef>
#include <cstdint>

#include <canard.h>
#include "cyphal_proto_sender.hpp"
#include "cyphal_task.hpp"

#include <FreeRTOS.h>
#include <semphr.h>
#include <inplace_function.hpp>
#include <bsod/bsod.h>

namespace can::cyphal {

/**
 * @brief Return from SenderData::TransformFunction.
 * @note Excluded from SenderData to be used without templating.
 */
struct TransformResult {
    bool success; ///< True if transformer ended successfully
    bool significant; ///< True if this is a significant change and data should be sent right away
};

/**
 * @brief Sender that holds data, serializes and sends message.
 * All parameters are transpiled from DSDL and found inside the same generated file.
 * @tparam T data type, looks like "module_submodule_MessageType_1_0"
 * @tparam SIZE size of the buffer to hold serialized data, looks "like module_submodule_MessageType_1_0_SERIALIZATION_BUFFER_SIZE_BYTES_"
 * @note Constructor parameter "fn_": function to serialize the data, looks like "module_submodule_MessageType_1_0_serialize_"
 */
template <typename T, size_t SIZE>
class SenderData : public ProtoSenderPeriodic {
public:
    static_assert(MAX_SERIALIZED_SIZE_BYTES >= SIZE, "Increase size of buffer for the serialized data!");

    /// Function to serialize the data
    using SerializeFn = int8_t(const T *const obj, uint8_t *const buffer, size_t *const inout_buffer_size_bytes);

private:
    SerializeFn &serialize_fn; ///< Function to serialize the data
    T data; ///< Data to be sent

public:
    /**
     * @brief Object that holds message data and handles periodic publication.
     * @note After creation, you must call add_to_task() to add itself to Cyphal Task.
     *
     * @param data_ data to be published
     * @param serialize_fn_ function to serialize the data, looks like "module_submodule_MessageType_1_0_serialize_"
     * @param port_id Cyphal message port-ID
     * @param period period of sending
     * @param timeout timeout to transmit, discard if it gets stuck in queue for this long
     * @param priority Cyphal priority of the message
     */
    SenderData(const T &data_, SerializeFn &serialize_fn_, CanardPortID port_id,
        CanardMicrosecond period = 0, CanardMicrosecond timeout = ProtoSender::send_timeout_default, CanardPriority priority = CanardPriorityNominal)
        : ProtoSenderPeriodic(port_id, period, timeout, priority)
        , serialize_fn(serialize_fn_)
        , data(data_) {}

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
        [[maybe_unused]] int8_t ret_serialize = serialize_fn(&data, buffer, &buffer_size); // Serialize data
        debug_assert(buffer != nullptr);
        debug_assert(ret_serialize == 0); // Serialization should never fail

        meta.priority = priority;
        meta.transfer_kind = CanardTransferKindMessage;
        meta.port_id = port_id;
        meta.remote_node_id = CANARD_NODE_ID_UNSET;
        meta.transfer_id = mark_sent(timestamp); // Also modify timestamp
    }

    /**
     * @brief Update data and publish them over CAN.
     * @param data new data to be published
     * @param significant true to send data immediately, false to send data on next update period
     * @param timeout timeout to wait for mutex lock
     * @return true if set, false if mutex timeout
     */
    bool set_data(const T &data_, bool significant = true, TickType_t timeout = portMAX_DELAY) {
        auto lock = cyphal_task.lock_senders_mutex(timeout);
        if (lock.is_locked() == false) {
            return false;
        }

        data = data_;
        if (significant) {
            dirty = true;
        }

        return true;
    }

    /**
     * @brief Get data that are to be sent.
     * @param mutex_timeout timeout to wait for mutex lock
     * @return data or nullopt if timeout
     */
    [[nodiscard]] std::optional<T> get_data(TickType_t mutex_timeout) {
        auto lock = cyphal_task.lock_senders_mutex(mutex_timeout);
        if (lock.is_locked() == false) {
            return std::nullopt;
        }
        return data;
    }

    /**
     * @brief Get data that are to be sent.
     * @return data
     */
    [[nodiscard]] T get_data() {
        auto lock = cyphal_task.lock_senders_mutex();
        debug_assert(lock.is_locked());
        return data;
    }

    /**
     * @brief Transform data using a function.
     * @param data reference to modifiable data
     * @return success of transformation and whether this was a significant change
     */
    using TransformFunction = stdext::inplace_function<TransformResult(T &data)>;

    /**
     * @brief Transform data using a function.
     * @param transformer use this function to modify data, return true if this is a significant change
     * @param timeout timeout to wait for mutex lock
     * @return if transformer run, return its success, otherwise return false on mutex timeout
     */
    bool transform_data(TransformFunction transformer, TickType_t timeout = portMAX_DELAY) {
        auto lock = cyphal_task.lock_senders_mutex(timeout);
        if (lock.is_locked() == false) {
            return false;
        }

        TransformResult res = transformer(data);
        if (res.significant) {
            dirty = true;
        }

        return res.success;
    }
};

template <typename Traits>
using SenderDataTraitedBase = SenderData<typename Traits::Type, Traits::serialization_buffer_size_bytes>;

template <typename Traits, CanardPortID port_id = Traits::fixed_port_id>
class SenderDataTraited : public SenderDataTraitedBase<Traits> {

public:
    SenderDataTraited(CanardMicrosecond period = 0, CanardMicrosecond timeout = ProtoSender::send_timeout_default, CanardPriority priority = CanardPriorityNominal)
        : SenderDataTraitedBase<Traits>({}, *Traits::serialize, port_id, period, timeout, priority) {
        if constexpr (Traits::has_fixed_port_id) {
            static_assert(port_id == Traits::fixed_port_id);
        }
    }
};

} // namespace can::cyphal
