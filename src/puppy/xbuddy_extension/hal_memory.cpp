/// @file
#include "hal_memory.hpp"

#include <stm32h5xx.h>
#include <utils/byte_utils.hpp>

const WritableBytes hal::memory::peripheral_address_region(reinterpret_cast<std::byte *>(PERIPH_BASE_NS), 0x10000000);
