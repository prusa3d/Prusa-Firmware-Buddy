#pragma once

// The path length constants live in gui :-(
#include <gui/file_list_defs.h>

#include <array>
#include <optional>
#include <cassert>
#include <cstdint>

namespace con {

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
    };

    std::optional<Borrow> borrow() {
        if (borrowed) {
            return std::nullopt;
        } else {
            borrowed = true;
            return Borrow(this);
        }
    }
};

// How large buffer do we need?
// 512: For the gcode commands.
// File names: for marlin vars and for commands that manipulate files.
constexpr size_t BORROW_BUF_SIZE = std::max(512, FILE_PATH_BUFFER_LEN + FILE_NAME_BUFFER_LEN);

using SharedBuffer = Buffer<BORROW_BUF_SIZE>;

}
