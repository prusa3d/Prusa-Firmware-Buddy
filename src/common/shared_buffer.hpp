#pragma once

// The path length constants live in gui :-(
#include <buddy/filename_defs.hpp>

#include <array>
#include <optional>
#include <cstdint>
#include <cstring>
#include <memory>
#include <span>
#include <bsod/bsod.h>

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
                debug_assert(buff->borrowed);
                buff->borrowed = false;
            }
            buff = other.buff;
            other.buff = nullptr;
            return *this;
        }
        ~Borrow() {
            if (buff != nullptr) {
                debug_assert(buff->borrowed);
                buff->borrowed = false;
            }
        }
        uint8_t *data() {
            debug_assert(buff != nullptr); // Using moved object
            return buff->data.data();
        }
        const uint8_t *data() const {
            debug_assert(buff != nullptr); // Using moved object
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
// - Two modes:
//   . Commands that create files have /sfn/sfn/sfn/last-segment-is-lfn-because-sfn-doesnt-exist-yet. These have no name after it. That's the hybrid size.
//   . Commands that ask about existing file. They have /sfn/sfn/sfn/lastsfn and, for the answer rendering, we may append a LFN name after it (that doesn't come from server, we append later).
// We _could_ do it one or the other, but enforcing it structurally is more work and the hybrid+name still fits below 512 - so we just allow for hybrid+name as the upper bound.
constexpr size_t BORROW_BUF_SIZE = std::max<size_t>(512, filename_defs::hybrid_path_buffer_size + filename_defs::filename_buffer_size);

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
    std::span<char> path_span() {
        // The path may be a hybrid (SFN dir + LFN tail); name() lives behind it.
        static_assert(filename_defs::hybrid_path_buffer_size <= decltype(borrow)::element_type::SIZE);
        return { path(), filename_defs::hybrid_path_buffer_size };
    }

    // Stored just behind the path (maybe!)
    char *name() {
        char *path = this->path();
        size_t plen = strlen(path);
        // The name (up to filename_buffer_size) is written at path+plen+1; keep
        // it within the buffer. Enforced even in release.
        release_assert(plen + 1 + filename_defs::filename_buffer_size <= BORROW_BUF_SIZE);
        return path + plen + 1;
    }
    const char *name() const {
        const char *path = this->path();
        size_t plen = strlen(path);
        // The name (up to filename_buffer_size) is written at path+plen+1; keep
        // it within the buffer. Enforced even in release.
        release_assert(plen + 1 + filename_defs::filename_buffer_size <= BORROW_BUF_SIZE);
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
    std::span<char> path_span() {
        // The path may be a hybrid (SFN dir + LFN tail); name() lives behind it.
        static_assert(filename_defs::hybrid_path_buffer_size <= decltype(borrow)::SIZE);
        return { path(), filename_defs::hybrid_path_buffer_size };
    }

    // Stored just behind the path (maybe!)
    char *name() {
        char *path = this->path();
        size_t plen = strlen(path);
        // The name (up to filename_buffer_size) is written at path+plen+1; keep
        // it within the buffer. Enforced even in release.
        release_assert(plen + 1 + filename_defs::filename_buffer_size <= BORROW_BUF_SIZE);
        return path + plen + 1;
    }
    const char *name() const {
        const char *path = this->path();
        size_t plen = strlen(path);
        // The name (up to filename_buffer_size) is written at path+plen+1; keep
        // it within the buffer. Enforced even in release.
        release_assert(plen + 1 + filename_defs::filename_buffer_size <= BORROW_BUF_SIZE);
        return path + plen + 1;
    }
};
