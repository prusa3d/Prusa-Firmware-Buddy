#include <stdx/type_traits.hpp>

#include <catch2/catch_test_macros.hpp>

#include <type_traits>

namespace {
[[maybe_unused]] auto func_no_args() {}
[[maybe_unused]] auto func_one_arg(int) {}

struct S {
    auto operator()() {}
};
struct T {
    template <typename> auto operator()() {}
};
} // namespace

TEST_CASE("is_function", "[type_traits]") {
    auto x = []() {};
    static_assert(not stdx::is_function_v<int>);
    static_assert(not stdx::is_function_v<int &>);
    static_assert(not stdx::is_function_v<decltype(x)>);
    static_assert(stdx::is_function_v<decltype(func_no_args)>);
    static_assert(stdx::is_function_v<decltype(func_one_arg)>);
}

TEST_CASE("is_function_object", "[type_traits]") {
    auto x = []() {};
    auto y = [](int) {};
    auto z = [](auto) {};
    static_assert(not stdx::is_function_object_v<int>);
    static_assert(not stdx::is_function_object_v<decltype(func_no_args)>);
    static_assert(not stdx::is_function_object_v<decltype(func_one_arg)>);
    static_assert(stdx::is_function_object_v<decltype(x)>);
    static_assert(stdx::is_function_object_v<decltype(y)>);
    static_assert(stdx::is_function_object_v<decltype(z)>);
    static_assert(stdx::is_function_object_v<S>);
    static_assert(stdx::is_function_object_v<T>);
}

TEST_CASE("is_callable", "[type_traits]") {
    auto x = []() {};
    auto y = [](int) {};
    auto z = [](auto) {};
    static_assert(not stdx::is_callable_v<int>);
    static_assert(stdx::is_callable_v<decltype(func_no_args)>);
    static_assert(stdx::is_callable_v<decltype(func_one_arg)>);
    static_assert(stdx::is_callable_v<decltype(x)>);
    static_assert(stdx::is_callable_v<decltype(y)>);
    static_assert(stdx::is_callable_v<decltype(z)>);
    static_assert(stdx::is_callable_v<S>);
    static_assert(stdx::is_callable_v<T>);
}
