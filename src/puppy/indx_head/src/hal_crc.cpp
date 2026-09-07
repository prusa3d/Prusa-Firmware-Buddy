/// @file
#include "hal_crc.hpp"

#include <stm32c0xx.h>
#include <utils/byte_utils.hpp>

void hal::crc::init() {
    __HAL_RCC_CRC_CLK_ENABLE();
    CRC->INIT = 0xffff;
    CRC->POL = 0x8005;
}

uint16_t hal::crc::compute_crc16_modbus(Bytes data) {
    CRC->CR = CRC_CR_RESET | CRC_CR_POLYSIZE_0 | CRC_CR_REV_IN_0 | CRC_CR_REV_OUT;
    for (auto byte : data) {
        *(volatile std::byte *)&CRC->DR = byte;
    }
    return CRC->DR;
}
