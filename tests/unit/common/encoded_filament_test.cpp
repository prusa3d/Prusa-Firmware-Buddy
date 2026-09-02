#include <catch2/catch_test_macros.hpp>
#include <config_store/filament_variant_decode.hpp>
#include <encoded_filament.hpp>

using config_store_ns::migrations::filament_type_from_variant_bytes;

TEST_CASE("from_variant_bytes: NoFilamentType (discriminant 0)", "[encoded_filament]") {
    // discriminant 0 = NoFilamentType, value byte doesn't matter
    auto result = filament_type_from_variant_bytes(0, 0);
    REQUIRE(result == FilamentType(NoFilamentType {}));

    // Value byte is ignored for NoFilamentType (it's an empty struct)
    result = filament_type_from_variant_bytes(0, 42);
    REQUIRE(result == FilamentType(NoFilamentType {}));
}

TEST_CASE("from_variant_bytes: PresetFilamentType (discriminant 1)", "[encoded_filament]") {
    auto result = filament_type_from_variant_bytes(1, 0);
    REQUIRE(result == FilamentType(PresetFilamentType::PLA));

    result = filament_type_from_variant_bytes(1, 5);
    REQUIRE(result == FilamentType(PresetFilamentType::ABS));
}

TEST_CASE("from_variant_bytes: UserFilamentType (discriminant 2)", "[encoded_filament]") {
    auto result = filament_type_from_variant_bytes(2, 0);
    REQUIRE(result == FilamentType(UserFilamentType { 0 }));

    result = filament_type_from_variant_bytes(2, 3);
    REQUIRE(result == FilamentType(UserFilamentType { 3 }));
}

TEST_CASE("from_variant_bytes: AdHocFilamentType (discriminant 3)", "[encoded_filament]") {
    auto result = filament_type_from_variant_bytes(3, 0);
    REQUIRE(result == FilamentType(AdHocFilamentType { VirtualToolIndex::from_raw(0) }));

    result = filament_type_from_variant_bytes(3, 2);
    REQUIRE(result == FilamentType(AdHocFilamentType { VirtualToolIndex::from_raw(2) }));
}

TEST_CASE("from_variant_bytes: PendingAdHocFilamentType (discriminant 4)", "[encoded_filament]") {
    // discriminant 4 = PendingAdHocFilamentType, should map to none
    auto result = filament_type_from_variant_bytes(4, 0);
    REQUIRE(result == FilamentType(NoFilamentType {}));
}

TEST_CASE("from_variant_bytes: unknown discriminant", "[encoded_filament]") {
    auto result = filament_type_from_variant_bytes(255, 42);
    REQUIRE(result == FilamentType(NoFilamentType {}));
}
