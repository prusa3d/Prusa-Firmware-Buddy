#ifndef SFL_TEST_STATELESS_FANCY_ALLOC_HPP
#define SFL_TEST_STATELESS_FANCY_ALLOC_HPP

#include <cstddef>
#include <memory>
#include <type_traits>

#include "fancy_ptr.hpp"

namespace sfl
{
namespace test
{

template<typename T>
class stateless_fancy_alloc
{
public:

    using value_type      = T;
    using pointer         = fancy_ptr<T>;
    using const_pointer   = fancy_ptr<const T>;
    using reference       = T&;
    using const_reference = const T&;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;

    using propagate_on_container_copy_assignment = std::true_type;
    using propagate_on_container_move_assignment = std::true_type;
    using propagate_on_container_swap            = std::true_type;

    template <typename U>
    struct rebind
    {
        using other = stateless_fancy_alloc<U>;
    };

    //
    // ---- CONSTRUCTION AND DESTRUCTION --------------------------------------
    //

    stateless_fancy_alloc() noexcept
    {}

    stateless_fancy_alloc(const stateless_fancy_alloc& /*other*/) noexcept
    {}

    template <typename U>
    stateless_fancy_alloc(const stateless_fancy_alloc<U>& /*other*/) noexcept
    {}

    stateless_fancy_alloc(stateless_fancy_alloc&& /*other*/) noexcept
    {}

    template <typename U>
    stateless_fancy_alloc(stateless_fancy_alloc<U>&& /*other*/) noexcept
    {}

    ~stateless_fancy_alloc() noexcept
    {}

    //
    // ---- ASSIGNMENT --------------------------------------------------------
    //

    stateless_fancy_alloc& operator=(const stateless_fancy_alloc& /*other*/) noexcept
    {
        return *this;
    }

    stateless_fancy_alloc& operator=(stateless_fancy_alloc&& /*other*/) noexcept
    {
        return *this;
    }

    //
    // ---- ALLOCATE AND DEALLOACTE -------------------------------------------
    //

    pointer allocate(size_type n, const void* = nullptr)
    {
        if (n > max_size())
        {
            #ifdef SFL_NO_EXCEPTIONS
            assert(!"n > max_size()");
            std::abort();
            #else
            throw std::bad_alloc();
            #endif
        }

        return std::pointer_traits<pointer>::pointer_to
        (
            *static_cast<T*>(::operator new(n * sizeof(T)))
        );
    }

    void deallocate(pointer p, size_type)
    {
        ::operator delete(std::addressof(*p));
    }

    //
    // ---- CONSTRUCT AND DESTROY ---------------------------------------------
    //

    template <typename U, typename... Args>
    void construct(U* p, Args&&... args) noexcept(
        std::is_nothrow_constructible<U, Args...>::value
    )
    {
        ::new ((void *)p) U(std::forward<Args>(args)...);
    }

    template <typename U>
    void destroy(U* p) noexcept(std::is_nothrow_destructible<U>::value)
    {
        p->~U();
    }

    //
    // ---- ADDRESS -----------------------------------------------------------
    //

    pointer address(reference x) const noexcept
    {
        return std::addressof(x);
    }

    const_pointer address(const_reference x) const noexcept
    {
        return std::addressof(x);
    }

    //
    // ---- MAX SIZE ----------------------------------------------------------
    //

    size_type max_size() const noexcept
    {
        return std::size_t(PTRDIFF_MAX) / sizeof(T);
    }

    //
    // ---- COMPARISONS -------------------------------------------------------
    //

    template <typename T1, typename T2>
    friend bool operator==
    (
        const stateless_fancy_alloc<T1>& /*x*/,
        const stateless_fancy_alloc<T2>& /*y*/
    ) noexcept;

    template <typename T1, typename T2>
    friend bool operator!=
    (
        const stateless_fancy_alloc<T1>& /*x*/,
        const stateless_fancy_alloc<T2>& /*y*/
    ) noexcept;
};

template <typename T1, typename T2>
bool operator==
(
    const stateless_fancy_alloc<T1>& /*x*/,
    const stateless_fancy_alloc<T2>& /*y*/
) noexcept
{
    return true;
}

template <typename T1, typename T2>
bool operator!=
(
    const stateless_fancy_alloc<T1>& /*x*/,
    const stateless_fancy_alloc<T2>& /*y*/
) noexcept
{
    return false;
}

} // namespace test
} // namespace sfl

#endif // SFL_TEST_STATELESS_FANCY_ALLOC_HPP
