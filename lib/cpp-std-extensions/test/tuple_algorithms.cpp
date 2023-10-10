#include "detail/tuple_types.hpp"

#include <stdx/tuple_algorithms.hpp>

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <functional>
#include <type_traits>
#include <utility>

TEST_CASE("unary transform", "[tuple_algorithms]") {
    static_assert(stdx::transform([](auto) { return 1; }, stdx::tuple{}) ==
                  stdx::tuple{});
    constexpr auto t = stdx::tuple{1, 2, 3};
    constexpr auto u = stdx::transform([](auto x) { return x + 1; }, t);
    static_assert(u == stdx::tuple{2, 3, 4});
}

TEST_CASE("unary type transform", "[tuple_algorithms]") {
    static_assert(stdx::transform([](auto) { return 1; }, stdx::tuple{}) ==
                  stdx::tuple{});
    constexpr auto t = stdx::tuple{1, 0, 3};
    constexpr auto u =
        stdx::transform([](auto x) -> bool { return x != 0; }, t);
    static_assert(u == stdx::tuple{true, false, true});
}

TEST_CASE("n-ary transform", "[tuple_algorithms]") {
    static_assert(stdx::transform([](auto, auto) { return 1; }, stdx::tuple{},
                                  stdx::tuple{}) == stdx::tuple{});
    constexpr auto t = stdx::tuple{1, 2, 3};
    constexpr auto u =
        stdx::transform([](auto x, auto y) { return x + y; }, t, t);
    static_assert(u == stdx::tuple{2, 4, 6});
}

TEST_CASE("rvalue transform", "[tuple_algorithms]") {
    auto t = stdx::tuple{1, 2, 3};
    auto const u = stdx::transform([](int &&x) { return x + 1; }, std::move(t));
    CHECK(u == stdx::tuple{2, 3, 4});
}

TEST_CASE("transform preserves references", "[tuple_algorithms]") {
    int value{1};
    auto const u = stdx::transform(
        [&](auto i) -> int & {
            value += i;
            return value;
        },
        stdx::tuple{1});
    CHECK(std::addressof(value) == std::addressof(u[stdx::index<0>]));
}

namespace {
template <typename Key, typename Value> struct map_entry {
    using key_t = Key;
    using value_t = Value;

    value_t value;
};
template <typename T> using key_for = typename T::key_t;
} // namespace

TEST_CASE("transform with index", "[tuple_algorithms]") {
    struct X;
    constexpr auto t = stdx::transform<key_for>(
        [](auto value) { return map_entry<X, int>{value}; }, stdx::tuple{42});
    static_assert(stdx::get<X>(t).value == 42);
}

TEST_CASE("apply", "[tuple_algorithms]") {
    static_assert(stdx::apply([](auto... xs) { return (0 + ... + xs); },
                              stdx::tuple{}) == 0);
    static_assert(stdx::apply([](auto... xs) { return (0 + ... + xs); },
                              stdx::tuple{1, 2, 3}) == 6);

    auto stateful = [calls = 0](auto...) mutable { return ++calls; };
    CHECK(stdx::apply(stateful, stdx::tuple{1, 2, 3}) == 1);
    CHECK(stdx::apply(stateful, stdx::tuple{1, 2, 3}) == 2);

    static_assert(stdx::apply([](auto x) { return x.value; },
                              stdx::tuple{move_only{42}}) == 42);

    auto t = stdx::tuple{1, 2, 3};
    stdx::apply([](auto &...xs) { (++xs, ...); }, t);
    CHECK(t == stdx::tuple{2, 3, 4});
}

TEST_CASE("join", "[tuple_algorithms]") {
    constexpr auto t = stdx::tuple{1, 2, 3};
    static_assert(t.join(std::plus{}) == 6);
    static_assert(stdx::tuple{1, 2, 3}.join(std::plus{}) == 6);
    static_assert(
        stdx::tuple{move_only{42}}
            .join([](auto x, auto y) { return move_only{x.value + y.value}; })
            .value == 42);
}

TEST_CASE("for_each", "[tuple_algorithms]") {
    {
        auto const t = stdx::tuple{};
        auto sum = 0;
        stdx::for_each([&](auto x, auto y) { sum += x + y; }, t, t);
        CHECK(sum == 0);
    }
    {
        auto const t = stdx::tuple{1, 2, 3};
        auto sum = 0;
        stdx::for_each([&](auto x, auto y) { sum += x + y; }, t, t);
        CHECK(sum == 12);
    }
    {
        auto const t = stdx::tuple{1};
        auto sum = 0;
        stdx::for_each([&](auto x, auto &&y) { sum += x + y.value; }, t,
                       stdx::tuple{move_only{2}});
        CHECK(sum == 3);
    }
    {
        auto const t = stdx::tuple{1, 2, 3};
        auto f = stdx::for_each(
            [calls = 0](auto) mutable {
                ++calls;
                return calls;
            },
            t);
        CHECK(f(0) == 4);
    }
}

