#pragma once

#include "types.h"

#include <string_view>
#include <dirent.h>
#include <memory>

// Why does FILE_PATH_BUFFER_LEN lives in *gui*!?
#include "../../src/gui/file_list_defs.h"

namespace nhttp::printer {

// TODO: Figure a way not to taint the server part with specific implementations
/**
 * \brief Handler for serving file info and directory listings.
 */
class FileInfo {
private:
    char filename[FILE_PATH_BUFFER_LEN];
    bool can_keep_alive;
    bool after_upload;
    class DirDeleter {
    public:
        void operator()(DIR *d) {
            closedir(d);
        }
    };
    /*
     * Present in case we have an open directory. Not present may mean either
     * that it's a file or that we didn't start listing the directory yet.
     */
    std::unique_ptr<DIR, DirDeleter> dir;
    bool need_comma = false;

public:
    FileInfo(const char *filename, bool can_keep_alive, bool after_upload);
    bool want_read() const { return false; }
    bool want_write() const { return true; }
    handler::Step step(std::string_view input, bool terminated_by_client, uint8_t *buffer, size_t buffer_size);
};

}
