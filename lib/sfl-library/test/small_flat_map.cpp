//
// g++ -std=c++11 -g -O0 -Wall -Wextra -Wpedantic -Werror -Wfatal-errors -I ../include small_flat_map.cpp
// valgrind --leak-check=full ./a.out
//

#undef NDEBUG // This is very important. Must be in the first line.

#define SFL_TEST_SMALL_FLAT_MAP
#include "sfl/small_flat_map.hpp"

#include "check.hpp"
#include "istream_view.hpp"
#include "nth.hpp"
#include "pair_io.hpp"
#include "print.hpp"

#include "xint.hpp"
#include "xobj.hpp"

#include "statefull_alloc.hpp"
#include "stateless_alloc.hpp"
#include "stateless_alloc_no_prop.hpp"
#include "stateless_fancy_alloc.hpp"

#include <sstream>
#include <vector>

#if 0 // TODO: Review this
static_assert
(
    sizeof(sfl::small_flat_map<double, double, 0>) ==
        3 * sizeof(sfl::small_flat_map<double, double, 0>::pointer),
    "Invalid size"
);

static_assert
(
    sizeof(sfl::small_flat_map<double, double, 5>) ==
        3 * sizeof(sfl::small_flat_map<double, double, 5>::pointer) +
        5 * sizeof(sfl::small_flat_map<double, double, 0>::value_type),
    "Invalid size"
);
#endif

template <>
void test_small_flat_map<1>()
{
    #undef   TPARAM_ALLOCATOR
    #define  TPARAM_ALLOCATOR std::allocator
    #include "small_flat_map.inc"
}

template <>
void test_small_flat_map<2>()
{
    #undef   TPARAM_ALLOCATOR
    #define  TPARAM_ALLOCATOR sfl::test::statefull_alloc
    #include "small_flat_map.inc"
}

template <>
void test_small_flat_map<3>()
{
    #undef   TPARAM_ALLOCATOR
    #define  TPARAM_ALLOCATOR sfl::test::stateless_alloc
    #include "small_flat_map.inc"
}

template <>
void test_small_flat_map<4>()
{
    #undef   TPARAM_ALLOCATOR
    #define  TPARAM_ALLOCATOR sfl::test::stateless_alloc_no_prop
    #include "small_flat_map.inc"
}

template <>
void test_small_flat_map<5>()
{
    #undef   TPARAM_ALLOCATOR
    #define  TPARAM_ALLOCATOR sfl::test::stateless_fancy_alloc
    #include "small_flat_map.inc"
}

int main()
{
    test_small_flat_map<1>();
    test_small_flat_map<2>();
    test_small_flat_map<3>();
    test_small_flat_map<4>();
    test_small_flat_map<5>();
}
