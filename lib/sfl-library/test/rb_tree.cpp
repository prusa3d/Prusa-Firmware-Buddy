//
// g++ -std=c++11 -g -O0 -Wall -Wextra -Wpedantic -Werror -Wfatal-errors -I ../include rb_tree.cpp
// valgrind --leak-check=full ./a.out
//

#undef NDEBUG // This is very important. Must be in the first line.

#define SFL_TEST_RB_TREE
#include "sfl/detail/rb_tree.hpp"

#include "sfl/detail/functional.hpp"

#include "check.hpp"
#include "nth.hpp"
#include "print.hpp"

#include "xint.hpp"
#include "xobj.hpp"

#include "statefull_alloc.hpp"
#include "stateless_alloc.hpp"
#include "stateless_alloc_no_prop.hpp"
#include "stateless_fancy_alloc.hpp"
#include "partially_propagable_alloc.hpp"

#include <vector>

template <>
void test_rb_tree<1>()
{
    #undef   TPARAM_ALLOCATOR
    #define  TPARAM_ALLOCATOR std::allocator
    #include "rb_tree.inc"
}

template <>
void test_rb_tree<2>()
{
    #undef   TPARAM_ALLOCATOR
    #define  TPARAM_ALLOCATOR sfl::test::statefull_alloc
    #include "rb_tree.inc"
}

template <>
void test_rb_tree<3>()
{
    #undef   TPARAM_ALLOCATOR
    #define  TPARAM_ALLOCATOR sfl::test::stateless_alloc
    #include "rb_tree.inc"
}

template <>
void test_rb_tree<4>()
{
    #undef   TPARAM_ALLOCATOR
    #define  TPARAM_ALLOCATOR sfl::test::stateless_alloc_no_prop
    #include "rb_tree.inc"
}

template <>
void test_rb_tree<5>()
{
    #undef   TPARAM_ALLOCATOR
    #define  TPARAM_ALLOCATOR sfl::test::stateless_fancy_alloc
    #include "rb_tree.inc"
}

template <>
void test_rb_tree<6>()
{
    #undef   TPARAM_ALLOCATOR
    #define  TPARAM_ALLOCATOR sfl::test::partially_propagable_alloc
    #include "rb_tree.inc"
}

int main()
{
    test_rb_tree<1>();
    test_rb_tree<2>();
    test_rb_tree<3>();
    test_rb_tree<4>();
    test_rb_tree<5>();
    test_rb_tree<6>();
}
