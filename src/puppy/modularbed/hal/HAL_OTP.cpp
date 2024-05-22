#include "PuppyConfig.hpp"
#include "hal/HAL_OTP.hpp"

namespace hal::OTPDriver {

#define OTP_MEMORY_ADDRESS 0x1FFF7000
#define OTP_MEMORY_SIZE    0x400

bool ReadOTPMemory(uint32_t addressOffset, void *pBuffer, size_t byteCount) {
    if (addressOffset > OTP_MEMORY_SIZE || byteCount > OTP_MEMORY_SIZE || (addressOffset + byteCount) > OTP_MEMORY_SIZE) {
        return false;
    }

    if (pBuffer == nullptr) {
        return false;
    }

    void *pMemory = (void *)(OTP_MEMORY_ADDRESS + addressOffset);
    memcpy(pBuffer, pMemory, byteCount);

    return true;
}

} // namespace hal::OTPDriver
