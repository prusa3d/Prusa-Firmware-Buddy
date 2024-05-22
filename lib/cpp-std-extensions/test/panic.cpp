#include <stdx/ct_string.hpp>
#include <stdx/panic.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <string_view>

namespace {
bool runtime_call{};
#if __cplusplus >= 202002L
bool compile_time_call{};
#endif

struct injected_handler {
    template <typename Why, typename... Ts>
    static auto panic(Why why, Ts &&...) noexcept -> void {
        CHECK(std::string_view{why} == "uh-oh");
        runtime_call = true;
    }

#if __cplusplus >= 202002L
    template <stdx::ct_string Why, typename... Ts>
    static auto panic(Ts &&...) noexcept -> void {
        static_assert(std::string_view{Why} == "uh-oh");
        compile_time_call = true;
    }
#endif
};
} // namespace

template <> inline auto stdx::panic_handler<> = injected_handler{};

TEST_CASE("panic called with runtime arguments", "[panic]") {
    runtime_call = false;
    stdx::panic("uh-oh");
    CHECK(runtime_call);
}

#if __cplusplus >= 202002L
TEST_CASE("panic called with compile-time strings", "[panic]") {
    compile_time_call = false;
    using namespace stdx::ct_string_literals;
    stdx::panic<"uh-oh"_cts>();
    CHECK(compile_time_call);
}

TEST_CASE("compile-time panic called through macro", "[panic]") {
    compile_time_call = false;
    STDX_PANIC("uh-oh");
    CHECK(compile_time_call);
}
#else
TEST_CASE("runtime panic called through macro", "[panic]") {
    runtime_call = false;
    STDX_PANIC("uh-oh");
    CHECK(runtime_call);
}
#endif
