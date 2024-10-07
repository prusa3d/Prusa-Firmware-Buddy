#pragma once
#include <dirent.h>
#include <stdio.h>
#include <cstring>
#include <iterator>
#include <buddy/filesystem_littlefs_bbf.h>
#include "bbf.hpp"
#include <common/mutable_path.hpp>

class DIRDeleter {
public:
    void operator()(DIR *dir) {
        closedir(dir);
    }
};

class ScopedFileSystemLittlefsBBF {
private:
    int device;

public:
    ScopedFileSystemLittlefsBBF(FILE *bbf, buddy::bbf::TLVType entry)
        : device(filesystem_littlefs_bbf_init(bbf, static_cast<uint8_t>(entry))) {}

    bool mounted() {
        return device != -1;
    }

    ~ScopedFileSystemLittlefsBBF() {
        if (mounted()) {
            filesystem_littlefs_bbf_deinit();
        }
    }
};

using Path = MutablePath;
