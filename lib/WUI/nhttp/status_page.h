#pragma once

#include "types.h"

#include <optional>

namespace nhttp::handler {

size_t write_headers(uint8_t *buffer, size_t buffer_len, Status status, ContentType content_type, ConnectionHandling handling, std::optional<uint64_t> content_length = std::nullopt, const char **extra_hdrs = nullptr);

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
    Step step(std::string_view input, uint8_t *output, size_t output_size);
};

}
