#ifndef SFL_TEST_STATEFULL_ALLOC_HPP
#define SFL_TEST_STATEFULL_ALLOC_HPP

#include <cstddef>
#include <type_traits>

namespace sfl
{
namespace test
{

std::size_t statefull_alloc_counter = 0;

template <typename T>
class statefull_alloc
{
private:

    template <typename>
    friend class statefull_alloc;

    std::size_t id_;

public:

    using value_type      = T;
    using pointer         = T*;
    using const_pointer   = const T*;
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
        using other = statefull_alloc<U>;
    };

    //
    // ---- CONSTRUCTION AND DESTRUCTION --------------------------------------
    //

    statefull_alloc() noexcept
        : id_(++statefull_alloc_counter)
    {}

    statefull_alloc(const statefull_alloc& other) noexcept
        : id_(other.id_)
    {}

    template <typename U>
    statefull_alloc(const statefull_alloc<U>& other) noexcept
        : id_(other.id_)
    {}

    statefull_alloc(statefull_alloc&& other) noexcept
        : id_(other.id_)
    {}

    template <typename U>
    statefull_alloc(statefull_alloc<U>&& other) noexcept
        : id_(other.id_)
    {}

    ~statefull_alloc() noexcept
    {}

    //
    // ---- ASSIGNMENT --------------------------------------------------------
    //

    statefull_alloc& operator=(const statefull_alloc& other) noexcept
    {
        id_ = other.id_;
        return *this;
    }

    statefull_alloc& operator=(statefull_alloc&& other) noexcept
    {
        id_ = other.id_;
        return *this;
    }

    //
    // ---- ALLOCATE AND DEALLOACTE -------------------------------------------
    //

    T* allocate(size_type n, const void* = nullptr)
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

        return static_cast<T*>(::operator new(n * sizeof(T)));
    }

    void deallocate(T* p, size_type)
    {
        ::operator delete(p);
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
        const statefull_alloc<T1>& x,
        const statefull_alloc<T2>& y
    ) noexcept;

    template <typename T1, typename T2>
    friend bool operator!=
    (
        const statefull_alloc<T1>& x,
        const statefull_alloc<T2>& y
    ) noexcept;
};

template <typename T1, typename T2>
bool operator==
(
    const statefull_alloc<T1>& x,
    const statefull_alloc<T2>& y
) noexcept
{
    return x.id_ == y.id_;
}

template <typename T1, typename T2>
bool operator!=
(
    const statefull_alloc<T1>& x,
    const statefull_alloc<T2>& y
) noexcept
{
    return !(x == y);
}

} // namespace test
} // namespace sfl

#endif // SFL_TEST_STATEFULL_ALLOC_HPP
