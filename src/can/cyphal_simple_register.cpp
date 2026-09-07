#include "cyphal_simple_register.hpp"
#include <cstring>
#include <utility>
#include <bsod/bsod.h>

namespace can::cyphal {

size_t RegisterVariantUtils::rv_sizeof(const RegisterVariant &rv) {
    return std::visit(
        Overloaded {
            []<typename T>(const T &) { return sizeof(T); },
            [](std::monostate) { return 0uz; } },
        rv);
}

RegisterVariantUtils::Type RegisterVariantUtils::rv_type(const RegisterVariant &rv) {
    return std::visit(
        Overloaded {
            [](std::monostate) { return Type::EMPTY; },
            [](bool) { return Type::BIT; },
            [](int32_t) { return Type::INT32; },
            [](int16_t) { return Type::INT16; },
            [](int8_t) { return Type::INT8; },
            [](uint32_t) { return Type::NATURAL32; },
            [](uint16_t) { return Type::NATURAL16; },
            [](uint8_t) { return Type::NATURAL8; },
            [](float) { return Type::REAL32; },
        },
        rv);
}

std::optional<RegisterVariant> RegisterVariantUtils::rv_from_buffer(const std::span<const uint8_t> &buffer, Type type) {
    auto handle_one = [&]<typename T>() -> std::optional<RegisterVariant> {
        T tmp;
        if (buffer.size() < sizeof(T)) {
            return std::nullopt;
        }
        std::memcpy(&tmp, buffer.data(), sizeof(T));
        return RegisterVariant { tmp };
    };
    switch (type) {
        using enum Type;
    case EMPTY:
        break;
    case BIT:
        return handle_one.template operator()<bool>();
    case INT32:
        return handle_one.template operator()<int32_t>();
    case INT16:
        return handle_one.template operator()<int16_t>();
    case INT8:
        return handle_one.template operator()<int8_t>();
    case NATURAL32:
        return handle_one.template operator()<uint32_t>();
    case NATURAL16:
        return handle_one.template operator()<uint16_t>();
    case NATURAL8:
        return handle_one.template operator()<uint8_t>();
    case REAL32:
        return handle_one.template operator()<float>();
    }
    return {};
}

bool RegisterVariantUtils::rv_to_buffer(const std::span<uint8_t> &buffer, const RegisterVariant &rv) {
    return std::visit(
        Overloaded {
            [&]<typename T>(const T &value) {
                if (buffer.size() < sizeof(T)) {
                    return false;
                }
                std::memcpy(buffer.data(), &value, sizeof(T));
                return true;
            },
            [&](std::monostate) { return true; } },
        rv);
}

int8_t SimpleRegisterClient::serialize(const SimpleRegisterRequest *const obj, uint8_t *const buffer, size_t *const inout_buffer_size_bytes) {
    debug_assert(obj != nullptr);
    debug_assert(buffer != nullptr);
    debug_assert(inout_buffer_size_bytes != nullptr);

    // Name
    size_t size = std::min(*inout_buffer_size_bytes, static_cast<size_t>(uavcan_register_Name_1_0_SERIALIZATION_BUFFER_SIZE_BYTES_));
    int8_t err = uavcan_register_Name_1_0_serialize_(&obj->name, buffer, &size);
    if (err < 0) {
        return err;
    }

    // Value
    static_assert(SimpleRegisterRequest::value_size == 6, "We need up to 6 bytes to serialize the value");
    buffer[size++] = std::to_underlying(RegisterVariantUtils::rv_type(obj->reg)); // Type
    buffer[size++] = 1; // Element count (1st byte)
    if (std::holds_alternative<bool>(obj->reg)) {
        /// @note 2048 bits can be sent, so bit has 2 length bytes. The rest has only 1 length byte.
        buffer[size++] = 0; // Element count (2nd byte)
    }

    auto rv_buffer = std::span<uint8_t>(&buffer[size], *inout_buffer_size_bytes - size);
    [[maybe_unused]] bool ret = RegisterVariantUtils::rv_to_buffer(rv_buffer, obj->reg);
    debug_assert(ret);
    size += RegisterVariantUtils::rv_sizeof(obj->reg);
    debug_assert(*inout_buffer_size_bytes >= size);
    *inout_buffer_size_bytes = size;

    return 0;
}

int8_t SimpleRegisterClient::deserialize(SimpleRegisterResponse *const obj, const uint8_t *buffer, size_t *const inout_buffer_size_bytes) {
    debug_assert(obj != nullptr);
    debug_assert(buffer != nullptr);
    debug_assert(inout_buffer_size_bytes != nullptr);

    obj->reg = std::nullopt; // Clear response

    // We currently only handle only a single value up to 4 bytes
    // Ignore the first 8 bytes:
    //   7 B - ignore timestamp
    //   1 B - ignore mutable and persistent
    static_assert(SimpleRegisterResponse::extent == 14, "Response can be up to 14 bytes long");
    if (*inout_buffer_size_bytes < 10) {
        // at least 10 bytes are needed to verify the type and size in all cases
        return 0;
    }

    // Register type
    auto type = static_cast<RegisterVariantUtils::Type>(buffer[8]);

    // Index where first value is written
    size_t value_offset = 10;

    if (buffer[9] != 1) { // Element count (1st byte)
        return 0;
    }

    if (type == RegisterVariantUtils::Type::BIT) {
        /// @note 2048 bits can be sent, so bit has 2 length bytes. The rest has only 1 length byte.
        if (buffer[10] != 0) { // Element count (2nd byte)
            return 0;
        }
        value_offset++; // Skip the 2nd length byte
    }

    // Get value
    auto rv_buffer = std::span<const uint8_t>(&buffer[value_offset], *inout_buffer_size_bytes - value_offset);
    obj->reg = RegisterVariantUtils::rv_from_buffer(rv_buffer, type);

    return 0;
}

SimpleRegisterClient::SimpleRegisterClient(CanardMicrosecond send_timeout, CanardMicrosecond multipart_timeout, CanardPriority priority)
    : Client(
        serialize, deserialize,
        uavcan_register_Access_1_0_FIXED_PORT_ID_, CANARD_NODE_ID_UNSET,
        [&](const SimpleRegisterResponse &data, [[maybe_unused]] const ProtoSuber::Meta &meta) {
            response_value = data.reg; // Just store response
        },
        send_timeout, multipart_timeout, priority) {
    debug_assert(call_mutex != nullptr);
}

std::optional<RegisterVariant> SimpleRegisterClient::call(const char *name, const RegisterVariant &reg,
    std::optional<TickType_t> response_timeout, CanardNodeID remote_node_id, TickType_t mutex_timeout, TickType_t tx_timeout) {
    // Request
    SimpleRegisterRequest request;
    request.name.name.count = strnlen(name, sizeof(request.name.name.elements));
    memcpy(request.name.name.elements, name, request.name.name.count);
    request.reg = reg;

    // Lock mutex
    Task::RAIILock lock(call_mutex, mutex_timeout);
    if (lock.is_locked() == false) {
        return std::nullopt; // Mutex timeout
    }

    // Send request and wait for response
    cancel_waiting(); // Cancel any previous waiting
    response_value = std::nullopt; // Clear response to be sure we got one
    if (Client::call(request, remote_node_id, response_timeout, tx_timeout) == ClientCallResult::Received) {
        return response_value;
    }

    return std::nullopt; // No response or Tx timeout
}

bool SimpleRegisterClient::repeat_write(const char *name, const RegisterVariant &reg, size_t attempts,
    CanardNodeID remote_node_id, bool verify, std::optional<TickType_t> response_timeout, TickType_t mutex_timeout, TickType_t tx_timeout) {
    // Request
    SimpleRegisterRequest request;
    request.name.name.count = strnlen(name, sizeof(request.name.name.elements));
    memcpy(request.name.name.elements, name, request.name.name.count);
    request.reg = reg;

    // Lock mutex
    Task::RAIILock lock(call_mutex, mutex_timeout);
    if (lock.is_locked() == false) {
        return false; // Mutex timeout
    }

    // Send request and wait for response
    for (uint32_t attempt = 0; attempt < attempts; attempt++) {
        cancel_waiting(); // Cancel any previous waiting
        response_value = std::nullopt; // Clear response to be sure we got one
        if (Client::call(request, response_timeout, remote_node_id, tx_timeout) == ClientCallResult::Received
            && response_value.has_value() && response_value.value().index() == reg.index() // Check type of the response
            && (!verify || response_value.value() == reg)) { // Check written value
            return true; // Written successfully
        }
    }

    return false; // No response, Tx timeout or read wrong type/value
}

} // namespace can::cyphal
