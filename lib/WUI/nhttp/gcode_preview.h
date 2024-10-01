#pragma once

#include "step.h"
#include <gcode/gcode_thumb_decoder.h>
#include <gcode/gcode_reader_any.hpp>
#include <unique_file_ptr.hpp>

#include <http/types.h>

#include <cstdio>
#include <string_view>
#include <memory>

namespace nhttp::printer {

class GCodePreview {
private:
    AnyGcodeFormatReader gcode;
    std::optional<uint32_t> etag;
    bool headers_sent = false;
    bool can_keep_alive;
    bool json_errors;
    bool etag_matches = false;
    uint16_t width;
    uint16_t height;
    bool allow_larger;

public:
    GCodePreview(AnyGcodeFormatReader f, const char *path, bool can_keep_alive, bool json_errors, uint16_t width, uint16_t height, bool allow_larger, uint32_t if_none_match);
    bool want_read() const { return false; }
    bool want_write() const { return true; }
    void step(std::string_view input, bool terminated_by_client, uint8_t *buffer, size_t buffer_size, handler::Step &out);
};

} // namespace nhttp::printer
