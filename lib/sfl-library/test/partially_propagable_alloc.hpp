#ifndef SFL_TEST_PARTIALLY_PROPAGABLE_ALLOC_HPP
#define SFL_TEST_PARTIALLY_PROPAGABLE_ALLOC_HPP

#include <cstddef>
#include <type_traits>

namespace sfl
{
namespace test
{

template<typename T>
class partially_propagable_alloc
{
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

    using is_partially_propagable = std::true_type;

    template <typename U>
    struct rebind
    {
        using other = partially_propagable_alloc<U>;
    };

    //
    // ---- CONSTRUCTION AND DESTRUCTION --------------------------------------
    //

    partially_propagable_alloc() noexcept
    {}

    partially_propagable_alloc(const partially_propagable_alloc& /*other*/) noexcept
    {}

    template <typename U>
    partially_propagable_alloc(const partially_propagable_alloc<U>& /*other*/) noexcept
    {}

    partially_propagable_alloc(partially_propagable_alloc&& /*other*/) noexcept
    {}

    template <typename U>
    partially_propagable_alloc(partially_propagable_alloc<U>&& /*other*/) noexcept
    {}

    ~partially_propagable_alloc() noexcept
    {}

    //
    // ---- ASSIGNMENT --------------------------------------------------------
    //

    partially_propagable_alloc& operator=(const partially_propagable_alloc& /*other*/) noexcept
    {
        return *this;
    }

    partially_propagable_alloc& operator=(partially_propagable_alloc&& /*other*/) noexcept
    {
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
    // ---- QUERY -------------------------------------------------------------
    //

    bool is_storage_unpropagable(pointer) const noexcept
    {
        return false;
    }
};

template <typename T1, typename T2>
bool operator==(const partially_propagable_alloc<T1>& /*x*/, const partially_propagable_alloc<T2>& /*y*/) noexcept
{
    return true;
}

template <typename T1, typename T2>
bool operator!=(const partially_propagable_alloc<T1>& /*x*/, const partially_propagable_alloc<T2>& /*y*/) noexcept
{
    return false;
}

} // namespace test
} // namespace sfl

#endif // SFL_TEST_PARTIALLY_PROPAGABLE_ALLOC_HPP
