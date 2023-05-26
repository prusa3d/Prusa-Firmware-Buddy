#include <cthash/sha3/shake128.hpp>

using namespace cthash::literals;

int main() {
    constexpr auto expected = "86089a77e15628597e45caf70c8ef271def6775c54d42d61fb45b9cd6d3b288e5fbd0042241a4aa9180c1bfe94542e16765b3a48d549771202e50aebf8d4f51bd00be2a427f81b7b58aaebc97f89559bca1ea21fec5047de70d075e14e5a3c95c002fd9f81925672d408d4b60c0105e5858df25b64af9b20cec973d66616da813e544b951bfedbc1f2f79214c900be9621611206aa8b5f5727c38372d4410576206a42908800d8520469b4a62fdb6633a4283fcc290d709b48dad426db1fbe6aff9dabfbf2537f0c0bfce550ce1af1269a65022d29e12268da51bee5f1c54c85be5782ccb6996c902ecfa9f6cc28d52d2e5710677fd076751af267405e8c5ae0"_shake128;

    constexpr cthash::shake128_value<32> calculated_32bit = cthash::shake128().update("hello there!").final<32>();
    constexpr cthash::shake128_value<128> calculated_128bit = cthash::shake128().update("hello there!").final<128>();
    constexpr cthash::shake128_value<512> calculated_512bit = cthash::shake128().update("hello there!").final<512>();
    constexpr cthash::shake128_value<1024> calculated_1024bit = cthash::shake128().update("hello there!").final<1024>();
    constexpr cthash::shake128_value<2048> calculated_2048bit = cthash::shake128().update("hello there!").final<2048>();
    constexpr cthash::shake128_value<4096> calculated_4096bit = cthash::shake128().update("hello there!").final<4096>();

    // it only checks common length of both operands
    static_assert(expected == calculated_32bit);
    static_assert(expected == calculated_128bit);
    static_assert(expected == calculated_512bit);
    static_assert(expected == calculated_1024bit);
    static_assert(expected == calculated_2048bit);
    static_assert(expected == calculated_4096bit);
}
