/// @file
#pragma once

#include <cstddef>
#include <utils/byte_utils.hpp>

namespace spi {

template <typename T>
concept Device = requires(T t) {
    { t.transmit_receive(Bytes {}, WritableBytes {}) } -> std::same_as<bool>;
};

} // namespace spi
