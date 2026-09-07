#pragma once

#include "cyphal_client.hpp"
#include "cyphal_proto_suber.hpp"
#include "cyphal_task.hpp"

#include <uavcan/_register/Access_1_0.h>
#include <variant>
#include <span>
#include <utils/overloaded_visitor.hpp>

namespace can::cyphal {

/// Register variant with type and value
using RegisterVariant = std::variant<std::monostate, bool, int32_t, int16_t, int8_t, uint32_t, uint16_t, uint8_t, float>;

namespace RegisterVariantUtils {

    /// Type enumeration, with value matching uavcan.register.Value.1.0 tag index
    enum class Type : uint8_t {
        EMPTY = 0,
        // We don't support STRING
        // We don't support UNSTRUCTURED
        BIT = 3,
        // We don't support INT64
        INT32 = 5,
        INT16 = 6,
        INT8 = 7,
        // We don't support UINT64
        NATURAL32 = 9,
        NATURAL16 = 10,
        NATURAL8 = 11,
        // We don't support REAL64
        REAL32 = 13,
        // We don't support REAL16
    };

    // Get size of type stored in RegisterVariant
    size_t rv_sizeof(const RegisterVariant &rv);

    // Get Type stored in RegisterVariant
    Type rv_type(const RegisterVariant &rv);

    // Load RegisterVariant from buffer
    std::optional<RegisterVariant> rv_from_buffer(const std::span<const uint8_t> &buffer, Type type);

    // Copy RegisterVariant to buffer
    bool rv_to_buffer(const std::span<uint8_t> &buffer, const RegisterVariant &rv);

    static constexpr auto Empty = RegisterVariant();
} // namespace RegisterVariantUtils

struct SimpleRegisterRequest {
    uavcan_register_Name_1_0 name; ///< Name of register
    RegisterVariant reg; ///< Value and type of the register

    static constexpr size_t value_size = 6; ///< Maximum serialized size of the value alone
};

struct SimpleRegisterResponse {
    std::optional<RegisterVariant> reg; ///< Value and type of the register, response not valid if nullopt

