#pragma once

#include "types.h"

#include <optional>
#include <string_view>

namespace nhttp::handler {

class StatusPage {
private:
    const char *extra_content;
    Status status;
    bool can_keep_alive;

public:
    StatusPage(Status status, bool can_keep_alive, const char *extra_content = "")
        : extra_content(extra_content)
        , status(status)
        , can_keep_alive(can_keep_alive) {}
    bool want_read() const { return false; }
    bool want_write() const { return true; }
    Step step(std::string_view input, bool terminated_by_client, uint8_t *output, size_t output_size);
};

}
