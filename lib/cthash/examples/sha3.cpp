#include <cthash/sha3/sha3-256.hpp>

using namespace cthash::literals;

int main() {
    constexpr auto a = cthash::sha3_256 {}.update("hello there!").final();
    // or
    constexpr auto b = cthash::simple<cthash::sha3_256>("hello there!");

    static_assert(a == b);
    static_assert(a == "c7fd85f649fba4bd6fb605038ae8530cf2239152bbbcb9d91d260cc2a90a9fea"_sha3_256);
}
