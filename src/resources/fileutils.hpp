#pragma once
#include <dirent.h>
#include <stdio.h>
#include <cstring>
#include <iterator>
#include "filesystem_littlefs_bbf.h"
#include "bbf.hpp"

#define log(severity, ...) _log_event(severity, log_component_find("Resources"), __VA_ARGS__)

class DIRDeleter {
public:
    void operator()(DIR *dir) {
        closedir(dir);
    }
};

class FILEDeleter {
public:
    void operator()(FILE *file) {
        fclose(file);
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

class Path {
private:
    char path[104]; // TODO: Provide proper defines of max path length
public:
    Path(const char *path) {
        set(path);
    }

    void set(const char *path) {
        strlcpy(this->path, path, sizeof(this->path));
    }

    const char *get() const {
        return path;
    }

    void push(const char *component) {
        size_t length = strlen(path);
        if (length == 0 || path[length - 1] != '/') {
            strlcat(path, "/", sizeof(path));
        }
        strlcat(path, component, sizeof(path));
    }

    void pop() {
        char *slash = strrchr(path, '/');
        if (slash == nullptr) {
            return;
        }
        *slash = 0;
    }

    char *get_buffer() {
        return path;
    }

    static int maximum_length() {
        return sizeof(path);
    }
};
