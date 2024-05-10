#pragma once

#include <type_traits> //aligned_union
#include <memory>

struct static_unique_ptr_deleter {
    void (*f)(void *) = nullptr;

    inline void operator()(void *ptr) {
        f(ptr);
    }
};

// unique pointer no inline (template is inline by default)
template <class T, class... Args>
auto make_static_unique_ptr(void *place, Args &&...args) {
    constexpr static_unique_ptr_deleter deleter(+[](void *ptr) {
        reinterpret_cast<T *>(ptr)->~T();
    });

    ::new (place) T(std::forward<Args>(args)...);
    return std::unique_ptr<T, static_unique_ptr_deleter>(reinterpret_cast<T *>(place), deleter);
}

template <class T>
using static_unique_ptr = std::unique_ptr<T, static_unique_ptr_deleter>; // alias
