#pragma once

#include <memory>
#include <type_traits>
#include <bsod/bsod.h>

template <auto>
class SingleLinkedList;

/// Class representing an intrusive linked list
template <class T, class NextPtr, NextPtr T::*next_ptr>
class SingleLinkedList<next_ptr> {
    static_assert(std::is_same_v<typename std::pointer_traits<NextPtr>::element_type, T>);

public:
    inline bool empty() const {
        return front_ == nullptr;
    }

    inline T &front() {
        debug_assert(!empty());
        return *front_;
    }

    /// Adds the item to the front of the linked list
    /// !!! Item is not copied, the referenced instance itself is linked in to the list
    void push_front(T &item) {
        item.*next_ptr = front_;
        front_ = &item;
    }

    void pop_front() {
        debug_assert(!empty());
        front_ = front_->*next_ptr;
    }

    /// complexity O(size)
    [[nodiscard]] T &back() {
        debug_assert(!empty());
        NextPtr curr = front_;
        while (auto next = curr->*next_ptr) {
            curr = next;
        }
        return *curr;
    }

    bool remove(T &item) {
        NextPtr *ii = &front_;
        while (NextPtr i = *ii) {
            if (i == &item) {
                *ii = i->*next_ptr;
                return true;
            }

            ii = &(i->*next_ptr);
        }

        return false;
    }

public:
    struct iterator {
        friend class SingleLinkedList;

    public:
        using difference_type = std::ptrdiff_t;
        using value_type = T;

    public:
        iterator() = default;
        iterator(const iterator &) = default;

        inline iterator &operator++() {
            i_ = i_->*next_ptr;
            return *this;
        }
        inline iterator operator++(int) {
            iterator tmp = *this;
            operator++();
            return tmp;
        }

        inline value_type &operator*() const {
            return *i_;
        }

        bool operator==(const iterator &) const = default;

        iterator &operator=(const iterator &) = default;

    private:
        explicit iterator(NextPtr i)
            : i_(i) {}

        NextPtr i_ = nullptr;
    };

    iterator begin() {
        return iterator { front_ };
    }

    iterator end() {
        return iterator { nullptr };
    }

private:
    NextPtr front_ = nullptr;
};
