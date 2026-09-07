/// @file
#include <modbus/modbus.hpp>

#include <cstdlib>
#include <crc/crc.hpp>
#include <modbus/modbus_constants.hpp>
#include <bsod/bsod.h>
#include <utils/byte_utils.hpp>

using Status = modbus::Callbacks::Status;

static constexpr const std::byte read_coils { 0x01 };
static constexpr const std::byte read_descrete_inputs { 0x02 };
static constexpr const std::byte read_holding_registers { 0x03 };
static constexpr const std::byte read_input_registers { 0x04 };
static constexpr const std::byte write_coil { 0x05 };
static constexpr const std::byte write_multiple_registers { 0x10 };
static constexpr const std::byte write_coils { 0x0F };

static constexpr std::byte modbus_byte_lo(uint16_t value) {
    return std::byte(value & 0xff);
}

static constexpr std::byte modbus_byte_hi(uint16_t value) {
    return std::byte(value >> 8);
}

namespace modbus {

Dispatch::Dispatch(std::span<Callbacks *> devices)
    : devices { devices } {
}

modbus::Callbacks *Dispatch::get_device(ServerAddress server_address) {
    for (const auto &device : devices) {
        if (device && device->server_address() == server_address) {
            return device;
        }
    }

    return nullptr;
}

uint16_t compute_crc(Bytes bytes) {
    Crc16Modbus crc;
    crc.update(bytes);
    return crc.get();
}

WritableBytes handle_transaction(
    Dispatch &dispatch,
    WritableBytes request,
    WritableBytes response_buffer,
    ComputeCRC compute_crc_fn) {

    if (request.size() < 4 || compute_crc_fn(request) != 0) {
        return {};
    }

    debug_assert(reinterpret_cast<intptr_t>(response_buffer.data()) % alignof(uint16_t) == 0);
    debug_assert(reinterpret_cast<intptr_t>(request.data()) % alignof(uint16_t) == 0);
    std::byte *orig_request = request.data();

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

    const auto device_id = request[0];
    modbus::Callbacks *device_callbacks = dispatch.get_device(static_cast<ServerAddress>(device_id));
    if (!device_callbacks) {
        return {};
    }

    const auto function = request[1];
    request = request.subspan(2, request.size() - 4);

    resp(device_id);
    resp(function);

    auto status = Status::Ok;

    auto get_word = [&](size_t offset) {
        uint16_t high = static_cast<uint8_t>(request[offset]);
        uint16_t low = static_cast<uint8_t>(request[offset + 1]);
        return high << 8 | low;
    };

    switch (function) {
    case modbus::fc::read_holding_registers:
    case modbus::fc::read_input_registers: {
        if (request.size() != 4) {
            return {};
        } else {
            const uint16_t address = get_word(0);
            const uint16_t count = get_word(2);

            const uint8_t bytes = 2 * count;
            resp(std::byte { bytes });

            // Using the response buffer for the data directly, then arranging the endians in-place.
            // This _should_ be OK, as uint8_t/byte is allowed to alias, so we
            // are fine pointing both byte * and uint16_t * into the same
            // place.
            std::byte *out_buffer = response_buffer.data() + (response - response_buffer.begin());
            if (reinterpret_cast<intptr_t>(out_buffer) % alignof(uint16_t) != 0) {
                // Should be OK, there's one extra byte for CRC
                out_buffer++;
            }
            if ((resp_end - response - 1 /*for the ++/CRC above*/) / 2 < count) {
                abort();
            }
            std::span<uint16_t> out(reinterpret_cast<uint16_t *>(out_buffer), count);
            status = device_callbacks->read_registers(address, out);

            if (status != Status::Ok) {
                break;
            }
            for (size_t i = 0; i < count; i++) {
                uint16_t value = out[i];
                resp(modbus_byte_hi(value));
                resp(modbus_byte_lo(value));
            }
        }
    } break;
    case modbus::fc::write_multiple_registers: {
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

            std::span<uint16_t> in(reinterpret_cast<uint16_t *>(orig_request), count);
            // Rearanging bytes in-place in the buffer. Allowed, since:
            // * The input is aligned (we require it in our interface and we have checked).
            // * byte and uint16_t are allowed to alias.
            // * We overwrite only the ones we have already read.
            // * Function return is a sequence point.
            for (size_t i = 0; i < count; ++i) {
                in[i] = get_word(0);
                request = request.subspan(2);
            }
            status = device_callbacks->write_registers(address, in);
            if (status != Status::Ok) {
                break;
            }
            resp(modbus_byte_hi(address));
            resp(modbus_byte_lo(address));
            resp(modbus_byte_hi(count));
            resp(modbus_byte_lo(count));
        }
    } break;
    case modbus::fc::write_coils:
        if (request.size() < 5) {
            return {};
        } else {
            const uint16_t address = get_word(0);
            const uint16_t no_coils = get_word(2);
            const uint8_t bytes = (uint8_t)request[4];

            // 5 comes from lenght of the header of the message
            // request has to be larger than header + bytes
            if (request.size() < static_cast<size_t>(5 + bytes)) {
                return {};
            }
            const auto data = request.subspan(5, bytes);

            status = device_callbacks->write_coils(address, no_coils, data);
            if (status != Status::Ok) {
                break;
            }

            resp(modbus_byte_hi(address));
            resp(modbus_byte_lo(address));
            resp(modbus_byte_hi(no_coils));
            resp(modbus_byte_lo(no_coils));
        }
        break;
    case modbus::fc::write_coil:
        if (request.size() != 4) {
            return {};
        } else {
            const uint16_t address = get_word(0);
            const uint16_t data = get_word(2);

            std::array<std::byte, 1> in;

            if (data == 0x0000) {
                in.at(0) = std::byte { 0x00 };
            } else if (data == 0xFF00) {
                in.at(0) = std::byte { 0x01 };
            } else {
                return {};
            }

            status = device_callbacks->write_coils(address, 1, in);
            if (status != Status::Ok) {
                break;
            }

            resp(modbus_byte_hi(address));
            resp(modbus_byte_lo(address));
            resp(modbus_byte_hi(data));
            resp(modbus_byte_lo(data));
        }
        break;
    case modbus::fc::read_discrete_inputs:
    case modbus::fc::read_coils:
        if (request.size() != 4) {
            return {};
        } else {
            const uint16_t address = get_word(0);
            const uint16_t no_coils = get_word(2);

            if (no_coils == 0) {
                return {};
            }
            const uint8_t bytes = (((no_coils - 1) / 8) + 1);

            resp(std::byte { bytes });

            std::byte *out_buffer = response_buffer.data() + (response - response_buffer.begin());
            WritableBytes out(out_buffer, bytes);
            status = device_callbacks->read_coils(address, no_coils, out);

            if (status != Status::Ok) {
                break;
            }

            response += bytes;
        }
        break;
    default: {
        WritableBytes out { response_buffer.data() + (response - response_buffer.begin()), static_cast<size_t>(response_buffer.end() - response) };
        status = device_callbacks->custom_function(static_cast<uint8_t>(function), request, out);
        if (status == Status::Ok) {
            response += out.size();
        }
    } break;
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

    uint16_t crc = compute_crc_fn(std::span { response_buffer.begin(), response });
    resp(modbus_byte_lo(crc));
    resp(modbus_byte_hi(crc));
    return { response_buffer.begin(), response };
}

Status Callbacks::read_coils(uint16_t, uint16_t, WritableBytes) {
    return Status::IllegalFunction;
}

Status Callbacks::write_coils(uint16_t, uint16_t, Bytes) {
    return Status::IllegalFunction;
}

Status Callbacks::custom_function([[maybe_unused]] uint8_t func_code, [[maybe_unused]] Bytes in, [[maybe_unused]] WritableBytes &out) {
    return Status::IllegalFunction;
}

} // namespace modbus
