#pragma once

#include "types.h"

#include <string_view>

namespace nhttp::handler {

class SendStaticMemory {
private:
    std::string_view data;
    ContentType content_type;
    bool can_keep_alive;
    bool headers_sent = false;
    bool is_done() const;
    const char **extra_hdrs;

public:
    SendStaticMemory(std::string_view data, ContentType content_type, bool can_keep_alive, const char **extra_hdrs = nullptr)
        : data(data)
        , content_type(content_type)
        , can_keep_alive(can_keep_alive)
        , extra_hdrs(extra_hdrs) {}
    Step step(std::string_view input, bool terminated_by_client, uint8_t *buffer, size_t buffer_size);
    bool want_write() const { return !is_done(); }
    bool want_read() const { return false; }
};

}
