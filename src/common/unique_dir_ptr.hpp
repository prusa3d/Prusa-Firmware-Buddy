/// RAII on DIR*

#pragma once

#include <memory>
#include <sys/types.h>
#include <dirent.h>

class DirDeleter {
public:
    void operator()(DIR *d) {
        closedir(d);
    }
};

using unique_dir_ptr = std::unique_ptr<DIR, DirDeleter>;
