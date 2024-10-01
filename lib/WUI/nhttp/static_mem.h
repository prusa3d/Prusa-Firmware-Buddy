/**
 * \file
 */
#pragma once

#include "step.h"
#include <http/types.h>

#include <string_view>

namespace nhttp::handler {

/**
 * \brief Handler to send a body from static memory.
 *
 * This sends a complete response where the body is provided as a constant. The
 * data provided is not copied anywhere and it is assumed the data is alive for
 * the whole lifetime of the handler (usually a constant in the program). This
 * allows "embedding" small/static resources directly into the program.
 */
class SendStaticMemory {
private:
    std::string_view data;
    http::ContentType content_type;
    bool can_keep_alive;
    bool headers_sent = false;
    bool is_done() const;
    const char **extra_hdrs;

public:
    SendStaticMemory(std::string_view data, http::ContentType content_type, bool can_keep_alive, const char **extra_hdrs = nullptr)
        : data(data)
        , content_type(content_type)
        , can_keep_alive(can_keep_alive)
        , extra_hdrs(extra_hdrs) {}
    void step(std::string_view input, bool terminated_by_client, uint8_t *buffer, size_t buffer_size, Step &out);
    bool want_write() const { return !is_done(); }
    bool want_read() const { return false; }
};

} // namespace nhttp::handler
