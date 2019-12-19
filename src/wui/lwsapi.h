#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "lwip/err.h"

//! Init (start) the LwIP WSAPI server.
err_t lwsapi_init(void);

#ifdef __cplusplus
}
#endif