TEST_CASE("tuple_cat", "[tuple_algorithms]") {
    static_assert(stdx::tuple_cat() == stdx::tuple{});
    static_assert(stdx::tuple_cat(stdx::tuple{}, stdx::tuple{}) ==
                  stdx::tuple{});
    static_assert(stdx::tuple_cat(stdx::tuple{1, 2}, stdx::tuple{}) ==
                  stdx::tuple{1, 2});
    static_assert(stdx::tuple_cat(stdx::tuple{}, stdx::tuple{1, 2}) ==
                  stdx::tuple{1, 2});
    static_assert(stdx::tuple_cat(stdx::tuple{1, 2}, stdx::tuple{3, 4}) ==
                  stdx::tuple{1, 2, 3, 4});
    static_assert(stdx::tuple_cat(stdx::tuple{1, 2}, stdx::tuple{3, 4},
                                  stdx::tuple{5, 6}) ==
                  stdx::tuple{1, 2, 3, 4, 5, 6});
    static_assert(
        stdx::tuple_cat(stdx::tuple{1, 2}, stdx::tuple{}, stdx::tuple{3, 4}) ==
        stdx::tuple{1, 2, 3, 4});

    auto t = stdx::tuple_cat(stdx::tuple{1}, stdx::tuple{2});
    static_assert(std::is_same_v<decltype(t), stdx::tuple<int, int>>);
}

TEST_CASE("tuple_cat (move only)", "[tuple_algorithms]") {
    auto t =
        stdx::tuple_cat(stdx::tuple{move_only{5}}, stdx::tuple{move_only{10}});
    static_assert(
        std::is_same_v<decltype(t), stdx::tuple<move_only, move_only>>);
    CHECK(t == stdx::tuple{move_only{5}, move_only{10}});
}

TEST_CASE("tuple_cat (references)", "[tuple_algorithms]") {
    auto x = 1;
    auto t = stdx::tuple_cat(stdx::tuple<int &>{x}, stdx::tuple<int &>{x});
    static_assert(std::is_same_v<decltype(t), stdx::tuple<int &, int &>>);
    stdx::get<0>(t) = 2;
    CHECK(x == 2);
    stdx::get<1>(t) = 1;
    CHECK(x == 1);
}

TEST_CASE("tuple_cat (const references)", "[tuple_algorithms]") {
    auto x = 1;
    auto t = stdx::tuple_cat(stdx::tuple<int const &>{x},
                             stdx::tuple<int const &>{x});
    static_assert(
        std::is_same_v<decltype(t), stdx::tuple<int const &, int const &>>);
    x = 2;
    CHECK(stdx::get<0>(t) == 2);
    CHECK(stdx::get<1>(t) == 2);
}

TEST_CASE("tuple_cat (rvalue references)", "[tuple_algorithms]") {
    auto x = 1;
    auto y = 2;
    auto t = stdx::tuple_cat(stdx::tuple<int &&>{std::move(x)},
                             stdx::tuple<int &&>{std::move(y)});
    static_assert(std::is_same_v<decltype(t), stdx::tuple<int &&, int &&>>);
    x = 2;
    CHECK(stdx::get<0>(t) == 2);
    y = 2;
    CHECK(stdx::get<1>(t) == 2);
}

TEST_CASE("fold_left", "[tuple_algorithms]") {
    constexpr auto t = stdx::tuple{1, 2, 3};
    static_assert(t.fold_left(0, std::minus{}) == (((0 - 1) - 2) - 3));
    static_assert(stdx::tuple{move_only{1}, move_only{2}, move_only{3}}
                      .fold_left(move_only{0},
                                 [](move_only &&x, move_only &&y) {
                                     return move_only{x.value + y.value};
                                 })
                      .value == 6);

    int calls{};
    auto stateful = [&](auto x, auto y) mutable {
        ++calls;
        return x + y;
    };
    CHECK(t.fold_left(0, stateful) == 6);
    CHECK(calls == 3);
}

