#undef NDEBUG // This is very important. Must be in the first line.

#include "sfl/detail/allocator_traits.hpp"

#include "check.hpp"
#include "print.hpp"

#include <memory>
#include <type_traits>

template <typename T>
using A0 = std::allocator<T>;

template <typename T>
struct A1
{
    using value_type = T;
};

template <typename T>
struct A2
{
    using value_type = T;
    using is_partially_propagable = std::true_type;
};

template <typename T>
struct A3
{
    using value_type = T;
    using is_partially_propagable = std::false_type;
};

static_assert(std::is_same<typename sfl::dtl::allocator_traits<A0<int>>::template rebind_alloc<double>::value_type, double>::value == true, "");
static_assert(std::is_same<typename sfl::dtl::allocator_traits<A1<int>>::template rebind_alloc<double>::value_type, double>::value == true, "");
static_assert(std::is_same<typename sfl::dtl::allocator_traits<A2<int>>::template rebind_alloc<double>::value_type, double>::value == true, "");
static_assert(std::is_same<typename sfl::dtl::allocator_traits<A3<int>>::template rebind_alloc<double>::value_type, double>::value == true, "");

static_assert(sfl::dtl::allocator_traits<A0<int>>::is_partially_propagable::value == false, "");
static_assert(sfl::dtl::allocator_traits<A1<int>>::is_partially_propagable::value == false, "");
static_assert(sfl::dtl::allocator_traits<A2<int>>::is_partially_propagable::value == true,  "");
static_assert(sfl::dtl::allocator_traits<A3<int>>::is_partially_propagable::value == false, "");

int main()
{
}
