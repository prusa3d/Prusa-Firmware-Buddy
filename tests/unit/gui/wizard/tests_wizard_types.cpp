#include "catch2/catch.hpp"

#include "wizard_types.hpp"

TEST_CASE("wizard_types header tests", "[wizard]") {

    SECTION("WizardMask") {
        CHECK(WizardMask(WizardState_t(0)) == uint64_t(1));
        CHECK(WizardMask(WizardState_t::last) == pow(2, int(WizardState_t::last))); // it uses bitshigt internaly, so I use POW
    }

    SECTION("WizardMaskUpTo") {
        CHECK(WizardMaskUpTo(WizardState_t(0)) == uint64_t(1));
        CHECK(WizardMaskUpTo(WizardState_t::last) == pow(2, int(WizardState_t::last)) * 2 - 1);
    }

    SECTION("WizardMaskAdd") {
        uint64_t mask = 0;

        mask = WizardMaskAdd(mask, WizardState_t(0));
        CHECK(mask == uint64_t(1));

        mask = WizardMaskAdd(mask, WizardState_t(1));
        CHECK(mask == uint64_t(3));

        //try again
        mask = WizardMaskAdd(mask, WizardState_t(1));
        CHECK(mask == uint64_t(3));
    }

    SECTION("WizardMaskRange") {
        CHECK(WizardMaskRange(WizardState_t(0), WizardState_t(0)) == WizardMaskUpTo(WizardState_t(0)));
        CHECK(WizardMaskRange(WizardState_t(1), WizardState_t(0)) == WizardMaskUpTo(WizardState_t(1)));
        CHECK(WizardMaskRange(WizardState_t(0), WizardState_t(1)) == WizardMaskUpTo(WizardState_t(1)));
    }
}