    static constexpr size_t extent = 14; ///< Maximum extent size of the simplified response
};

/**
 * @brief Simplified register client.
 * It provides client to register interface that can access only simple registers with single value up to 32 bits.
 * It uses less resources than full register client.
 */
class SimpleRegisterClient : public Client<SimpleRegisterRequest,
                                 uavcan_register_Name_1_0_SERIALIZATION_BUFFER_SIZE_BYTES_ + SimpleRegisterRequest::value_size,
                                 SimpleRegisterResponse, SimpleRegisterResponse::extent> {

    std::optional<RegisterVariant> response_value = std::nullopt; ///< Response obtained from last call

    /// Mutex because we can use this port only one call at a time
    StaticSemaphore_t call_mutex_buffer = {};
    SemaphoreHandle_t call_mutex = xSemaphoreCreateMutexStatic(&call_mutex_buffer);

    /// Custom serialization function that can do only a single value
    static int8_t serialize(const SimpleRegisterRequest *const obj, uint8_t *const buffer, size_t *const inout_buffer_size_bytes);

    /// Custom deserialization function that can do only a single value
    static int8_t deserialize(SimpleRegisterResponse *const obj, const uint8_t *buffer, size_t *const inout_buffer_size_bytes);

public:
    /**
     * @brief Construct a new register client object.
     * @warning Don't forget to add_to_task() after construction.
     *
     * @param send_timeout timeout to transmit request, discard if it gets stuck in queue for this long
     * @param multipart_timeout timeout for response, this applies to multipart messages that arrive far apart
     * @param priority Cyphal priority of the request
     */
    SimpleRegisterClient(
        CanardMicrosecond send_timeout = ProtoSender::send_timeout_default,
        CanardMicrosecond multipart_timeout = ProtoSuber::multipart_timeout_default,
        CanardPriority priority = CanardPriorityNominal);

    /**
     * @brief Write or read a simple register.
     *
     * @warning If this timeouts, you can repeat the call,
     *   but you cannot call this 32 times during the sum of timeouts on both sides.
     *   If server uses the same timeouts, it is 2 * get_client_timeout().
     *
     * @param name register name
     * @param reg value and type of the register to write, RegisterVariantUtils::Empty to read
     * @param response_timeout timeout to receive response in RTOS ticks, 0 to not wait, nullopt for default of 2 * get_client_timeout()
     * @param remote_node_id node id of remote node
     * @param mutex_timeout timeout to lock mutex to prevent concurrent calls of this function
     * @param tx_timeout timeout to transmit request, discard if it gets stuck in queue for this long
     * @return value of register read, nullopt if response not received
     */
    [[nodiscard]] std::optional<RegisterVariant> call(const char *name, const RegisterVariant &reg,
        std::optional<TickType_t> response_timeout, CanardNodeID remote_node_id,
        TickType_t mutex_timeout = portMAX_DELAY, TickType_t tx_timeout = portMAX_DELAY);

    /**
     * @brief Get response value from last call.
     * Useful when called with response timeout 0 together with wait_response_ready().
     */
    [[nodiscard]] std::optional<RegisterVariant> get_response_value() const {
        return response_value;
    }

    /**
     * @brief Read a simple register.
     *
     * @warning If this timeouts, you can repeat the call,
     *   but you cannot call this 32 times during the sum of timeouts on both sides.
     *   If server uses the same timeouts, it is 2 * get_client_timeout().
     *
     * @param name register name
     * @param response_timeout timeout to receive response in RTOS ticks, 0 to not wait, nullopt for default of 2 * get_client_timeout()
     * @param remote_node_id node id of remote node
     * @param mutex_timeout timeout to lock mutex to prevent concurrent calls of this function
     * @param tx_timeout timeout to transmit request, discard if it gets stuck in queue for this long
     * @return value and type of register read, nullopt if response not received
     */
    [[nodiscard]] inline std::optional<RegisterVariant> read(const char *name,
        std::optional<TickType_t> response_timeout, CanardNodeID remote_node_id,
        TickType_t mutex_timeout = portMAX_DELAY, TickType_t tx_timeout = portMAX_DELAY) {
        return call(name, RegisterVariantUtils::Empty, response_timeout, remote_node_id, mutex_timeout, tx_timeout);
    }

    /**
     * @brief Write a simple register.
     *
     * @warning If this timeouts, you can repeat the call,
     *   but you cannot call this 32 times during the sum of timeouts on both sides.
     *   If server uses the same timeouts, it is 2 * get_client_timeout().
     *
     * @param name register name
     * @param reg value and type of the register to write
     * @param response_timeout timeout to receive response in RTOS ticks, 0 to not wait, nullopt for default of 2 * get_client_timeout()
     * @param remote_node_id node id of remote node
     * @param mutex_timeout timeout to lock mutex to prevent concurrent calls of this function
     * @param tx_timeout timeout to transmit request, discard if it gets stuck in queue for this long
     * @return true if written successfully
     */
    [[nodiscard]] inline bool write(const char *name, const RegisterVariant &reg,
        std::optional<TickType_t> response_timeout, CanardNodeID remote_node_id,
        TickType_t mutex_timeout = portMAX_DELAY, TickType_t tx_timeout = portMAX_DELAY) {
        auto ret = call(name, reg, response_timeout, remote_node_id, mutex_timeout, tx_timeout);
        return ret.has_value() && ret == reg;
    }

    /**
     * @brief Write a simple register, try multiple times.
     *
     * @warning If this timeouts, you can repeat the call,
     *   but you cannot use 32 attempts during the sum of timeouts on both sides.
     *   If server uses the same timeouts, it is 2 * get_client_timeout().
     *
     * @param name register name
     * @param reg value and type of the register
     * @param attempts try to write this many times
     * @param remote_node_id node id of remote node
     * @param verify Verify that written value matches the request
     * @param response_timeout timeout to receive response in RTOS ticks, 0 to not wait, nullopt for default of 2 * get_client_timeout()
     * @param mutex_timeout timeout to lock mutex to prevent concurrent calls of this function
     * @param tx_timeout timeout to transmit request, discard if it gets stuck in queue for this long
     * @return true if written successfully
     */
    [[nodiscard]] bool repeat_write(const char *name, const RegisterVariant &reg,
        size_t attempts, CanardNodeID remote_node_id, bool verify = true,
        std::optional<TickType_t> response_timeout = std::nullopt, TickType_t mutex_timeout = portMAX_DELAY, TickType_t tx_timeout = portMAX_DELAY);
};

} // namespace can::cyphal
