#pragma once

#include "types.h"
#include "../../../src/common/gcode_thumb_decoder.h"
#include "unique_file_ptr.hpp"

#include <cstdio>
#include <string_view>
#include <memory>

namespace nhttp::printer {

class GCodePreview {
private:
    unique_file_ptr gcode;
    std::optional<uint32_t> etag;
    GCodeThumbDecoder decoder;
    bool headers_sent = false;
    bool can_keep_alive;
    bool json_errors;
    bool etag_matches = false;

public:
    GCodePreview(FILE *f, const char *path, bool can_keep_alive, bool json_errors, uint16_t width, uint16_t height, uint32_t if_none_match);
    bool want_read() const { return false; }
    bool want_write() const { return true; }
    handler::Step step(std::string_view input, bool terminated_by_client, uint8_t *buffer, size_t buffer_size);
};

}
