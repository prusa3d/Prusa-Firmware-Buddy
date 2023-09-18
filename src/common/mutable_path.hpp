#pragma once
#include <string.h>

extern "C" size_t strlcat(char *, const char *, size_t);
extern "C" size_t strlcpy(char *dst, const char *src, size_t dsize);

class MutablePath {
private:
    static constexpr size_t BUFFER_SIZE = 104; // TODO: Provide proper defines of max path length
    char path[BUFFER_SIZE];

public:
    MutablePath() {
        set("");
    }

    MutablePath(const char *path) {
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

    static constexpr size_t maximum_length() {
        return sizeof(path);
    }
};
