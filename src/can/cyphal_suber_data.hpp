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
 * @brief Subscription for a message. This one stores the data to be read anytime. This one can receive from multiple sources.
 * @note This allocates the data structure, so use this only for small data that are accessed asynchronously.
 * All parameters are transpiled from DSDL and found inside the same generated file.
 * @tparam T data type, looks like "module_submodule_MessageType_1_0"
 * @tparam EXTENT size of the buffer to receive serialized data, looks "like module_submodule_MessageType_1_0_EXTENT_BYTES_"
 * @tparam SOURCES number of different node-IDs that can send this message
 * @note Constructor parameter "deserialize_fn_": function to deserialize the data, looks like "module_submodule_MessageType_1_0_deserialize_"
 */
template <typename T, size_t EXTENT, size_t SOURCES>
class SuberDataSource : public ProtoSuber {
public:
    static_assert(MAX_DATA_SIZE >= sizeof(T), "Increase size of buffer for the data structure!");
    static_assert(SOURCES > 0, "At least one source is needed!");
    /// Function to deserialize the data
    using DeserializeFn = int8_t(T *const obj, const uint8_t *buffer, size_t *const inout_buffer_size_bytes);

private:
    DeserializeFn &deserialize_fn; ///< Function to deserialize the data
    T data[SOURCES] = {}; ///< Received data
    Meta meta[SOURCES] = {}; ///< Metadata of the above
    std::array<CanardNodeID, SOURCES> source_nodes; ///< Node-IDs that can be received

    /**
     * @brief Callback when message, request or response is received.
     * @param transfer received transfer, needs to be deserialized
     * @param buffer buffer to deserialize the data
     */
    void raw_callback(const CanardRxTransfer &transfer, uint8_t buffer[MAX_DATA_SIZE]) override {
        // Mutex "lock_subers_mutex()" is locked at this time by the CAN thread

        // Find which source
        size_t index = 0;
        for (; index < SOURCES; index++) {
            if (source_nodes[index] == CANARD_NODE_ID_UNSET
                || source_nodes[index] == transfer.metadata.remote_node_id) {
                break;
            }
        }
        if (index < SOURCES) {
            // Deserialize
            size_t buffer_size_bytes = transfer.payload_size;
            T *temporary_data = ::new (buffer) T();
            if (deserialize_fn(temporary_data, reinterpret_cast<uint8_t *>(transfer.payload), &buffer_size_bytes) == 0) {
                // Store
                meta[index].timestamp = transfer.timestamp_usec;
                meta[index].remote_node_id = transfer.metadata.remote_node_id;
                meta[index].transfer_id = transfer.metadata.transfer_id;
                meta[index].priority = transfer.metadata.priority;
                data[index] = *temporary_data; // Extra copy to prevent poisoning data with badly deserialized packet
            }
        }
    }

public:
    /**
     * @brief Object that can be subscribed and handles deserialization of messages.
     * @note After creation, you must call add_to_task() to add itself to Cyphal Task.
     *
     * @param deserialize_fn_ function to deserialize the data, looks like "module_submodule_MessageType_1_0_deserialize_"
     * @param port_id_ Cyphal message port-ID
     * @param source_nodes_ node-IDs that can be received into each data bin,
     *                      CANARD_NODE_ID_UNSET means any node-ID into that bin (needs to be last),
     *                      use CANARD_NODE_ID_MAX + 1 to receive nothing
     * @param timeout_ timeout for message, this applies to multipart messages that arrive far apart
     */
    SuberDataSource(DeserializeFn &deserialize_fn_,
        CanardPortID port_id,
        std::array<CanardNodeID, SOURCES> source_nodes_ = { CANARD_NODE_ID_UNSET }, // Default, catch any source to first bin
        CanardMicrosecond timeout = ProtoSuber::multipart_timeout_default)
        : ProtoSuber(CanardTransferKindMessage, port_id, EXTENT, timeout)
        , deserialize_fn(deserialize_fn_)
        , source_nodes(source_nodes_) {}

    /**
     * @brief Get received data.
     * @param mutex_timeout timeout to wait for mutex lock
     * @param index which data bin to get
     * @return data and metadata or nullopt if timeout
     */
    [[nodiscard]] std::optional<std::pair<T, Meta>> get_data_meta_timeout(TickType_t mutex_timeout, size_t index = 0) {
        debug_assert(index < SOURCES);
        auto lock = cyphal_task.lock_subers_mutex(mutex_timeout);
        if (lock.is_locked() == false) {
            return std::nullopt;
        }
        return std::pair<T, Meta>(data[index], meta[index]);
    }

    /**
     * @brief Get received data.
     * @param index which data bin to get
     * @return data and metadata
     */
    [[nodiscard]] std::pair<T, Meta> get_data_meta(size_t index = 0) {
        debug_assert(index < SOURCES);
        auto lock = cyphal_task.lock_subers_mutex();
        debug_assert(lock.is_locked());
        return std::pair<T, Meta>(data[index], meta[index]);
    }

