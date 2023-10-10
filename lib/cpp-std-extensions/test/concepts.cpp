#include <stdx/concepts.hpp>

#include <catch2/catch_test_macros.hpp>

#include <type_traits>

TEST_CASE("integral", "[concepts]") {
    static_assert(stdx::integral<int>);
    static_assert(not stdx::integral<float>);
}

TEST_CASE("floating_point", "[concepts]") {
    static_assert(stdx::floating_point<float>);
    static_assert(not stdx::floating_point<int>);
}

TEST_CASE("signed_integral", "[concepts]") {
    static_assert(stdx::signed_integral<int>);
    static_assert(not stdx::signed_integral<unsigned int>);
}

TEST_CASE("unsgined_integral", "[concepts]") {
    static_assert(stdx::unsigned_integral<unsigned int>);
    static_assert(not stdx::unsigned_integral<int>);
}

TEST_CASE("same_as", "[concepts]") {
    static_assert(stdx::same_as<int, int>);
    static_assert(not stdx::same_as<float, int>);
}

TEST_CASE("convertible_to", "[concepts]") {
    static_assert(stdx::convertible_to<char, int>);
    static_assert(not stdx::convertible_to<float *, int *>);
}

namespace {
struct S {};
} // namespace

TEST_CASE("equality_comparable", "[concepts]") {
    static_assert(stdx::equality_comparable<int>);
    static_assert(not stdx::equality_comparable<S>);
}

TEST_CASE("totally_ordered", "[concepts]") {
    static_assert(stdx::totally_ordered<int>);
    static_assert(not stdx::totally_ordered<S>);
}

namespace {
struct A {};
struct B : A {};
struct C : private A {};
} // namespace

TEST_CASE("derived_from", "[concepts]") {
    static_assert(stdx::derived_from<A, A>);
    static_assert(stdx::derived_from<B, A>);
    static_assert(not stdx::derived_from<C, A>);
    static_assert(not stdx::derived_from<int, int>);
}

TEST_CASE("invocable", "[concepts]") {
    using invocable_no_args = auto (*)()->void;
    using invocable_int = auto (*)(int)->void;
    [[maybe_unused]] constexpr auto l_invocable_no_args = [] {};
    [[maybe_unused]] constexpr auto l_invocable_int = [](int) {};

    static_assert(not stdx::invocable<int>);
    static_assert(stdx::invocable<invocable_no_args>);
    static_assert(not stdx::invocable<invocable_no_args, int>);
    static_assert(stdx::invocable<invocable_int, int>);
    static_assert(not stdx::invocable<invocable_int>);
    static_assert(stdx::invocable<decltype(l_invocable_no_args)>);
    static_assert(not stdx::invocable<decltype(l_invocable_no_args), int>);
    static_assert(stdx::invocable<decltype(l_invocable_int), int>);
    static_assert(not stdx::invocable<decltype(l_invocable_int)>);
}

TEST_CASE("predicate (negative cases)", "[concepts]") {
    using not_predicate_no_args = auto (*)()->void;
    using not_predicate_int = auto (*)(int)->void;
    [[maybe_unused]] constexpr auto l_not_predicate_no_args = [] {};
    [[maybe_unused]] constexpr auto l_not_predicate_int = [](int) {};

    static_assert(not stdx::predicate<not_predicate_no_args>);
    static_assert(not stdx::predicate<not_predicate_int, int>);
    static_assert(not stdx::predicate<decltype(l_not_predicate_no_args)>);
    static_assert(not stdx::predicate<decltype(l_not_predicate_int), int>);
}

TEST_CASE("predicate (positive cases)", "[concepts]") {
    using predicate_no_args = auto (*)()->bool;
    using predicate_int = auto (*)(int)->bool;
    [[maybe_unused]] constexpr auto l_predicate_no_args = [] { return true; };
    [[maybe_unused]] constexpr auto l_predicate_int = [](int) { return true; };
    [[maybe_unused]] constexpr auto convert_predicate_no_args = [] {
        return std::true_type{};
    };
    [[maybe_unused]] constexpr auto convert_predicate_int = [](int) {
        return std::true_type{};
    };

    static_assert(stdx::predicate<predicate_no_args>);
    static_assert(stdx::predicate<predicate_int, int>);
    static_assert(stdx::predicate<decltype(l_predicate_no_args)>);
    static_assert(stdx::predicate<decltype(l_predicate_int), int>);
    static_assert(stdx::predicate<decltype(convert_predicate_no_args)>);
    static_assert(stdx::predicate<decltype(convert_predicate_int), int>);
}

namespace {
[[maybe_unused]] auto func_no_args() {}
[[maybe_unused]] auto func_one_arg(int) {}

struct funcobj {
    auto operator()() {}
};
struct generic_funcobj {
    template <typename> auto operator()() {}
};
} // namespace

TEST_CASE("callable", "[concepts]") {
    [[maybe_unused]] constexpr auto l_callable_no_args = [] {};
    [[maybe_unused]] constexpr auto l_callable_int = [](int) {};
    [[maybe_unused]] constexpr auto l_callable_generic = [](auto) {};

    static_assert(not stdx::callable<int>);
    static_assert(stdx::callable<decltype(func_no_args)>);
    static_assert(stdx::callable<decltype(func_one_arg)>);
    static_assert(stdx::callable<funcobj>);
    static_assert(stdx::callable<generic_funcobj>);
    static_assert(stdx::callable<decltype(l_callable_no_args)>);
    static_assert(stdx::callable<decltype(l_callable_int)>);
    static_assert(stdx::callable<decltype(l_callable_generic)>);
}
