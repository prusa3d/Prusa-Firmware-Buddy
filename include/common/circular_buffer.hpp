#pragma once

#include <cstddef>
#include <array>

// Minimal circular buffer.
// Unlike CircleBuffer, this one actually works.
// Note that actual capacity is N-1.
// This must be externally synchronized.
template <class T, size_t N>
class CircularBuffer {
private:
    static constexpr bool is_power_of_2(size_t n) {
        return n && ((n & (n - 1)) == 0);
    }

    // Ensure that indices are advanced using bit operations
    static_assert(is_power_of_2(N));

    size_t write_index = 0;
    size_t read_index = 0;
    std::array<T, N> buffer;

public:
    [[nodiscard]] bool try_put(const T &item) {
        const size_t new_write_index = (write_index + 1) % N;
        if (new_write_index == read_index) {
            return false; // buffer is full
        }
        buffer[write_index] = item;
        write_index = new_write_index;
        return true;
    }

    [[nodiscard]] bool try_get(T &item) {
        if (read_index == write_index) {
            return false; // buffer is empty
        }
        item = buffer[read_index];
        read_index = (read_index + 1) % N;
        return true;
    }

    void clear() {
        write_index = 0;
        read_index = 0;
    }

    size_t size() const {
        if (read_index > write_index) {
            return N + write_index - read_index;
        } else {
            return write_index - read_index;
        }
    }
};
