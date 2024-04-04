#pragma once

#include <lwip/tcpip.h>

// Even though we are using the „blocking“ variant (eg. not _try), it
// is, at least by reading the code, possible this would consume the
// internal buffers for messages because it allocates that message
// semi-dynamically from a mem pool :-(.
//
// We can't afford to ever lose the callback, but we are allowed to
// block here and the occurence is probably only theoretical, so wait a
// bit if it happens (so the tcpip thread chews through few of the
// messages there and frees something) and retry.
inline void tcpip_callback_nofail(tcpip_callback_fn function, void *ctx) {
    while (tcpip_callback(function, ctx) != ERR_OK) {
        osDelay(10);
    }
}
