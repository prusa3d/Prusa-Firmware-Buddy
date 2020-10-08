#include "catch2/catch.hpp"

#include "Jogwheel.hpp"
#include "hwio_pindef.h"

namespace buddy::hw {
jogPin jogWheelEN1;
jogPin jogWheelEN2;
jogPin jogWheelENC;
}; // namespace buddy::hw

void __disable_irq() {}
void __enable_irq() {}
uint32_t HAL_GetTick() { return 0; }

TEST_CASE("Jogwheel tests", "[jogwheel]") {
    SECTION("j") {
        CHECK(1 == 1);
    }
}
