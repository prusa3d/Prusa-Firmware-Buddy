/// @file
#include "modbus.hpp"

#include <cstdlib>

using Status = modbus::Callbacks::Status;

static constexpr const std::byte read_holding_registers { 0x03 };
static constexpr const std::byte read_input_registers { 0x04 };
static constexpr const std::byte write_multiple_registers { 0x10 };

static constexpr std::byte modbus_byte_lo(uint16_t value) {
    return std::byte(value & 0xff);
}

static constexpr std::byte modbus_byte_hi(uint16_t value) {
    return std::byte(value >> 8);
}

namespace modbus {

uint16_t compute_crc(std::span<const std::byte> bytes) {
    uint16_t crc = 0xffff;
    for (auto byte : bytes) {
        crc ^= uint16_t(byte);
        for (int bit = 0; bit < 8; ++bit) {
            if (crc & 1) {
                crc >>= 1;
                crc ^= 0xa001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

std::span<std::byte> handle_transaction(
    Callbacks &callbacks,
    std::span<const std::byte> request,
    std::span<std::byte> response_buffer) {

    if (request.size() < 4 || modbus::compute_crc(request) != 0) {
        return {};
    }

    auto response = response_buffer.begin();
    const auto resp_end = response_buffer.end();
    auto resp = [&](std::byte b) {
        if (response < resp_end) {
            *response++ = b;
        } else {
            // Short output buffer. Do we have a better strategy? As this is a
            // result of a mismatch between our buffer (the extboard FW) and
            // the printer, programmer needs to fix it, it's not some kind of
            // external condition...
            abort();
        }
    };

    // have at least device, function and valid crc
    const auto device = request[0];
    const auto function = request[1];
    request = request.subspan(2, request.size() - 4);

    resp(device);
    resp(function);

    auto status = Status::Ok;

    auto get_word = [&](size_t offset) {
        uint16_t high = static_cast<uint8_t>(request[offset]);
        uint16_t low = static_cast<uint8_t>(request[offset + 1]);
        return high << 8 | low;
    };

    switch (function) {
    case read_holding_registers:
    case read_input_registers: {
        if (request.size() != 4) {
            return {};
        } else {
            const uint16_t address = get_word(0);
            const uint16_t count = get_word(2);

            const uint8_t bytes = 2 * count;
            resp(std::byte { bytes });
            for (uint16_t i = 0; i < count; ++i) {
                uint16_t value;
                status = callbacks.read_register((uint8_t)device, address + i, value);
                if (status != Status::Ok) {
                    break;
                }
                resp(modbus_byte_hi(value));
                resp(modbus_byte_lo(value));
            }
        }
    } break;
    case write_multiple_registers: {
        if (request.size() < 5) {
            return {};
        } else {
            const uint16_t address = get_word(0);
            const uint16_t count = get_word(2);
            const uint8_t bytes = (uint8_t)request[4];
            request = request.subspan(5);
            if (request.size() < bytes || bytes < count * 2) {
                // Incomplete message.
                return {};
            }

            for (uint16_t i = 0; i < count; ++i) {
                const uint16_t value = get_word(0);
                request = request.subspan(2);
                status = callbacks.write_register((uint8_t)device, address + i, value);
                if (status != Status::Ok) {
                    break;
                }
            }
            resp(modbus_byte_hi(address));
            resp(modbus_byte_lo(address));
            resp(modbus_byte_hi(count));
            resp(modbus_byte_lo(count));
        }
    } break;
    default:
        status = Status::IllegalFunction;
    }

    switch (status) {
    case Status::Ok:
        // Everything is set up for being sent.
        break;
    case Status::Ignore:
        // Do _not_ send any answer. Just throw it away.
        return {};
    default:
        response_buffer[1] |= std::byte { 0x80 };
        response = response_buffer.begin() + 2;
        resp(std::byte { static_cast<uint8_t>(status) });
    }

    uint16_t crc = compute_crc(std::span { response_buffer.begin(), response });
    resp(modbus_byte_lo(crc));
    resp(modbus_byte_hi(crc));
    return { response_buffer.begin(), response };
}

} // namespace modbus
