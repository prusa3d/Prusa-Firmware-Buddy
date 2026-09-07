#pragma once

#include <canard.h>
#include "cyphal_sender_direct.hpp"
#include "cyphal_proto_suber.hpp"
#include "cyphal_suber_call.hpp"
#include "cyphal_task.hpp"
#include <bsod/bsod.h>

namespace can::cyphal {

/**
 * @brief Untemplated server to save on codesize.
 * @note This uses void to send data, so it is dangerous. Do not use without the templated face.
 */
class ServerVoid : public ProtoSuber {
private:
    SenderDirectLambda response; ///< Response message

    std::atomic<bool> awaiting_response = false; ///< True if response should be sent ASAP

    /**
     * @brief Callback when message, request or response is received.
     * @param transfer received transfer, needs to be deserialized
     * @param buffer buffer to deserialize the data
     */
    void raw_callback(const CanardRxTransfer &transfer, uint8_t buffer[MAX_DATA_SIZE]) override {
        size_t buffer_size_bytes = transfer.payload_size;
        if (deserialize_request(buffer, reinterpret_cast<uint8_t *>(transfer.payload), &buffer_size_bytes) == 0) {
            if (awaiting_response) { // We need to send response to previous request first
                return; // Ignore this request
            }

            // Response with the same meta as request
            response.set_transfer_id(transfer.metadata.transfer_id);
            response.set_remote_node_id(transfer.metadata.remote_node_id);
            response.set_priority(transfer.metadata.priority);

            // Callback to app
            awaiting_response = true;
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
    /**
     * @brief Void version of Server that is not templated.
     * @note This uses void to send data, so it is dangerous. Do not use without the templated face.
     * @note After creation, you must call add_to_task() to add itself to Cyphal Task.
     *
     * @param port_id Cyphal service port-ID
     * @param extent maximum size of serialized data
     * @param send_timeout timeout to transmit response, discard if it gets stuck in queue for this long
     * @param multipart_timeout timeout for request, this applies to multipart messages that arrive far apart
     */
    ServerVoid(CanardPortID port_id, size_t extent, CanardMicrosecond send_timeout, CanardMicrosecond multipart_timeout)
        : ProtoSuber(CanardTransferKindRequest, port_id, extent, multipart_timeout)
        , response(
              [this](const void *const data, uint8_t *const buffer, size_t *const size) -> int8_t {
                  return serialize_response(data, buffer, size);
              },
              port_id, CanardTransferKindResponse, CANARD_NODE_ID_UNSET, send_timeout, CanardPriorityNominal) {}

    /// @brief Deserialize request data from buffer.
    virtual int8_t deserialize_request(void *const data, uint8_t *const buffer, size_t *const size) = 0;

    /// @brief Serialize response data to buffer.
    virtual int8_t serialize_response(const void *const data, uint8_t *const buffer, size_t *const size) = 0;

    /**
     * @brief Callback when request is received.
     * @param data received data
     * @param meta metadata of the received transfer
     */
    virtual void void_callback(const void *const data, const Meta &meta) = 0;

    /**
     * @brief Send response.
     * @param data data to be sent
     * @param timeout timeout to wait for tx_buffer, not needed if responding from the callback
     * @return true if put to tx_buffer, false if failed
     * @note This can fail by timeout or when in anonymous mode or if buffer cannot be taken (trying to send 2 things from cyphal callback).
     */
    [[nodiscard]] bool send_response_void(const void *data, TickType_t timeout) {
        debug_assert(awaiting_response == true); // We need to be responding to something

        bool ret = response.send_data_void(data, std::nullopt, timeout);
        awaiting_response = false;

        return ret;
    }

public:
    /// @return True if response should be sent as soon as possible.
    bool response_needed() const {
        return awaiting_response;
    }

    /// @return Server port-ID.
    [[nodiscard]] inline CanardPortID get_server_id() const {
        return get_port_id();
    }

    /// @return Request suber for PortList.
    [[nodiscard]] inline ProtoPortList &get_protoportlist() {
        return *this;
    }
};

/**
 * @brief Server object that receives a request and immediately sends a response.
 * All parameters are transpiled from DSDL and found inside the same generated file.
 *
 * Request
 * @tparam T_REQUEST data type, looks like "module_submodule_ServiceType_Request_1_0"
 * @tparam EXTENT_REQUEST size of the buffer to receive serialized data, looks "like module_submodule_ServiceType_Request_1_0_EXTENT_BYTES_"
 * @note Constructor parameter "deserialize_request_fn": function to deserialize the data, looks like "module_submodule_ServiceType_Request_1_0_deserialize_".
 *
 * Response
 * @tparam T_RESPONSE data type, looks like "module_submodule_ServiceType_Response_1_0"
 * @tparam SIZE_RESPONSE size of the buffer to hold serialized data, looks "like module_submodule_ServiceType_Response_1_0_SERIALIZATION_BUFFER_SIZE_BYTES_"
 * @note Constructor parameter "serialize_response_fn" function to serialize the data, looks like "module_submodule_ServiceType_Response_1_0_serialize_".
 */
template <typename T_REQUEST, size_t EXTENT_REQUEST,
    typename T_RESPONSE, size_t SIZE_RESPONSE>
class Server : public ServerVoid {

    SuberCall<T_REQUEST, EXTENT_REQUEST>::Callback callback; ///< Callback to be called when request is received

    /**
     * @brief Callback when message, request or response is received.
     * @param data received data
     * @param meta metadata of the received transfer
     */
    void void_callback(const void *const data, const Meta &meta) override {
        if (callback) {
            callback(*reinterpret_cast<const T_REQUEST *>(data), meta);
        }
    }

    /// Function to deserialize the request data
    SuberCall<T_REQUEST, EXTENT_REQUEST>::DeserializeFn &deserialize_request_fn;
    int8_t deserialize_request(void *const data, uint8_t *const buffer, size_t *const size) override {
        return deserialize_request_fn(reinterpret_cast<T_REQUEST *>(data), buffer, size);
    }

    /// Function to serialize the response data
    SenderDirect<T_RESPONSE, SIZE_RESPONSE>::SerializeFn &serialize_response_fn;
    int8_t serialize_response(const void *const data, uint8_t *const buffer, size_t *const size) override {
        return serialize_response_fn(reinterpret_cast<const T_RESPONSE *>(data), buffer, size);
    }

public:
    /**
     * @brief Server object that receives a request and sends a response.
     * @note After creation, you must call add_to_task() to add itself to Cyphal Task.
     *
     * @param deserialize_request_fn_ function to deserialize the request data, looks like "module_submodule_ServiceType_Request_1_0_deserialize_"
     * @param serialize_response_fn_ function to serialize the response data, looks like "module_submodule_ServiceType_Response_1_0_serialize_"
     *
     * @param port_id_ Cyphal service port-ID
     * @param callback_ callback to be called when request is received
     *    @note Callback is called from the CAN thread. Do not use this callback for heavy processing.
     *    @note User has to call send_response() to send the response. It doesn't have to be called from the callback, but should be soon.
     *    @note You can use only one SenderDirect::send_data() during the callback. Calling send_response() counts as the one use of SenderDirect::send_data().
     *
     * @param send_timeout timeout to transmit response, discard if it gets stuck in queue for this long
     *    Default is ProtoSender::send_timeout_default. It should be enough for most cases.
     * @param multipart_timeout timeout for request, this applies to multipart messages that arrive far apart
     *    Deafult is ProtoSuber::multipart_timeout_default to access Python server or ProtoSuber::multipart_timeout_short to access normal server.
     * @note Roundtrip delay limit and suggested call() timeout is sum of send_timeout and multipart_timeout in both client and server.
     *    If same, then it is "2 * (send_timeout + multipart_timeout)" or "2 * get_client_timeout()".
     */
    Server(SuberCall<T_REQUEST, EXTENT_REQUEST>::DeserializeFn &deserialize_request_fn_, SenderDirect<T_RESPONSE, SIZE_RESPONSE>::SerializeFn &serialize_response_fn_,
        CanardPortID port_id, const SuberCall<T_REQUEST, EXTENT_REQUEST>::Callback callback_,
        CanardMicrosecond send_timeout, CanardMicrosecond multipart_timeout)
        : ServerVoid(port_id, EXTENT_REQUEST, send_timeout, multipart_timeout)
        , callback(callback_)
        , deserialize_request_fn(deserialize_request_fn_)
        , serialize_response_fn(serialize_response_fn_) {}

    /**
     * @brief Send response.
     * @param data data to be sent
     * @param timeout timeout to wait for tx_buffer, not needed if responding from the callback
     * @return true if put to tx_buffer, false if failed
     * @note This can fail by timeout or when in anonymous mode or if buffer cannot be taken (trying to send 2 things from cyphal callback).
     */
    [[nodiscard]] bool send_response(const T_RESPONSE &data, TickType_t timeout) {
        return send_response_void(reinterpret_cast<const void *>(&data), timeout);
    }

    /**
     * @brief Send response.
     * @param data data to be sent
     * @note Asserts when in anonymous mode or if buffer cannot be taken (trying to send 2 things from cyphal callback).
     */
    void send_response(const T_RESPONSE &data) {
        [[maybe_unused]] bool ret = send_response(data, portMAX_DELAY);
        debug_assert(ret);
    }
};

template <typename Traits>
using ServerTraitedBase = Server<typename Traits::Request::Type, Traits::Request::extent_bytes, typename Traits::Response::Type, Traits::Response::serialization_buffer_size_bytes>;

template <typename Traits, CanardPortID port_id = Traits::fixed_port_id>
class ServerTraited : public ServerTraitedBase<Traits> {

public:
    /**
     * @brief Server object that receives a request and sends a response.
     * @note After creation, you must call add_to_task() to add itself to Cyphal Task.
     *
     * @param callback_ callback to be called when request is received
     *    @note Callback is called from the CAN thread. Do not use this callback for heavy processing.
     *    @note User has to call send_response() to send the response. It doesn't have to be called from the callback, but should be soon.
     *    @note You can use only one SenderDirect::send_data() during the callback. Calling send_response() counts as the one use of SenderDirect::send_data().
     *
     * @param send_timeout timeout to transmit response, discard if it gets stuck in queue for this long
     *    Default is ProtoSender::send_timeout_default. It should be enough for most cases.
     * @param multipart_timeout timeout for request, this applies to multipart messages that arrive far apart
     *    Deafult is ProtoSuber::multipart_timeout_default to access Python server or ProtoSuber::multipart_timeout_short to access normal server.
     * @note Roundtrip delay limit and suggested call() timeout is sum of send_timeout and multipart_timeout in both client and server.
     *    If same, then it is "2 * (send_timeout + multipart_timeout)" or "2 * get_client_timeout()".
     */
    ServerTraited(const SuberCall<typename Traits::Request::Type, Traits::Request::extent_bytes>::Callback callback_,
        CanardMicrosecond send_timeout, CanardMicrosecond multipart_timeout)
        : ServerTraitedBase<Traits>(*Traits::Request::deserialize, *Traits::Response::serialize, port_id, callback_, send_timeout, multipart_timeout) {
        if constexpr (Traits::has_fixed_port_id) {
            static_assert(port_id == Traits::fixed_port_id);
        }
    }
};

} // namespace can::cyphal
