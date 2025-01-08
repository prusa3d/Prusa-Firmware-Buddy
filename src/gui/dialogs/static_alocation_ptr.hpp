#pragma once

#include <type_traits> //aligned_union
#include <memory>

/// Alternative to std::unique_ptr that does not delete the underlying memory, only calls the destructor
/// and \p static_unique_ptr::make constructs the object on the provided memory (without allocating anything).
/// We cannot simply use std::unique_ptr, because we want to the deleter to support non-virtual destructors -> we need a deleter function.
/// But with std::unique_ptr, the deleter is not hardly tied with the pointer passed around, so it got broken in some cases.
template <class T>
class static_unique_ptr {

public:
    using Deleter = void (*)(void *);

public:
    static_unique_ptr() = default;

    static_unique_ptr(std::nullptr_t) {}

    template <typename T2, typename = std::enable_if_t<std::is_convertible_v<T2 *, T *>>>
    static_unique_ptr(static_unique_ptr<T2> &&o)
        : static_unique_ptr(o.release(), o.get_deleter()) {}

    ~static_unique_ptr() {
        reset();
    }

    template <class... Args>
    static static_unique_ptr make(void *place, Args &&...args) {
        constexpr Deleter deleter = +[](void *ptr) {
            reinterpret_cast<T *>(ptr)->~T();
        };
        return static_unique_ptr(::new (place) T(std::forward<Args>(args)...), deleter);
    }

    template <typename T2, typename = std::enable_if_t<std::is_convertible_v<T2 *, T *>>>
    void reset(static_unique_ptr<T2> &&o) {
        if (ptr) {
            deleter(ptr);
        }

        deleter = o.get_deleter();
        ptr = o.release();
    }

    void reset(std::nullptr_t = nullptr) {
        if (ptr) {
            deleter(ptr);
        }
        ptr = nullptr;
    }

    T *get() const {
        return ptr;
    }

    T *release() {
        T *result = ptr;
        ptr = nullptr;
        return result;
    }

    Deleter get_deleter() const {
        return deleter;
    }

    T *operator->() const {
        return get();
    }

    T &operator*() const {
        return *get();
    }

    operator bool() const {
        return ptr != nullptr;
    }

    template <typename T2, typename = std::enable_if_t<std::is_convertible_v<T2 *, T *>>>
    bool operator==(const static_unique_ptr<T2> &o) const {
        return ptr == o.get();
    }

    bool operator==(std::nullptr_t) const {
        return ptr == nullptr;
    }

    template <typename T2, typename = std::enable_if_t<std::is_convertible_v<T2 *, T *>>>
    bool operator!=(const static_unique_ptr<T2> &o) const {
        return ptr != o.get();
    }

    bool operator!=(std::nullptr_t) const {
        return ptr != nullptr;
    }

    static_unique_ptr &operator=(std::nullptr_t) {
        reset();
        return *this;
    }

    template <typename T2, typename = std::enable_if_t<std::is_convertible_v<T2 *, T *>>>
    static_unique_ptr &operator=(static_unique_ptr<T2> &&o) {
        reset(std::move(o));
        return *this;
    }

private:
    static_unique_ptr(T *ptr, Deleter deleter)
        : ptr(ptr)
        , deleter(deleter) {}

private:
    T *ptr = nullptr;
    Deleter deleter = nullptr;
};

template <class T, class... Args>
auto make_static_unique_ptr(void *place, Args &&...args) {
    return static_unique_ptr<T>::make(place, std::forward<Args>(args)...);
}
