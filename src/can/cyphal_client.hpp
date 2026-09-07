#pragma once

#include <FreeRTOS.h>
#include <semphr.h>

#include "cyphal_sender_direct.hpp"
#include "cyphal_suber_call.hpp"
#include "cyphal_task.hpp"
#include <bsod/bsod.h>

namespace can::cyphal {

/**
 * @brief Result of call() and repeat_call().
 * @note Outside of the class to avoid template parameters.
 */
enum class ClientCallResult {
    Received = 0, ///< Received response
    TxTimeout = 1, ///< Couldn't transmit in tx_timeout time
    RxTimeout = 2, ///< Didn't receive response in response_timeout time
};

/**
 * @brief Untemplated client to save on codesize.
 * @note This uses void to send data, so it is dangerous. Do not use without the templated face.
 */
class ClientVoid : public ProtoSuber {
    SenderDirectLambda request; ///< Request message

    StaticSemaphore_t response_semaphore_buffer; ///< Buffer for response semaphore
    SemaphoreHandle_t response_semaphore = xSemaphoreCreateBinaryStatic(&response_semaphore_buffer); ///< Semaphore to wait for response

    CanardTransferID transfer_id = 1U << CANARD_TRANSFER_ID_BIT_LENGTH; ///< Transfer-ID of the request and response

    /**
     * @brief Callback when response is received.
     * @param transfer received transfer, needs to be deserialized
     * @param buffer buffer to deserialize the data
     */
    void raw_callback(const CanardRxTransfer &transfer, uint8_t buffer[MAX_DATA_SIZE]) override {
        size_t buffer_size_bytes = transfer.payload_size;
        if (deserialize_response(buffer, reinterpret_cast<uint8_t *>(transfer.payload), &buffer_size_bytes) == 0) {
            if (transfer.metadata.remote_node_id != request.get_remote_node_id() // Wrong remote node-ID
                || transfer.metadata.transfer_id != transfer_id) { // Wrong transfer-ID
                return;
            }

            // Callback to app
            void_callback(buffer,
                {
                    .timestamp = transfer.timestamp_usec,
                    .remote_node_id = transfer.metadata.remote_node_id,
                    .transfer_id = transfer.metadata.transfer_id,
                    .priority = transfer.metadata.priority,
                });
            cancel_waiting(); // Cancel waiting to prevent catching a second response
            xSemaphoreGive(response_semaphore); // Notify waiting thread
        }
    }

protected:
    ClientVoid(CanardPortID port_id, CanardNodeID remote_node_id, size_t extent, CanardMicrosecond send_timeout, CanardMicrosecond multipart_timeout, CanardPriority priority)
        : ProtoSuber(CanardTransferKindResponse, port_id, extent, multipart_timeout)
        , request(
              [this](const void *const data, uint8_t *const buffer, size_t *const size) -> int8_t {
                  return serialize_request(data, buffer, size);
              },
              port_id, CanardTransferKindRequest, remote_node_id, send_timeout, priority) {
        debug_assert(response_semaphore != nullptr);
    }

    /// @brief Serialize request data to buffer.
    virtual int8_t serialize_request(const void *const data, uint8_t *const buffer, size_t *const size) = 0;

    /// @brief Deserialize response data from buffer.
    virtual int8_t deserialize_response(void *const data, uint8_t *const buffer, size_t *const size) = 0;

    /**
     * @brief Callback when response is received.
     * @param data received data
     * @param meta metadata of the received transfer
     */
    virtual void void_callback(const void *const data, const Meta &meta) = 0;

