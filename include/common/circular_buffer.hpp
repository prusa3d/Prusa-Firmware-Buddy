#pragma once

#include <cstddef>
#include <array>
#include <cassert>

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
    void put(const T &item) {
        assert(!is_full());
        const size_t new_write_index = (write_index + 1) % N;
        buffer[write_index] = item;
        write_index = new_write_index;
    }

    T get() {
        assert(!is_empty());
        auto item = front();
        pop();
        return item;
    }

    T front() const {
        assert(!is_empty());
        return buffer[read_index];
    }

    inline bool is_empty() const { return read_index == write_index; }
    inline bool is_full() const { return read_index == (write_index + 1) % N; }

    void pop() {
        assert(!is_empty());
        read_index = (read_index + 1) % N;
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
