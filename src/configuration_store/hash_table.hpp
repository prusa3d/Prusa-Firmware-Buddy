#pragma once
#include <stdint.h>
#include <functional>
#include "optional"
#include <array>
#include "../logging/log.h"
#include "bsod.h"

// LOG_COMPONENT_DEF(HashTable, LOG_SEVERITY_DEBUG);

template <size_t SIZE>
class HashMap {
    static constexpr size_t MISS_JUMP = 0; //< How many items to skip after collision
    enum class Field : int32_t {
        vacant = -1,
        tombstone = -2
    };
    // using int32_t because input is uint16_t, so I can use negative numbers as markers
    std::array<std::pair<uint32_t, int32_t>, SIZE> data;

    size_t calculate_index(uint32_t key, size_t unsuccesfull);

public:
    class Iterator : public std::forward_iterator_tag {
        using ArrayIterator = typename std::array<std::pair<uint32_t, int32_t>, SIZE>::const_iterator;
        ArrayIterator it;
        ArrayIterator end;

    public:
        bool operator==(const Iterator &rhs) const {
            return it == rhs.it;
        }

        bool operator!=(const Iterator &rhs) const {
            return !(rhs == *this);
        }

        explicit Iterator(ArrayIterator it, ArrayIterator end)
            : it(it)
            , end(end) {}
        Iterator(const Iterator &other)
            : it(other.it)
            , end(other.end) {}

        Iterator &operator++() {
            if (it != end) {
                it++;
            }
            // skip all unoccupied items
            while (it != end && it->second < 0) {
                it++;
            }
            return *this;
        }
        Iterator operator++(int) {
            Iterator to_ret = *this;
            // skip all unoccupied items
            ++it;
            return to_ret;
        }
        const std::pair<uint32_t, int32_t> &operator*() const { return *it; }
    };
    HashMap() {
        data.fill(std::pair<uint32_t, int32_t> { 0, static_cast<int32_t>(Field::vacant) });
    }
    Iterator begin() const {
        auto it = data.begin();
        while (it != data.end() && it->second < 0) {
            it++;
        }
        return Iterator(it, data.end());
    }

    Iterator end() const { return Iterator(data.end(), data.end()); }

    /**
     * Inserts the key to the table
     * @param key to insert
     * @return true if the key was already present, false if not
     */
    bool
    Set(uint32_t key, uint16_t value);
    /**
     *  Gets item stored with key
     * @param key
     * @return Nullopt if not present else the value
     */
    std::optional<uint16_t> Get(uint32_t key);
    /**
     * Removes the key from map
     * @param key
     */
    void remove(uint32_t key);
};
template <size_t SIZE>
size_t HashMap<SIZE>::calculate_index(uint32_t key, size_t unsuccesfull) {
    return (key + unsuccesfull + MISS_JUMP) % SIZE;
}
template <size_t SIZE>
bool HashMap<SIZE>::Set(uint32_t key, uint16_t value) {
    size_t index = 0;
    size_t missed = 0;
    for (index = calculate_index(key, missed); data[index].first != key && data[index].second >= 0; index = calculate_index(key, missed++)) {
        if (missed > SIZE) {
            fatal_error("HashTable full", "HashTable");
        }
    }
    //    log_debug(HashTable, "Input had %d collisions", missed);
    bool present = data[index].first == key;
    data[index].first = key;
    data[index].second = value;
    return present;
}

template <size_t SIZE>
std::optional<uint16_t> HashMap<SIZE>::Get(uint32_t key) {
    size_t index = 0;
    size_t missed = 0;
    for (index = calculate_index(key, missed); data[index].first != key && (data[index].second >= 0 || data[index].second == static_cast<int32_t>(Field::tombstone)); index = calculate_index(key, missed++)) {
        if (missed > SIZE) {
            return std::nullopt; // hashtable is full and we have not found the item
        }
    }
    if (data[index].first != key) {
        return std::nullopt;
    } else {
        return data[index].second;
    }
}
template <size_t SIZE>
void HashMap<SIZE>::remove(uint32_t key) {
    size_t index = 0;
    size_t missed = 0;
    for (index = calculate_index(key, missed); data[index].first != key && (data[index].second >= 0 || data[index].second == static_cast<int32_t>(Field::tombstone)); index = calculate_index(key, missed++)) {
        if (missed > SIZE) {
            return;
        }
    }
    if (data[index].first == key) {
        data[index].first = 0;
        data[index].second = static_cast<int32_t>(Field::tombstone);
    }
}