    /**
     * @brief Get received data.
     * @param mutex_timeout timeout to wait for mutex lock
     * @param index which data bin to get
     * @return data or nullopt if timeout
     */
    [[nodiscard]] std::optional<T> get_data_timeout(TickType_t mutex_timeout, size_t index = 0) {
        debug_assert(index < SOURCES);
        auto lock = cyphal_task.lock_subers_mutex(mutex_timeout);
        if (lock.is_locked() == false) {
            return std::nullopt;
        }
        return data[index];
    }

    /**
     * @brief Get received data.
     * @param index which data bin to get
     * @return data
     */
    [[nodiscard]] T get_data(size_t index = 0) {
        debug_assert(index < SOURCES);
        auto lock = cyphal_task.lock_subers_mutex();
        debug_assert(lock.is_locked());
        return data[index];
    }

    /**
     * @brief Set local data if know from other source.
     * @param data new data
     * @param index which data bin to set
     * @param mutex_timeout timeout to wait for mutex lock
     * @return true if set, false if mutex timeout
     */
    bool set_data(const T &data_, size_t index = 0, TickType_t mutex_timeout = portMAX_DELAY) {
        debug_assert(index < SOURCES);
        auto lock = cyphal_task.lock_subers_mutex(mutex_timeout);
        if (lock.is_locked() == false) {
            return false;
        }

        data[index] = data_;

        return true;
    }

    /**
     * @brief Transform local data if know from other source.
     * @param transformer use this function to modify data
     * @param index which data bin to set
     * @param mutex_timeout timeout to wait for mutex lock
     * @return return from transformer if applied, false if mutex timeout
     */
    [[nodiscard]] bool transform_data(std::function<bool(T &data)> transformer, size_t index = 0, TickType_t mutex_timeout = portMAX_DELAY) {
        debug_assert(index < SOURCES);
        auto lock = cyphal_task.lock_subers_mutex(mutex_timeout);
        if (lock.is_locked() == false) {
            return false;
        }

        bool ret = transformer(data[index]);

        return ret;
    }

    /**
     * @brief Configure node ID for one data bin.
     * @param index which data bin to set
     * @param node_id new node-ID, CANARD_NODE_ID_UNSET means any node-ID, use CANARD_NODE_ID_MAX + 1 to receive nothing
     * @param mutex_timeout timeout to wait for mutex lock
     * @return true if set, false if mutex timeout
     */
    bool set_node_id(size_t index, CanardNodeID node_id, TickType_t mutex_timeout = portMAX_DELAY) {
        debug_assert(index < SOURCES);
        auto lock = cyphal_task.lock_subers_mutex(mutex_timeout);
        if (lock.is_locked() == false) {
            return false;
        }

        source_nodes[index] = node_id;

        return true;
    }
};

template <typename Traits, size_t SOURCES>
using SuberDataSourceTraitedBase = SuberDataSource<typename Traits::Type, Traits::extent_bytes, SOURCES>;

template <typename Traits, size_t SOURCES, CanardPortID port_id = Traits::fixed_port_id>
class SuberDataSourceTraited : public SuberDataSourceTraitedBase<Traits, SOURCES> {

public:
    SuberDataSourceTraited(std::array<CanardNodeID, SOURCES> source_nodes = { CANARD_NODE_ID_UNSET }, // Default, catch any source to first bin
        CanardMicrosecond timeout = ProtoSuber::multipart_timeout_default)
        : SuberDataSourceTraitedBase<Traits, SOURCES>(*Traits::deserialize, port_id, source_nodes, timeout) {
        if constexpr (Traits::has_fixed_port_id) {
            static_assert(port_id == Traits::fixed_port_id);
        }
    }
};

/**
 * @brief Subscription for a message. This one stores the data to be read anytime. This one holds only one copy but receives from any node-ID.
 * @note This allocates the data structure, so use this only for small data that are accessed asynchronously.
 * All parameters are transpiled from DSDL and found inside the same generated file.
 * @tparam T data type, looks like "module_submodule_MessageType_1_0"
 * @tparam EXTENT size of the buffer to receive serialized data, looks "like module_submodule_MessageType_1_0_EXTENT_BYTES_"
 * @note Constructor parameter "deserialize_fn_": function to deserialize the data, looks like "module_submodule_MessageType_1_0_deserialize_"
 */
template <typename T, size_t EXTENT>
using SuberData = SuberDataSource<T, EXTENT, 1>;

template <typename Traits, CanardPortID port_id = Traits::fixed_port_id>
using SuberDataTraited = SuberDataSourceTraited<Traits, 1, port_id>;

} // namespace can::cyphal
