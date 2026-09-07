#pragma once

#include <cstddef>
#include <cstdint>
#include <concepts>
#include <utils/byte_utils.hpp>

namespace i2c {
using Address = uint8_t;

template <typename HWImpl>
concept I2cBus = requires(HWImpl impl, Address address, size_t offset, Bytes tx_buf, WritableBytes rx_buf, uint32_t us) {
    { impl.write_memory(address, offset, tx_buf) } -> std::same_as<bool>;
    { impl.read_memory(address, offset, rx_buf) } -> std::same_as<bool>;
    { impl.raw_transmit(address, tx_buf) } -> std::same_as<bool>;
    { impl.raw_receive(address, rx_buf) } -> std::same_as<bool>;
    { impl.delay_us(us) } -> std::same_as<void>;
};
} // namespace i2c
