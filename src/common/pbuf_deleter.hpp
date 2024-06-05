#pragma once

#include <lwip/pbuf.h>

class PbufDeleter {
public:
    void operator()(pbuf *buff) {
        pbuf_free(buff);
    }
};