    /**
     * @brief Send request and wait for response.
     *
     * @warning If this timeouts, you can repeat the call,
     *   but you cannot call this 32 times during the sum of timeouts on both sides.
     *   If server uses the same timeouts, it is 2 * get_client_timeout().
     *
     * @param request_data send this
     * @param response_timeout timeout to receive response in RTOS ticks, 0 to not wait, nullopt for default of 2 * get_client_timeout()
     * @param remote_node_id set remote node ID if set, otherwise keep previous
     * @param tx_timeout timeout for transmit mutex, discard if the Cyphal thread is busy for this long
     * @return whether the response was received or not
     *
     * @note Don't mix polling and blocking calls.
     */
    ClientCallResult call_void(const void *const request_data, std::optional<TickType_t> response_timeout, std::optional<CanardNodeID> remote_node_id, TickType_t tx_timeout) {
        xSemaphoreTake(response_semaphore, 0); // Clear semaphore
        transfer_id = request.get_next_transfer_id(); // Remember what transfer ID we are expecting
        if (request.send_data_void(request_data, remote_node_id, tx_timeout) == false) { // Send request
            return ClientCallResult::TxTimeout; // Couldn't send in time
        }

        TickType_t timeout = response_timeout.value_or(pdMS_TO_TICKS(2 * get_client_timeout() / 1000 + 1));
        if (timeout > 0 && wait_response_ready(timeout)) { // Wait for response
            return ClientCallResult::Received;
        } else {
            return ClientCallResult::RxTimeout; // No response in time
        }
    }

public:
    /**
     * @brief Cancel waiting for response.
     */
    void cancel_waiting() {
        // Invalidate transfer ID to prevent catching a response
        transfer_id = 1U << CANARD_TRANSFER_ID_BIT_LENGTH;
    }

    /**
     * @brief Wait for response to be ready.
     * @param timeout timeout to receive response in RTOS ticks
     * @return will return true only once after call() that timeouted
     * @note Don't mix polling and blocking calls. This is useful after call() with response_timeout = 0.
     */
    [[nodiscard]] bool wait_response_ready(TickType_t response_timeout) {
        return (xSemaphoreTake(response_semaphore, response_timeout) == pdTRUE); // Try for response
    }

    /**
     * @brief Check if response is ready.
     * @return will return true only once after call() that timeouted
     * @note Don't mix polling and blocking calls. This is useful after call() with response_timeout = 0.
     */
    [[nodiscard]] inline bool is_response_ready() {
        return wait_response_ready(0);
    }

    /**
     * @brief Get request transmit timeout.
     * @warning This is in microseconds, not RTOS ticks.
     * @return for how long can request get stuck in transmit queue
     */
    [[nodiscard]] CanardMicrosecond get_request_timeout() const {
        return request.get_timeout();
    }

    /**
     * @brief Get response multipart timeout.
     * @warning This is in microseconds, not RTOS ticks.
     * @return for how long can response be partially received
     */
    [[nodiscard]] CanardMicrosecond get_response_timeout() const {
        return get_timeout();
    }

    /**
     * @brief Get timeout for client exchange.
     * @warning This is maximal delay only on this side. The server can have different timeout.
     * @warning This is in microseconds, not RTOS ticks.
     * @return client's timeout in microseconds
     */
    [[nodiscard]] CanardMicrosecond get_client_timeout() const {
        return get_request_timeout() + get_response_timeout();
    }

    /// @return Client port-ID.
    [[nodiscard]] CanardPortID get_client_id() const {
        return request.get_port_id();
    }

    /// @return Request sender for PortList.
    [[nodiscard]] ProtoPortList &get_protoportlist() {
        return request;
    }
};

/**
 * @brief Client object that sends a request and receives a response.
 * All parameters are transpiled from DSDL and found inside the same generated file.
 *
 * Request
 * @tparam T_REQUEST data type, looks like "module_submodule_ServiceType_Request_1_0"
 * @tparam SIZE_REQUEST size of the buffer to hold serialized data, looks "like module_submodule_ServiceType_Request_1_0_SERIALIZATION_BUFFER_SIZE_BYTES_"
 * @note Constructor parameter "fn_request": function to serialize the data, looks like "module_submodule_ServiceType_Request_1_0_serialize_"
 *
 * Response
 * @tparam T_RESPONSE data type, looks like "module_submodule_ServiceType_Response_1_0"
 * @tparam EXTENT_RESPONSE size of the buffer to receive serialized data, looks "like module_submodule_ServiceType_Response_1_0_EXTENT_BYTES_"
 * @note Constructor parameter "fn_response" function to deserialize the data, looks like "module_submodule_ServiceType_Response_1_0_deserialize_"
 */
template <typename T_REQUEST, size_t SIZE_REQUEST,
    typename T_RESPONSE, size_t EXTENT_RESPONSE>
class Client : public ClientVoid {
    SuberCall<T_RESPONSE, EXTENT_RESPONSE>::Callback callback; ///< Callback for user when response is received

