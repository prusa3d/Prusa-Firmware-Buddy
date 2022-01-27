#pragma once

#include "types.h"

#include <string_view>

// Why does FILE_PATH_BUFFER_LEN lives in *gui*!?
#include "../../src/gui/file_list_defs.h"

namespace nhttp::printer {

// TODO: Figure a way not to taint the server part with specific implementations
class FileInfo {
private:
    char filename[FILE_PATH_BUFFER_LEN];
    bool can_keep_alive;
    bool after_upload;

public:
    FileInfo(const char *filename, bool can_keep_alive, bool after_upload);
    bool want_read() const { return false; }
    bool want_write() const { return true; }
    handler::Step step(std::string_view input, bool terminated_by_client, uint8_t *buffer, size_t buffer_size);
};

}
