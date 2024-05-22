#pragma once

// The path length constants live in gui :-(
#include <gui/file_list_defs.h>

#include <array>
#include <optional>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <memory>

template <size_t S>
class Buffer {
private:
    std::array<uint8_t, S> data;
    bool borrowed = false;

public:
    class Borrow {
    private:
        Buffer *buff;
        friend class Buffer<S>;
        Borrow(Buffer *buff)
            : buff(buff) {}

    public:
        Borrow(const Borrow &) = delete;
        Borrow &operator=(const Borrow &) = delete;
        Borrow(Borrow &&other)
            : buff(other.buff) {
            other.buff = nullptr;
        }
        Borrow &operator=(Borrow &&other) {
            if (buff != nullptr) {
                assert(buff->borrowed);
                buff->borrowed = false;
            }
            buff = other.buff;
            other.buff = nullptr;
            return *this;
        }
        ~Borrow() {
            if (buff != nullptr) {
                assert(buff->borrowed);
                buff->borrowed = false;
            }
        }
        uint8_t *data() {
            assert(buff != nullptr); // Using moved object
            return buff->data.data();
        }
        const uint8_t *data() const {
            assert(buff != nullptr); // Using moved object
            return buff->data.data();
        }
        constexpr static size_t SIZE = S;
        constexpr size_t size() const {
            return SIZE;
        }
    };

    std::optional<Borrow> borrow() {
        if (borrowed) {
            return std::nullopt;
        } else {
            borrowed = true;
            return Borrow(this);
        }
    }
    constexpr static size_t SIZE = S;
    constexpr size_t size() const {
        return SIZE;
    }
};

// How large buffer do we need?
// 512: For the gcode commands.
// File names: for marlin vars and for commands that manipulate files.
constexpr size_t BORROW_BUF_SIZE = std::max(512, FILE_PATH_BUFFER_LEN + FILE_NAME_BUFFER_LEN);

using SharedBuffer = Buffer<BORROW_BUF_SIZE>;
using SharedBorrow = std::shared_ptr<SharedBuffer::Borrow>;

class SharedPath {
private:
    SharedBorrow borrow;

public:
    SharedPath() = default;
    SharedPath(SharedBuffer::Borrow borrow)
        : borrow(std::make_shared<SharedBuffer::Borrow>(std::move(borrow))) {}
    // Pointing into that borrow.
    const char *path() const {
        return reinterpret_cast<const char *>(borrow->data());
    }
    char *path() {
        return reinterpret_cast<char *>(borrow->data());
    }

    // Stored just behind the path (maybe!)
    char *name() {
        char *path = this->path();
        size_t plen = strlen(path);
        // Enough space for the name too.
        assert(plen < FILE_PATH_BUFFER_LEN);
        return path + plen + 1;
    }
    const char *name() const {
        const char *path = this->path();
        size_t plen = strlen(path);
        // Enough space for the name too.
        assert(plen < FILE_PATH_BUFFER_LEN);
        return path + plen + 1;
    }
};

// Kind of similar to SharedPath, except it uses the borrow directly, not a
// shared pointer to one.
//
// Not easy to template this difference out, due to accessing the data through
// either `->` or `.` :-(. (I won't claim there's no way to do it in C++, but a
// small copy-paste is probably better than arcane dark arts).
class BorrowPaths {
private:
    SharedBuffer::Borrow borrow;

public:
    BorrowPaths(SharedBuffer::Borrow borrow)
        : borrow(std::move(borrow)) {}
    // Pointing into that borrow.
    const char *path() const {
        return reinterpret_cast<const char *>(borrow.data());
    }
    char *path() {
        return reinterpret_cast<char *>(borrow.data());
    }

    // Stored just behind the path (maybe!)
    char *name() {
        char *path = this->path();
        size_t plen = strlen(path);
        // Enough space for the name too.
        assert(plen < FILE_PATH_BUFFER_LEN);
        return path + plen + 1;
    }
    const char *name() const {
        const char *path = this->path();
        size_t plen = strlen(path);
        // Enough space for the name too.
        assert(plen < FILE_PATH_BUFFER_LEN);
        return path + plen + 1;
    }
};
