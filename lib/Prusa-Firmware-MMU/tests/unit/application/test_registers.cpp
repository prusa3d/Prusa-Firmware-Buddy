#include "catch2/catch_test_macros.hpp"
#include "registers.h"
#include "modules/globals.h"
#include "../modules/stubs/stub_eeprom.h"
#include <stdint.h>

TEST_CASE("registers::read", "[registers]") {
    uint16_t reg;
    REQUIRE(ReadRegister(0, reg));
    REQUIRE(reg == PROJECT_VERSION_MAJOR);

    REQUIRE(ReadRegister(1, reg));
    REQUIRE(reg == PROJECT_VERSION_MINOR);

    REQUIRE(ReadRegister(2, reg));
    REQUIRE(reg == PROJECT_VERSION_REV);

    REQUIRE(ReadRegister(3, reg));
    REQUIRE(reg == PROJECT_BUILD_NUMBER);

    // set some MMU errors
    hal::eeprom::ClearEEPROM();
    REQUIRE(mg::globals.DriveErrors() == 0);
    mg::globals.IncDriveErrors();
    REQUIRE(ReadRegister(4, reg));
    REQUIRE(reg == 1);
}
