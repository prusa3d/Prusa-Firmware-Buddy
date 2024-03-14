#pragma once

#include <cstdint>
#include <cstring>

namespace hal::OTPDriver {

bool ReadOTPMemory(uint32_t addressOffset, void *pBuffer, size_t byteCount);

} // namespace hal::OTPDriver