    /**
     * @brief Callback when message, request or response is received.
     * @param data received data
     * @param meta metadata of the received transfer
     */
    void void_callback(const void *const data, const Meta &meta) override {
        if (callback) {
            callback(*reinterpret_cast<const T_RESPONSE *>(data), meta);
        }
    }

    /// Function to serialize the request data
    SenderDirect<T_REQUEST, SIZE_REQUEST>::SerializeFn &serialize_request_fn;
    int8_t serialize_request(const void *const data, uint8_t *const buffer, size_t *const size) override {
        return serialize_request_fn(reinterpret_cast<const T_REQUEST *>(data), buffer, size);
    }

    /// Function to deserialize the response data
    SuberCall<T_RESPONSE, EXTENT_RESPONSE>::DeserializeFn &deserialize_response_fn;
    int8_t deserialize_response(void *const data, uint8_t *const buffer, size_t *const size) override {
        return deserialize_response_fn(reinterpret_cast<T_RESPONSE *>(data), buffer, size);
    }

public:
    /**
     * @brief Client object that sends a request and receives a response.
     * @note After creation, you must call add_to_task() to add itself to Cyphal Task.
     *
     * @param serialize_request_fn_ function to serialize the request data, looks like "module_submodule_ServiceType_Request_1_0_serialize_"
     * @param deserialize_response_fn_ function to deserialize the response data, looks like "module_submodule_ServiceType_Response_1_0_deserialize_"
     *
     * @param port_id_ Cyphal message port-ID
     * @param remote_node_id remote node-ID, used for requests and responses
     * @param callback_ callback to be called when response is received
     *    @note Callback is called from the CAN thread. Do not use this callback for heavy processing.
     *    @note You can use only one SenderDirect::send_data() during the callback.
     *
     * @param send_timeout timeout to transmit request, discard if it gets stuck in queue for this long
     *    Default is ProtoSender::send_timeout_default. It should be enough for most cases.
     * @param multipart_timeout timeout for response, this applies to multipart messages that arrive far apart
     *    Default is ProtoSuber::multipart_timeout_default to access Python server or ProtoSuber::multipart_timeout_short to access normal server.
     * @note Roundtrip delay limit and suggested call() timeout is sum of send_timeout and multipart_timeout in both client and server.
     *    If same, then it is "2 * (send_timeout + multipart_timeout)" or "2 * get_client_timeout()".
     * @warning The multipart_timeout_default can get you stuck for too long.
     *    Clients that ask Python server should use only single frame requests and responses.
     *    That way, the multipart_timeout does not matter and only sets the roundtrip delay.
     *    Clients that ask non-Python server should use multipart_timeout_short.
     *    The sum of timeouts should be equal to the timeouts corresponding server.
     *
     * @param priority Cyphal priority of the request
     */
    Client(
        SenderDirect<T_REQUEST, SIZE_REQUEST>::SerializeFn &serialize_request_fn_, SuberCall<T_RESPONSE, EXTENT_RESPONSE>::DeserializeFn &deserialize_response_fn_,
        CanardPortID port_id, CanardNodeID remote_node_id,
        const SuberCall<T_RESPONSE, EXTENT_RESPONSE>::Callback callback_,
        CanardMicrosecond send_timeout,
        CanardMicrosecond multipart_timeout,
        CanardPriority priority = CanardPriorityNominal)
        : ClientVoid(port_id, remote_node_id, EXTENT_RESPONSE, send_timeout, multipart_timeout, priority)
        , callback(callback_)
        , serialize_request_fn(serialize_request_fn_)
        , deserialize_response_fn(deserialize_response_fn_) {
    }