TEST_CASE("fold_left (heterogeneous types in fold)", "[tuple_algorithms]") {
    constexpr auto t = stdx::tuple{1, 2, 3};
    static_assert(t.fold_left(stdx::tuple{}, [](auto acc, auto n) {
        return stdx::tuple_cat(acc, stdx::tuple{n});
    }) == t);

    static_assert(
        stdx::tuple{1, 2, 3}.fold_left(stdx::tuple{}, [](auto acc, auto n) {
            return stdx::tuple_cat(acc, stdx::tuple{n});
        }) == t);
}

template <auto N> struct addend {
    constexpr friend auto operator==(addend, addend) -> bool { return true; }
};
template <auto X, auto Y> constexpr auto operator+(addend<X>, addend<Y>) {
    return addend<X + Y>{};
}

TEST_CASE("fold_left (heterogeneous types in tuple)", "[tuple_algorithms]") {
    constexpr auto t = stdx::tuple{addend<1>{}, addend<2>{}};
    static_assert(t.fold_left(addend<0>{}, std::plus{}) == addend<3>{});
}

TEST_CASE("fold_right", "[tuple_algorithms]") {
    constexpr auto t = stdx::tuple{1, 2, 3};
    static_assert(t.fold_right(4, std::minus{}) == (1 - (2 - (3 - 4))));
    static_assert(stdx::tuple{move_only{1}, move_only{2}, move_only{3}}
                      .fold_right(move_only{0},
                                  [](move_only &&x, move_only &&y) {
                                      return move_only{x.value + y.value};
                                  })
                      .value == 6);

    int calls{};
    auto stateful = [&](auto x, auto y) mutable {
        ++calls;
        return x + y;
    };
    CHECK(t.fold_right(0, stateful) == 6);
    CHECK(calls == 3);
}

TEST_CASE("fold_right (heterogeneous types in fold)", "[tuple_algorithms]") {
    constexpr auto t = stdx::tuple{1, 2, 3};
    static_assert(t.fold_right(stdx::tuple{}, [](auto n, auto acc) {
        return stdx::tuple_cat(stdx::tuple{n}, acc);
    }) == t);

    static_assert(
        stdx::tuple{1, 2, 3}.fold_right(stdx::tuple{}, [](auto n, auto acc) {
            return stdx::tuple_cat(stdx::tuple{n}, acc);
        }) == t);
}

TEST_CASE("fold_right (heterogeneous types in tuple)", "[tuple_algorithms]") {
    constexpr auto t = stdx::tuple{addend<1>{}, addend<2>{}};
    static_assert(t.fold_right(addend<0>{}, std::plus{}) == addend<3>{});
}

template <typename T> struct is_even {
    constexpr static auto value = T::value % 2 == 0;
};

TEST_CASE("filter", "[tuple_algorithms]") {
    constexpr auto t = stdx::tuple{
        std::integral_constant<int, 1>{}, std::integral_constant<int, 2>{},
        std::integral_constant<int, 3>{}, std::integral_constant<int, 4>{}};
    constexpr auto u = stdx::filter<is_even>(t);
    static_assert(u == stdx::tuple{std::integral_constant<int, 2>{},
                                   std::integral_constant<int, 4>{}});
}

TEST_CASE("copy/move behavior for tuple_cat", "[tuple_algorithms]") {
    auto t1 = stdx::tuple{counter{}};
    auto t2 = stdx::tuple{counter{}};

    counter::reset();
    [[maybe_unused]] auto t3 = stdx::tuple_cat(t1, t2);
    CHECK(counter::moves == 0);
    CHECK(counter::copies == 2);

    [[maybe_unused]] auto t4 = stdx::tuple_cat(std::move(t1), std::move(t2));
    CHECK(counter::moves == 2);
    CHECK(counter::copies == 2);
}

template <typename> using always_true = std::true_type;

TEST_CASE("copy/move behavior for filter", "[tuple_algorithms]") {
    auto t1 = stdx::tuple{counter{}};

    counter::reset();
    [[maybe_unused]] auto t2 = stdx::filter<always_true>(t1);
    CHECK(counter::moves == 0);
    CHECK(counter::copies == 1);

    [[maybe_unused]] auto t3 = stdx::filter<always_true>(std::move(t1));
    CHECK(counter::moves == 1);
    CHECK(counter::copies == 1);
}

TEST_CASE("all_of", "[tuple_algorithms]") {
    constexpr auto t = stdx::tuple{1, 2, 3};
    static_assert(stdx::all_of([](auto n) { return n > 0; }, t));
    static_assert(
        stdx::all_of([](auto x, auto y) { return (x + y) % 2 == 0; }, t, t));
}

TEST_CASE("any_of", "[tuple_algorithms]") {
    constexpr auto t = stdx::tuple{1, 2, 3};
    static_assert(stdx::any_of([](auto n) { return n % 2 == 0; }, t));
    static_assert(
        stdx::any_of([](auto x, auto y) { return (x + y) % 2 == 0; }, t, t));
}

