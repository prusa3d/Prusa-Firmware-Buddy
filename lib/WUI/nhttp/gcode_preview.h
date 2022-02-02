#pragma once

#include "types.h"
#include "../../../src/common/gcode_thumb_decoder.h"

#include <cstdio>
#include <string_view>
#include <memory>

namespace nhttp::printer {

class GCodePreview {
private:
    class FileDeleter {
    public:
        void operator()(FILE *f) {
            fclose(f);
        }
    };
    std::unique_ptr<FILE, FileDeleter> gcode;
    GCodeThumbDecoder decoder;
    bool headers_sent = false;
    bool can_keep_alive;

public:
    GCodePreview(FILE *f, bool can_keep_alive, uint16_t width, uint16_t height);
    bool want_read() const { return false; }
    bool want_write() const { return true; }
    handler::Step step(std::string_view input, bool terminated_by_client, uint8_t *buffer, size_t buffer_size);
};

}
