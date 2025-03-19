#undef NDEBUG // This is very important. Must be in the first line.

#include "sfl/detail/node_static_allocator.hpp"

#include "sfl/detail/uninitialized_memory_algorithms.hpp"

#include "check.hpp"
#include "print.hpp"

#include "xint.hpp"

int main()
{
    using sfl::test::xint;

    PRINT("Test node_static_allocator<xint, 4>");

    sfl::dtl::node_static_allocator<xint, 4> a;

    auto* p1 = sfl::dtl::allocate(a, 1);
    auto* p2 = sfl::dtl::allocate(a, 1);
    auto* p3 = sfl::dtl::allocate(a, 1);
    auto* p4 = sfl::dtl::allocate(a, 1);

    sfl::dtl::construct_at_a(a, p1, 10);
    sfl::dtl::construct_at_a(a, p2, 20);
    sfl::dtl::construct_at_a(a, p3, 30);
    sfl::dtl::construct_at_a(a, p4, 40);

    sfl::dtl::destroy_at_a(a, p1);
    sfl::dtl::destroy_at_a(a, p2);
    sfl::dtl::destroy_at_a(a, p3);
    sfl::dtl::destroy_at_a(a, p4);

    sfl::dtl::deallocate(a, p1, 1);
    sfl::dtl::deallocate(a, p2, 1);
    sfl::dtl::deallocate(a, p3, 1);
    sfl::dtl::deallocate(a, p4, 1);
}
