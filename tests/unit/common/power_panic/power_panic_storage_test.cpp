#include <catch2/catch_test_macros.hpp>

#include <common/power_panic_storage.hpp>
#include <common/w25x.hpp>
#include <version/version.hpp>

const version::BuildIdentification version::project_build_identification {
    .commit_hash = "0123456789abcdef0123456789abcdef01234567",
    .project_version_full = "9.9.9-UNITTEST",
    .enabled_translations = 0,
    .printer_code = 12,
    .commit_dirty = false,
    .has_bootloader = false,
};

namespace power_panic {

// mirrors the declaration in src/common/power_panic.hpp (not includable on host)
bool state_stored();

runtime_state_t runtime_state {};

} // namespace power_panic

bed_mesh_t unified_bed_leveling::z_values {};

TEST_CASE("power_panic: state_stored") {
    auto &flash = W25xFlash::instance();
    flash.reset();

    SECTION("erased region") {
        REQUIRE_FALSE(power_panic::state_stored());
    }

    power_panic::erase();
    power_panic::fixed_t::save();

    SECTION("header stamped, validity flag never written") {
        REQUIRE_FALSE(power_panic::state_stored());
    }

    power_panic::state_t::save();

    SECTION("saved") {
        REQUIRE(power_panic::state_stored());
    }

    SECTION("erased after save") {
        power_panic::erase();
        REQUIRE_FALSE(power_panic::state_stored());
    }

    SECTION("read error") {
        flash.sim_flash.inject_errors();
        REQUIRE_FALSE(power_panic::state_stored());
    }

    SECTION("corrupted header magic") {
        flash.sim_flash.byte(W25xFlash::pp_start_address) ^= 0xFF;
        REQUIRE_FALSE(power_panic::state_stored());
    }
}
