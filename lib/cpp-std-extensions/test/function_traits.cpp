#include <stdx/function_traits.hpp>
#include <stdx/priority.hpp>

#include <catch2/catch_test_macros.hpp>

#include <tuple>
#include <type_traits>

namespace {
[[maybe_unused]] auto func_no_args() -> void {}
[[maybe_unused]] auto func_one_arg(int) -> int { return 0; }
[[maybe_unused]] auto func_ref_arg(int &) -> void {}
} // namespace

TEST_CASE("function return type", "[function_traits]") {
    static_assert(std::is_void_v<typename stdx::function_traits<
                      decltype(func_no_args)>::return_type>);
    static_assert(std::is_void_v<stdx::return_t<decltype(func_no_args)>>);

    static_assert(
        std::is_same_v<
            typename stdx::function_traits<decltype(func_one_arg)>::return_type,
            int>);
    static_assert(std::is_same_v<stdx::return_t<decltype(func_one_arg)>, int>);
}

TEST_CASE("lambda return type", "[function_traits]") {
    [[maybe_unused]] auto const x = []() {};
    [[maybe_unused]] auto const y = []() mutable {};

    static_assert(std::is_void_v<stdx::return_t<decltype(x)>>);
    static_assert(std::is_void_v<stdx::return_t<decltype(y)>>);
}

TEST_CASE("function args", "[function_traits]") {
    static_assert(
        std::is_same_v<typename stdx::function_traits<
                           decltype(func_no_args)>::template args<std::tuple>,
                       std::tuple<>>);
    static_assert(
        std::is_same_v<stdx::args_t<decltype(func_no_args), std::tuple>,
                       std::tuple<>>);

    static_assert(
        std::is_same_v<typename stdx::function_traits<
                           decltype(func_one_arg)>::template args<std::tuple>,
                       std::tuple<int>>);
    static_assert(
        std::is_same_v<stdx::args_t<decltype(func_one_arg), std::tuple>,
                       std::tuple<int>>);
}

TEST_CASE("function decayed args", "[function_traits]") {
    static_assert(
        std::is_same_v<typename stdx::function_traits<decltype(func_ref_arg)>::
                           template decayed_args<std::tuple>,
                       std::tuple<int>>);
    static_assert(
        std::is_same_v<stdx::decayed_args_t<decltype(func_ref_arg), std::tuple>,
                       std::tuple<int>>);
}

TEST_CASE("lambda args", "[function_traits]") {
    [[maybe_unused]] auto const x = [](int) {};
    [[maybe_unused]] auto const y = [](int) mutable {};

    static_assert(std::is_same_v<typename stdx::function_traits<
                                     decltype(x)>::template args<std::tuple>,
                                 std::tuple<int>>);
    static_assert(
        std::is_same_v<stdx::args_t<decltype(x), std::tuple>, std::tuple<int>>);

    static_assert(std::is_same_v<typename stdx::function_traits<
                                     decltype(y)>::template args<std::tuple>,
                                 std::tuple<int>>);
    static_assert(
        std::is_same_v<stdx::args_t<decltype(y), std::tuple>, std::tuple<int>>);
}

TEST_CASE("lambda decayed args", "[function_traits]") {
    [[maybe_unused]] auto const x = [](int &) {};
    [[maybe_unused]] auto const y = [](int &) mutable {};

    static_assert(
        std::is_same_v<typename stdx::function_traits<
                           decltype(x)>::template decayed_args<std::tuple>,
                       std::tuple<int>>);
    static_assert(std::is_same_v<stdx::decayed_args_t<decltype(x), std::tuple>,
                                 std::tuple<int>>);

    static_assert(
        std::is_same_v<typename stdx::function_traits<
                           decltype(y)>::template decayed_args<std::tuple>,
                       std::tuple<int>>);
    static_assert(std::is_same_v<stdx::decayed_args_t<decltype(y), std::tuple>,
                                 std::tuple<int>>);
}

TEST_CASE("function arity", "[function_traits]") {
    static_assert(stdx::function_traits<decltype(func_no_args)>::arity::value ==
                  0u);
    static_assert(stdx::function_traits<decltype(func_one_arg)>::arity::value ==
                  1u);
    static_assert(stdx::arity_t<decltype(func_no_args)>::value == 0u);
    static_assert(stdx::arity_t<decltype(func_one_arg)>::value == 1u);

    static_assert(stdx::arity_v<decltype(func_no_args)> == 0u);
    static_assert(stdx::arity_v<decltype(func_one_arg)> == 1u);
}

TEST_CASE("lambda arity", "[function_traits]") {
    [[maybe_unused]] auto const x = []() {};
    [[maybe_unused]] auto const y = [](int) {};

    static_assert(stdx::function_traits<decltype(x)>::arity::value == 0u);
    static_assert(stdx::function_traits<decltype(y)>::arity::value == 1u);
    static_assert(stdx::arity_t<decltype(x)>::value == 0u);
    static_assert(stdx::arity_t<decltype(y)>::value == 1u);

    static_assert(stdx::arity_v<decltype(x)> == 0u);
    static_assert(stdx::arity_v<decltype(y)> == 1u);
}

TEST_CASE("generic lambda arity", "[function_traits]") {
    [[maybe_unused]] auto const x = [](auto) {};
    [[maybe_unused]] auto const y = [](auto, auto) {};
    static_assert(stdx::arity_t<decltype(x)>::value == 1u);
    static_assert(stdx::arity_t<decltype(y)>::value == 2u);

    static_assert(stdx::arity_v<decltype(x)> == 1u);
    static_assert(stdx::arity_v<decltype(y)> == 2u);
}

namespace {
bool called_1{};
bool called_2{};

template <typename F>
constexpr auto call_f(F f, stdx::priority_t<1>) -> stdx::return_t<F> {
    called_1 = true;
    return f();
}

template <typename F> constexpr auto call_f(F f, stdx::priority_t<0>) -> void {
    called_2 = true;
    f(0);
}
} // namespace

TEST_CASE("SFINAE friendly", "[function_traits]") {
    called_1 = false;
    called_2 = false;
    auto f1 = []() -> int { return 1; };
    auto f2 = [](auto) -> void {};
    call_f(f1, stdx::priority<1>);
    CHECK(called_1);
    call_f(f2, stdx::priority<1>);
    CHECK(called_2);
}
