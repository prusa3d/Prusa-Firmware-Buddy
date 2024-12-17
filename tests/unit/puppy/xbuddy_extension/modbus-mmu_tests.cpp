#include <puppy/xbuddy_extension/modbus.hpp>
#include <puppy/xbuddy_extension/mmu.hpp>

#include <catch2/catch.hpp>

#include <cstring>

using namespace modbus;

// this is a counterpart of mmu2-modbus unit tests:
// - decoding MODBUS requests into MMU requests
// - waiting for MMU response
// - encoding MMU response into MODBUS response
// tbd. later
// TEST_CASE("") {
// }
