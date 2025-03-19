#ifndef SFL_TEST_FANCY_PTR_HPP
#define SFL_TEST_FANCY_PTR_HPP

#include <cstddef>

namespace sfl
{

namespace test
{

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

namespace dtl
{

struct nat
{
};

template <typename T>
struct add_reference
{
    using type = T&;
};

template <class T>
struct add_reference<T &>
{
    using type = T&;
};

template <>
struct add_reference<void>
{
    using type = nat&;
};

template <>
struct add_reference<const void>
{
    using type = const nat&;
};

} // namespace dtl

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template <typename T>
class fancy_ptr
{
    template <typename>
    friend class fancy_ptr;

public:

    using element_type      = T;
    using difference_type   = std::ptrdiff_t;
    using value_type        = typename std::remove_cv<T>::type;
    using pointer           = T*;
    using reference         = typename sfl::test::dtl::add_reference<T>::type;
    using iterator_category = std::random_access_iterator_tag;

private:

    pointer ptr_;

private:

    fancy_ptr(pointer ptr, bool /*dummy*/) noexcept
        : ptr_(ptr)
    {};

public:

    static fancy_ptr pointer_to(reference r) noexcept
    {
        return fancy_ptr(&r, false);
    }

public:

    // Default constructor
    fancy_ptr() noexcept
    {}

    // Construct from raw pointer
    fancy_ptr(pointer ptr) noexcept
        : ptr_(ptr)
    {}

    // Construct from other raw pointer
    template <typename U,
              typename std::enable_if<std::is_convertible<T*, U*>::value>::type* = nullptr>
    fancy_ptr(U* ptr) noexcept
        : ptr_(static_cast<T*>(ptr))
    {}

    // Copy constructor
    fancy_ptr(const fancy_ptr& other) noexcept
        : ptr_(other.ptr_)
    {}

    // Construct from other fancy_ptr.
    // Participates in overload resolution if U* is convertible to T*.
    template <typename U,
              typename std::enable_if<std::is_convertible<U*, T*>::value>::type* = nullptr>
    fancy_ptr(const fancy_ptr<U>& other) noexcept
        : ptr_(static_cast<T*>(other.ptr_))
    {}

    // Construct from other fancy_ptr.
    // Participates in overload resolution if U* is constructible from T*.
    template <typename U,
              typename std::enable_if<!std::is_convertible<U*, T*>::value && std::is_constructible<U*, T*>::value>::type* = nullptr>
    explicit
    fancy_ptr(const fancy_ptr<U>& other) noexcept
        : ptr_(static_cast<T*>(other.ptr_))
    {}

    #if 0 // Old code.
    template <typename U = T, typename std::enable_if<std::is_const<U>::value>::type* = nullptr>
    fancy_ptr(const fancy_ptr<typename std::remove_const<T>::type>& other) noexcept
        : ptr_(other.operator->()  /* std::to_address(other) in C++20 */)
    {}
    #endif

    //
    // ---- ASSIGNMENT --------------------------------------------------------
    //

    // Copy assignment operator.
    fancy_ptr& operator=(const fancy_ptr& other) noexcept
    {
        ptr_ = other.ptr_;
        return *this;
    }

    //
    // ---- OBSERVERS ---------------------------------------------------------
    //

    explicit operator bool() const noexcept
    {
        return ptr_ != nullptr;
    }

    reference operator*() const noexcept
    {
        return *ptr_;
    }

    reference operator[](difference_type n) const noexcept
    {
        return *(ptr_ + n);
    }

    pointer operator->() const noexcept
    {
        return ptr_;
    }

    //
    // ---- INCREMENT AND DECREMENT -------------------------------------------
    //

    fancy_ptr& operator++() noexcept
    {
        ++ptr_;
        return *this;
    }

    fancy_ptr operator++(int) noexcept
    {
        auto temp = *this;
        ++(*this);
        return temp;
    }

    fancy_ptr& operator--() noexcept
    {
        --ptr_;
        return *this;
    }

    fancy_ptr operator--(int) noexcept
    {
        auto temp = *this;
        --(*this);
        return temp;
    }

    fancy_ptr& operator+=(difference_type n) noexcept
    {
        ptr_ += n;
        return *this;
    }

    fancy_ptr& operator-=(difference_type n) noexcept
    {
        ptr_ -= n;
        return *this;
    }

    friend
    fancy_ptr operator+(const fancy_ptr& p, difference_type n) noexcept
    {
        auto temp = p;
        temp += n;
        return temp;
    }

    friend
    fancy_ptr operator-(const fancy_ptr& p, difference_type n) noexcept
    {
        auto temp = p;
        temp -= n;
        return temp;
    }

    friend
    difference_type operator-(const fancy_ptr& x, const fancy_ptr& y) noexcept
    {
        return x.ptr_ - y.ptr_;
    }

    //
    // ---- COMPARISONS -------------------------------------------------------
    //

    friend
    bool operator==(const fancy_ptr& x, const fancy_ptr& y) noexcept
    {
        return x.ptr_ == y.ptr_;
    }

    friend
    bool operator!=(const fancy_ptr& x, const fancy_ptr& y) noexcept
    {
        return !(x == y);
    }

    friend
    bool operator<(const fancy_ptr& x, const fancy_ptr& y) noexcept
    {
        return x.ptr_ < y.ptr_;
    }

    friend
    bool operator>(const fancy_ptr& x, const fancy_ptr& y) noexcept
    {
        return y < x;
    }

    friend
    bool operator<=(const fancy_ptr& x, const fancy_ptr& y) noexcept
    {
        return !(y < x);
    }

    friend
    bool operator>=(const fancy_ptr& x, const fancy_ptr& y) noexcept
    {
        return !(x < y);
    }
};

} // namespace test
} // namespace sfl

#endif // SFL_TEST_FANCY_PTR_HPP
