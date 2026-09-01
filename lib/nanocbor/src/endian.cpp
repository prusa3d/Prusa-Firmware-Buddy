
extern "C" {
#include "endian.h"
}

#include <bit>

extern "C" uint64_t nanocbor_be64toh(uint64_t val)
{
    static_assert(std::endian::native == std::endian::little);
    return std::byteswap(val);
}

extern "C" uint64_t nanocbor_htobe64(uint64_t val)
{
    static_assert(std::endian::native == std::endian::little);
    return std::byteswap(val);
}

extern "C" uint32_t nanocbor_htobe32(uint32_t val)
{
    static_assert(std::endian::native == std::endian::little);
    return std::byteswap(val);
}
