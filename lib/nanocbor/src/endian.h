#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint64_t nanocbor_be64toh(uint64_t val);
uint64_t nanocbor_htobe64(uint64_t val);
uint32_t nanocbor_htobe32(uint32_t val);

#ifdef __cplusplus
}
#endif
