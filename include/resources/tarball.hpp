/// @file
#pragma once

#include <cstddef>
#include <cstdint>
#include <inplace_function.hpp>
#include <span>

namespace buddy::resources::tarball {

inline constexpr size_t block_size = 512;

/// Hook called for each chunk of data read by extract(), in order
/// and exactly once.
using ExtractDataHook = stdext::inplace_function<void(std::span<const uint8_t>)>;

/// Extract the ustar payload into /system/res.
/// File descriptor must be positioned at the payload.
/// Caller provides a scratch buffer; its size must be a non-zero
/// multiple of block_size.
[[nodiscard]] bool extract(int fd, uint32_t length, std::span<uint8_t> buffer, const ExtractDataHook &on_data);

} // namespace buddy::resources::tarball
