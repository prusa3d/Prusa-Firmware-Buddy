#pragma once

#include <type_traits> //aligned_union
#include <memory>

// custom deleter
struct static_unique_ptr_deleter {
    template <class T>
    void operator()(T *ptr) const {
        ptr->~T();
    }
};

// unique pointer no inline (template is inline by default)
template <class T, class... Args>
std::unique_ptr<T, static_unique_ptr_deleter>
make_static_unique_ptr(void *place, Args &&...args) {
    return std::unique_ptr<T, static_unique_ptr_deleter> {
        ::new (place) T(std::forward<Args>(args)...) //::new - global new (no other new defined elsewhere)
    };
}

template <class T>
using static_unique_ptr = std::unique_ptr<T, static_unique_ptr_deleter>; // alias
