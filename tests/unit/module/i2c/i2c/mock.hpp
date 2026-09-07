#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <variant>
#include <vector>

#include <i2c/base.hpp>
#include <bsod/bsod.h>
#include <utils/byte_utils.hpp>

namespace i2c::mock {

struct RawTransmit {
    i2c::Address address;
    std::vector<std::byte> data;

    bool operator==(const RawTransmit &) const = default;
};

struct RawReceive {
    i2c::Address address;

    bool operator==(const RawReceive &) const = default;
};

struct WriteMemory {
    i2c::Address address;
    uint8_t offset;
    std::vector<std::byte> data;

    bool operator==(const WriteMemory &) const = default;
};

struct ReadMemory {
    i2c::Address address;
    uint8_t offset;

    bool operator==(const ReadMemory &) const = default;
};

struct Delay {
    uint32_t delay_us;

    bool operator==(const Delay &) const = default;
};

using Call = std::variant<RawTransmit, RawReceive, WriteMemory, ReadMemory, Delay>;

struct Response {
    bool success;
    std::vector<std::byte> data;
};

class HWImplMock {
public:
    std::vector<Call> calls;
    std::vector<Response> responses;

    bool raw_transmit(Address address, Bytes buff) {
        calls.push_back(RawTransmit {
            .address = address,
            .data = { buff.begin(), buff.end() },
        });
        debug_assert(!responses.empty());
        const bool success = responses.front().success;
        responses.erase(responses.begin());
        return success;
    }

    bool raw_receive(Address address, WritableBytes buff) {
        calls.push_back(RawReceive {
            .address = address,
        });
        debug_assert(!responses.empty());
        const auto response = std::move(responses.front());
        responses.erase(responses.begin());
        debug_assert(response.data.size() <= buff.size());
        std::copy(response.data.begin(), response.data.end(), buff.begin());
        return response.success;
    }

    bool write_memory(Address address, uint8_t offset, Bytes buff) {
        calls.push_back(WriteMemory {
            .address = address,
            .offset = offset,
            .data = { buff.begin(), buff.end() },
        });
        debug_assert(!responses.empty());
        const bool success = responses.front().success;
        responses.erase(responses.begin());
        return success;
    }

    bool read_memory(Address address, uint8_t offset, WritableBytes buff) {
        calls.push_back(ReadMemory {
            .address = address,
            .offset = offset,
        });
        debug_assert(!responses.empty());
        const auto response = std::move(responses.front());
        responses.erase(responses.begin());
        debug_assert(response.data.size() <= buff.size());
        std::copy(response.data.begin(), response.data.end(), buff.begin());
        return response.success;
    }

    void delay_us(uint32_t us) {
        calls.push_back(Delay {
            .delay_us = us,
        });
    }
};

static_assert(I2cBus<HWImplMock>);

} // namespace i2c::mock
