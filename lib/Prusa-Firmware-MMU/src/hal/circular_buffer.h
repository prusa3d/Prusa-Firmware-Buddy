/// @file circular_buffer.h
#pragma once
#include <stddef.h>
#include "../intlimits.h"

/// A generic circular index class which can be used to build circular buffers
/// Can hold up to size elements
/// @param index_t data type of indices into array of elements
///   (recommended to keep uint8_fast8_t as single byte operations are atomical on the AVR)
/// @param size number of index positions.
///   It is recommended to keep a power of 2 to allow for optimal code generation on the AVR (there is no HW modulo instruction)
template <typename index_t = uint_fast8_t, index_t size = 16>
class CircularIndex {
public:
    static constexpr bool size_is_power2 = !(size & (size - 1));

    static_assert(size <= std::numeric_limits<index_t>::max() / 2,
        "index_t is too small for the requested size");

    constexpr inline CircularIndex()
        : tail(0)
        , head(0) {}

    /// @returns true if empty
    inline bool empty() const {
        return tail == head;
    }

    /// @returns true if full
    inline bool full() const {
        return count() == size;
    }

    /// Reset the indexes to empty
    inline void reset() {
        head = tail;
    }

    /// Advance the head index of the buffer.
    /// No checks are performed. full() needs to be queried beforehand.
    inline void push() {
        head = next(head);
    }

    /// Advance the tail index from the buffer.
    /// No checks are performed. empty() needs to be queried beforehand.
    inline void pop() {
        tail = next(tail);
    }

    /// @returns return the tail index from the buffer.
    /// Does not perform any range checks for performance reasons, should be preceeded by if(!empty()) in the user code
    inline index_t front() const {
        return mask(tail);
    }

    /// @returns return the head index from the buffer.
    /// Does not perform any range checks for performance reasons, should be preceeded by if(!empty()) in the user code
    inline index_t back() const {
        return mask(head);
    }

    /// @returns number of elements in the buffer
    inline index_t count() const {
        if constexpr (size_is_power2)
            return head - tail;
        else {
            return head >= tail
                ? (head - tail)
                : (size * 2 + head) - tail;
        }
    }

protected:
    index_t tail; ///< cursor of the element to read (pop/extract) from the buffer
    index_t head; ///< cursor of the empty spot or element insertion (write)

    /// @return the index position given a cursor
    static index_t mask(index_t cursor) { return cursor % size; }

    /// @returns next cursor for internal comparisons
    static index_t next(index_t cursor) {
        // note: the modulo can be avoided if size is a power of two: we can do this
        // relying on the optimizer eliding the following check at compile time.
        if constexpr (size_is_power2)
            return (cursor + 1);
        else
            return (cursor + 1) % (size * 2);
    }
};

/// A generic circular buffer class
/// Can hold up to size elements
/// @param T data type of stored elements
/// @param index_t data type of indices into array of elements
///   (recommended to keep uint8_fast8_t as single byte operations are atomical on the AVR)
/// @param size number of elements to store
///   It is recommended to keep a power of 2 to allow for optimal code generation on the AVR (there is no HW modulo instruction)
template <typename T = uint8_t, typename index_t = uint_fast8_t, size_t size = 16>
class CircularBuffer {
public:
    inline bool empty() const {
        return index.empty();
    }

    bool full() const {
        return index.full();
    }

    /// Reset the circular buffer to empty
    inline void reset() {
        index.reset();
    }

    /// Insert an element into the buffer.
    /// Checks for empty spot for the element and does not change the buffer content
    /// in case the buffer is full.
    /// @returns true if the insertion was successful (i.e. there was an empty spot for the element)
    bool push(T elem) {
        if (full())
            return false;
        data[index.back()] = elem;
        index.push();
        return true;
    }

    /// @returns peeks the current element to extract from the buffer, however the element is left in the buffer
    /// Does not perform any range checks for performance reasons, should be preceeded by if(!empty()) in the user code
    inline T front() const {
        return data[index.front()];
    }

    /// Extracts the current element from the buffer
    /// @returns true in case there was an element for extraction (i.e. the buffer was not empty)
    bool pop(T &elem) {
        if (empty())
            return false;
        elem = front();
        index.pop();
        return true;
    }

    index_t count() const {
        return index.count();
    }

protected:
    T data[size]; ///< array of stored elements
    CircularIndex<index_t, size> index; ///< circular index
};
