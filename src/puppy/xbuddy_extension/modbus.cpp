/// @file
#include "modbus.hpp"

using Status = modbus::Callbacks::Status;

static constexpr const std::byte read_holding_registers { 0x03 };
static constexpr const std::byte read_input_registers { 0x04 };
static constexpr const std::byte write_multiple_registers { 0x10 };

static constexpr uint16_t modbus_compute_crc(std::span<const std::byte> bytes) {
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

static constexpr std::byte modbus_byte_lo(uint16_t value) {
    return std::byte(value & 0xff);
}

static constexpr std::byte modbus_byte_hi(uint16_t value) {
    return std::byte(value >> 8);
}

std::span<std::byte> modbus::handle_transaction(
    Callbacks &callbacks,
    std::span<const std::byte> request,
    std::span<std::byte> response_buffer) {

    if (request.size() < 4 || modbus_compute_crc(request) != 0) {
        return {};
    }

    // have at least device, function and valid crc
    std::byte *response = response_buffer.data();

    const auto device = request[0];
    const auto function = request[1];
    request = request.subspan(2, request.size() - 4);

    *response++ = device;
    *response++ = function;

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
            *response++ = std::byte { bytes };
            for (uint16_t i = 0; i < count; ++i) {
                uint16_t value;
                status = callbacks.read_register((uint8_t)device, address + i, value);
                if (status != Status::Ok) {
                    break;
                }
                *response++ = modbus_byte_hi(value);
                *response++ = modbus_byte_lo(value);
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
            if (request.size() < bytes) {
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
            *response++ = modbus_byte_hi(address);
            *response++ = modbus_byte_lo(address);
            *response++ = modbus_byte_hi(count);
            *response++ = modbus_byte_lo(count);
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
        response = &response_buffer[2];
        *response++ = std::byte { static_cast<uint8_t>(status) };
    }

    uint16_t crc = modbus_compute_crc(std::span { response_buffer.data(), (size_t)std::distance(response_buffer.data(), response) });
    *response++ = modbus_byte_lo(crc);
    *response++ = modbus_byte_hi(crc);
    return { response_buffer.data(), response };
}
