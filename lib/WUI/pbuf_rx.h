#pragma once

#include "pbuf.h"

#ifdef __cplusplus
extern "C" {
#endif

struct pbuf *pbuf_alloc_rx(u16_t length);

#ifdef __cplusplus
}
#endif
