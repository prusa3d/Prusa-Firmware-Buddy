/// @file
#pragma once

#include <cstdint>
#include <bsod/bsod.h>

struct EndToolIndexIterator {};

/// Doubles as a range
template <typename Index>
class ToolIndexIterator {

public:
    // Necessary to satisfy std::input_iterator, even though it doesn't make sense
    using difference_type = std::ptrdiff_t;
    using value_type = Index;

public:
    ToolIndexIterator(const ToolIndexIterator &) = default;

    static constexpr ToolIndexIterator make_empty() {
        return ToolIndexIterator {};
    }

    /// @returns iterator iterating over all tools
    static constexpr ToolIndexIterator make_all() {
        ToolIndexIterator r;
        r.pos_ = 0;
        return r;
    }

    /// @returns an iterator that iterates through a single tool index
    static constexpr ToolIndexIterator make_single(Index index) {
        ToolIndexIterator r;
        r.pos_ = index.to_raw();
        r.single_ = true;
        return r;
    }

    /// @returns an 'adjusted' iterator that skips disabled tools
    ToolIndexIterator skip_all_disabled() const {
        ToolIndexIterator r = *this;
        r.skip_disabled_ = true;
        r.ensure_valid();
        return r;
    }

public:
    constexpr inline ToolIndexIterator begin() const {
        return *this;
    }

    constexpr inline auto end() const {
        return EndToolIndexIterator {};
    }

public:
    inline bool at_end() const {
        return pos_ == Index::count;
    }

    /// Prefix operator
    constexpr inline ToolIndexIterator &operator++() {
        debug_assert(!at_end());
        increment_impl();
        ensure_valid();
        return *this;
    }
    constexpr void operator++(int) {
        ++*this;
    }

    constexpr inline Index index() const {
        debug_assert(!at_end());
        return Index::from_raw(pos_);
    }

    constexpr Index operator*() const {
        return index();
    }

    constexpr inline bool operator==(const ToolIndexIterator &) const = default;
    constexpr inline bool operator!=(const ToolIndexIterator &) const = default;

    constexpr inline bool operator==(EndToolIndexIterator) const {
        return pos_ == Index::count;
    }

protected:
    constexpr void increment_impl() {
        if (single_) {
            // Set to end
            pos_ = Index::count;
            return;
        }

        pos_++;
    }

    /// If pos_ is on an item that should be skipped/filtered out, increases it until it isn't
    constexpr void ensure_valid() {
        while (!at_end()) {
            const bool should_skip = //
                (skip_disabled_ && !index().is_enabled());

            if (!should_skip) {
                return;
            }

            increment_impl();
        }
    }

private:
    constexpr ToolIndexIterator() = default;

private:
    uint8_t pos_ = Index::count;

    /// Iterate only over enabled tools
    bool skip_disabled_ : 1 = false;

    /// Iterate over only single tool
    bool single_ : 1 = false;
};