TEST_CASE("none_of", "[tuple_algorithms]") {
    constexpr auto t = stdx::tuple{1, 3, 5};
    static_assert(stdx::none_of([](auto n) { return n % 2 == 0; }, t));
    static_assert(
        stdx::none_of([](auto x, auto y) { return (x + y) % 2 != 0; }, t, t));
}

TEST_CASE("contains_type", "[tuple_algorithms]") {
    using T = stdx::tuple<int, bool, int &>;
    static_assert(stdx::contains_type<T, int>);
    static_assert(stdx::contains_type<T, bool>);
    static_assert(stdx::contains_type<T, int &>);
    static_assert(not stdx::contains_type<T, float>);
}

TEST_CASE("contains_type (indexed)", "[tuple_algorithms]") {
    struct X;
    struct Y;
    using T = stdx::indexed_tuple<stdx::detail::index_function_list<key_for>,
                                  map_entry<X, int>, map_entry<Y, int>>;

    static_assert(stdx::contains_type<T, map_entry<X, int>>);
    static_assert(stdx::contains_type<T, X>);
    static_assert(not stdx::contains_type<T, float>);
}

TEST_CASE("sort (empty tuple)", "[tuple_algorithms]") {
    constexpr auto t = stdx::tuple{};
    [[maybe_unused]] constexpr auto sorted = stdx::sort(t);
    static_assert(std::is_same_v<decltype(sorted), stdx::tuple<> const>);
}

TEST_CASE("sort", "[tuple_algorithms]") {
    constexpr auto t = stdx::tuple{1, 1.0, true};
    static_assert(
        std::is_same_v<decltype(t), stdx::tuple<int, double, bool> const>);
    constexpr auto sorted = stdx::sort(t);
    static_assert(
        std::is_same_v<decltype(sorted), stdx::tuple<bool, double, int> const>);
    CHECK(sorted == stdx::tuple{true, 1.0, 1});
}

TEST_CASE("chunk (empty tuple)", "[tuple_algorithms]") {
    constexpr auto t = stdx::tuple{};
    [[maybe_unused]] constexpr auto chunked = stdx::chunk(t);
    static_assert(std::is_same_v<decltype(chunked), stdx::tuple<> const>);
}

TEST_CASE("chunk (1-element tuple)", "[tuple_algorithms]") {
    constexpr auto t = stdx::tuple{1};
    constexpr auto chunked = stdx::chunk(t);
    static_assert(
        std::is_same_v<decltype(chunked), stdx::tuple<stdx::tuple<int>> const>);
    CHECK(chunked == stdx::tuple{stdx::tuple{1}});
}

TEST_CASE("count chunks", "[tuple_algorithms]") {
    static_assert(stdx::detail::count_chunks<stdx::tuple<int, int>>() == 1);
    static_assert(stdx::detail::count_chunks<stdx::tuple<int, float>>() == 2);
    static_assert(stdx::detail::count_chunks<stdx::tuple<int, int, float>>() ==
                  2);
    static_assert(
        stdx::detail::count_chunks<stdx::tuple<int, float, float>>() == 2);
}

TEST_CASE("create chunks", "[tuple_algorithms]") {
    static_assert(stdx::detail::create_chunks<stdx::tuple<int, int>>() ==
                  std::array{stdx::detail::chunk{0, 2}});
    static_assert(
        stdx::detail::create_chunks<stdx::tuple<int, int, float>>() ==
        std::array{stdx::detail::chunk{0, 2}, stdx::detail::chunk{2, 1}});
    static_assert(
        stdx::detail::create_chunks<
            stdx::tuple<int, int, float, int, int, float>>() ==
        std::array{stdx::detail::chunk{0, 2}, stdx::detail::chunk{2, 1},
                   stdx::detail::chunk{3, 2}, stdx::detail::chunk{5, 1}});
}

TEST_CASE("chunk (general case)", "[tuple_algorithms]") {
    constexpr auto t = stdx::tuple{1, 2, 3, 1.0, 2.0, true};
    constexpr auto chunked = stdx::chunk(t);
    static_assert(
        std::is_same_v<
            decltype(chunked),
            stdx::tuple<stdx::tuple<int, int, int>, stdx::tuple<double, double>,
                        stdx::tuple<bool>> const>);
    CHECK(chunked == stdx::tuple{stdx::tuple{1, 2, 3}, stdx::tuple{1.0, 2.0},
                                 stdx::tuple{true}});
}
