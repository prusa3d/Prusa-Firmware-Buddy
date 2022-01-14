#pragma once

#include "types.h"

#include <cstdint>
#include <optional>

namespace nhttp {

struct StatusText {
    Status status;
    const char *text;
    const char **extra_hdrs = nullptr;

    static const StatusText &find(Status status);
};

size_t write_headers(uint8_t *buffer, size_t buffer_len, Status status, ContentType content_type, ConnectionHandling handling, std::optional<uint64_t> content_length = std::nullopt, const char **extra_hdrs = nullptr);

ContentType guess_content_by_ext(const char *fname);

}
