#undef NDEBUG // This is very important. Must be in the first line.

#include "sfl/detail/node_small_allocator.hpp"

#include "sfl/detail/uninitialized_memory_algorithms.hpp"

#include "check.hpp"
#include "print.hpp"

#include "xint.hpp"

#include "statefull_alloc.hpp"
#include "stateless_alloc.hpp"
#include "stateless_alloc_no_prop.hpp"
#include "stateless_fancy_alloc.hpp"

template <typename T, std::size_t N, typename Allocator>
void test_node_small_allocator()
{
    using allocator_type = sfl::dtl::node_small_allocator<T, N, Allocator>;
    using pointer = typename allocator_type::pointer;

    allocator_type a;

    pointer p1 = sfl::dtl::allocate(a, 1);
    pointer p2 = sfl::dtl::allocate(a, 1);
    pointer p3 = sfl::dtl::allocate(a, 1);
    pointer p4 = sfl::dtl::allocate(a, 1);

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

int main()
{
    using sfl::test::xint;

    PRINT("Test node_small_allocator<xint, 2, std::allocator<xint>>");
    test_node_small_allocator<xint, 2, std::allocator<xint>>();

    PRINT("Test node_small_allocator<xint, 0, std::allocator<xint>>");
    test_node_small_allocator<xint, 0, std::allocator<xint>>();

    PRINT("Test node_small_allocator<xint, 2, sfl::test::statefull_alloc<xint>>");
    test_node_small_allocator<xint, 2, sfl::test::statefull_alloc<xint>>();

    PRINT("Test node_small_allocator<xint, 0, sfl::test::statefull_alloc<xint>>");
    test_node_small_allocator<xint, 0, sfl::test::statefull_alloc<xint>>();

    PRINT("Test node_small_allocator<xint, 2, sfl::test::stateless_alloc<xint>>");
    test_node_small_allocator<xint, 2, sfl::test::stateless_alloc<xint>>();

    PRINT("Test node_small_allocator<xint, 0, sfl::test::stateless_alloc<xint>>");
    test_node_small_allocator<xint, 0, sfl::test::stateless_alloc<xint>>();

    PRINT("Test node_small_allocator<xint, 2, sfl::test::stateless_alloc_no_prop<xint>>");
    test_node_small_allocator<xint, 2, sfl::test::stateless_alloc_no_prop<xint>>();

    PRINT("Test node_small_allocator<xint, 0, sfl::test::stateless_alloc_no_prop<xint>>");
    test_node_small_allocator<xint, 0, sfl::test::stateless_alloc_no_prop<xint>>();

    PRINT("Test node_small_allocator<xint, 2, sfl::test::stateless_fancy_alloc<xint>>");
    test_node_small_allocator<xint, 2, sfl::test::stateless_fancy_alloc<xint>>();

    PRINT("Test node_small_allocator<xint, 0, sfl::test::stateless_fancy_alloc<xint>>");
    test_node_small_allocator<xint, 0, sfl::test::stateless_fancy_alloc<xint>>();
}
