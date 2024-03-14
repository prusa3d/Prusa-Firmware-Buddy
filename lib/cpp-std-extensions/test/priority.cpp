#include <stdx/concepts.hpp>
#include <stdx/priority.hpp>

#include <catch2/catch_test_macros.hpp>

#include <type_traits>

namespace {
#if __cpp_concepts >= 201907L
auto select_priority(auto, stdx::priority_t<0>) -> int { return 0; }
auto select_priority(stdx::integral auto, stdx::priority_t<1>) -> int {
    return 1;
}
auto select_priority(stdx::floating_point auto, stdx::priority_t<2>) -> int {
    return 2;
}
#else
template <typename T> auto select_priority(T, stdx::priority_t<0>) -> int {
    return 0;
}
template <typename T, typename = std::enable_if_t<stdx::integral<T>>>
auto select_priority(T, stdx::priority_t<1>) -> int {
    return 1;
}
template <typename T, typename = std::enable_if_t<stdx::floating_point<T>>>
auto select_priority(T, stdx::priority_t<2>) -> int {
    return 2;
}
#endif
} // namespace

TEST_CASE("priority", "[priority]") {
    CHECK(select_priority(1.0f, stdx::priority<2>) == 2);
    CHECK(select_priority(1, stdx::priority<2>) == 1);
    CHECK(select_priority(stdx::priority<2>, stdx::priority<2>) == 0);
}