    /**
     * @brief Send request and wait for response.
     *
     * @warning If this timeouts, you can repeat the call,
     *   but you cannot call this 32 times during the sum of timeouts on both sides.
     *   If server uses the same timeouts, it is 2 * get_client_timeout().
     *
     * @param request_data send this
     * @param response_timeout timeout to receive response in RTOS ticks, 0 to not wait, nullopt for default of 2 * get_client_timeout()
     * @param remote_node_id set remote node ID if set, otherwise keep previous
     * @param tx_timeout timeout for transmit mutex, discard if the Cyphal thread is busy for this long
     * @return whether the response was received or not
     *
     * @note Don't mix polling and blocking calls.
     */
    ClientCallResult call(const T_REQUEST &request_data, std::optional<TickType_t> response_timeout = std::nullopt, std::optional<CanardNodeID> remote_node_id = std::nullopt, TickType_t tx_timeout = portMAX_DELAY) {
        return call_void(reinterpret_cast<const void *>(&request_data), response_timeout, remote_node_id, tx_timeout);
    }
};

template <typename Traits>
using ClientTraitedBase = Client<typename Traits::Request::Type, Traits::Request::serialization_buffer_size_bytes, typename Traits::Response::Type, Traits::Response::extent_bytes>;

template <typename Traits, CanardPortID port_id = Traits::fixed_port_id>
class ClientTraited : public ClientTraitedBase<Traits> {

public:
    /**
     * @brief Client object that sends a request and receives a response.
     * @note After creation, you must call add_to_task() to add itself to Cyphal Task.
     *
     * @param remote_node_id remote node-ID, used for requests and responses
     * @param callback_ callback to be called when response is received
     *    @note Callback is called from the CAN thread. Do not use this callback for heavy processing.
     *    @note You can use only one SenderDirect::send_data() during the callback.
     *
     * @param send_timeout timeout to transmit request, discard if it gets stuck in queue for this long
     *    Default is ProtoSender::send_timeout_default. It should be enough for most cases.
     * @param multipart_timeout timeout for response, this applies to multipart messages that arrive far apart
     *    Deafult is ProtoSuber::multipart_timeout_default to access Python server or ProtoSuber::multipart_timeout_short to access normal server.
     * @note Roundtrip delay limit and suggested call() timeout is sum of send_timeout and multipart_timeout in both client and server.
     *    If same, then it is "2 * (send_timeout + multipart_timeout)" or "2 * get_client_timeout()".
     * @warning The multipart_timeout_default can get you stuck for too long.
     *    Clients that ask Python server should use only single frame requests and responses.
     *    That way, the multipart_timeout does not matter and only sets the roundtrip delay.
     *    Clients that ask non-Python server should use multipart_timeout_short.
     *    The sum of timeouts should be equal to the timeouts corresponding server.
     *
     * @param priority Cyphal priority of the request
     */
    ClientTraited(
        CanardNodeID remote_node_id,
        const SuberCall<typename Traits::Response::Type, Traits::Response::extent_bytes>::Callback callback_,
        CanardMicrosecond send_timeout,
        CanardMicrosecond multipart_timeout,
        CanardPriority priority = CanardPriorityNominal)
        : ClientTraitedBase<Traits>(*Traits::Request::serialize, *Traits::Response::deserialize,
            port_id, remote_node_id, callback_, send_timeout, multipart_timeout, priority) {
        if constexpr (Traits::has_fixed_port_id) {
            static_assert(port_id == Traits::fixed_port_id);
        }
    }
};
} // namespace can::cyphal
