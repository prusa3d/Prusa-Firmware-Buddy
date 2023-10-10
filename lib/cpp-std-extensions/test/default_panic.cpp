#include <stdx/ct_string.hpp>
#include <stdx/panic.hpp>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("default panic called with runtime arguments", "[default panic]") {
    stdx::panic("uh-oh");
}

#if __cplusplus >= 202002L
TEST_CASE("default panic called with compile-time strings", "[default panic]") {
    using namespace stdx::ct_string_literals;
    stdx::panic<"uh-oh"_cts>();
}
#endif

TEST_CASE("default panic called through macro", "[default panic]") {
    STDX_PANIC("uh-oh");
}
