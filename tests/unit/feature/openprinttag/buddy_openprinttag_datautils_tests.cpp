#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <vector>

#include <feature/openprinttag/data_utils.hpp>
#include <utils/byte_utils.hpp>

using namespace buddy::openprinttag;

void Request::issue() {
}

const ToolTag tool_tag { VirtualToolIndex::from_raw(0), 10 };

template <CField auto field>
auto stub_field(typename ReadFieldRequest<field>::Value val) {
    return std::pair { tool_tag.field(field), val };
}

Request::~Request() {}

void Request::set_finished(std::expected<std::monostate, Error> result) {
    assert(!finished_);
    finished_ = true;
    error_ = result.error_or(Error::_cnt);
}

void ReadEnumArrayRequestBase::complete(Bytes event_data) {}
void ReadEnumFieldRequestBase::complete(Bytes event_data) {}
void ReadFloatFieldRequest::complete(Bytes event_data) {}
void ReadInt32FieldRequest::complete(Bytes event_data) {}
void ReadStringRequestBase::complete(Bytes event_data) {}
void ReadColorFieldRequest::complete(Bytes event_data) {}

Request::SerializeResult TagRequest::serialize(ManagerNoLockBadge badge, RequestID request_id, anfc::modbus::Request &request) {
    return std::nullopt;
}

void ReadEnumArrayRequestBase::serialize(RequestID, TagID, anfc::modbus::Request &) {}
void ReadEnumFieldRequestBase::serialize(RequestID, TagID, anfc::modbus::Request &) {}
void ReadFloatFieldRequest::serialize(RequestID, TagID, anfc::modbus::Request &) {}
void ReadInt32FieldRequest::serialize(RequestID, TagID, anfc::modbus::Request &) {}
void ReadStringRequestBase::serialize(RequestID, TagID, anfc::modbus::Request &) {}
void ReadColorFieldRequest::serialize(RequestID, TagID, anfc::modbus::Request &) {}

std::optional<ToolTag> ToolTag::for_tool_ephemeral(VirtualToolIndex tool) {
    return std::nullopt;
}

std::optional<ToolTag> ToolTag::for_tool_assigned(VirtualToolIndex tool) {
    return std::nullopt;
}

FilamentTypeParameters FilamentType::parameters() const {
    return std::visit([]<typename T>(const T &v) -> FilamentTypeParameters {
        if constexpr (std::is_same_v<T, PresetFilamentType>) {
            return preset_filament_parameters[v];
        } else {
            return FilamentTypeParameters {};
        }
    },
        *this);
}

TEST_CASE("buddy::openprinttag::data_utils::AmountsInfo") {
    // If the requirements change, we probably want to update the tests
    static_assert(AmountsInfo::Requirements::size == 7);

    SECTION("Nominal only (fresh spool)") {
        stub_data = StubData {
            stub_field<MainField::nominal_netto_full_weight>(1000.0f),
            stub_field<MainField::nominal_full_length>(330000.0f),
        };

        MultiReadFieldRequest<AmountsInfo::Requirements {}> req { tool_tag };
        req.issue();

        AmountsInfo info { req };

        // Consumed defaults to 0 if full weight is present but consumed is missing
        CHECK(info.full_weight_g == 1000.0f);
        CHECK(info.remaining_weight_g == 1000.0f);

        CHECK(info.full_length_mm == 330000.0f);
        CHECK(info.remaining_length_mm == 330000.0f);
    }

    SECTION(" Actual overrides Nominal, and consumed is subtracted") {
        stub_data = StubData {
            stub_field<MainField::nominal_netto_full_weight>(1000.0f),
            stub_field<MainField::actual_netto_full_weight>(950.0f),
            stub_field<MainField::nominal_full_length>(330000.0f),
            stub_field<MainField::actual_full_length>(320000.0f),
            stub_field<AuxField::consumed_weight>(150.0f),
        };

        MultiReadFieldRequest<AmountsInfo::Requirements {}> req { tool_tag };
        req.issue();

        AmountsInfo info { req };

        CHECK(info.full_weight_g == 950.0f);
        CHECK(info.remaining_weight_g == 800.0f); // 950 - 150

        CHECK(info.full_length_mm == 320000.0f);
        CHECK(info.remaining_length_mm == 320000.0f / 950.0f * 800.0f);
    }

    SECTION(" Missing critical data") {
        stub_data = StubData {
            stub_field<AuxField::consumed_weight>(50.0f),
            stub_field<MainField::nominal_full_length>(330000.0f),
            // Missing full weights
        };

        MultiReadFieldRequest<AmountsInfo::Requirements {}> req { tool_tag };
        req.issue();

        AmountsInfo info { req };

        CHECK(!info.full_weight_g.has_value());
        CHECK(!info.remaining_weight_g.has_value());

        // Length alone is known, but without the weight ratio the remaining length cannot be derived
        CHECK(info.full_length_mm == 330000.0f);
        CHECK(!info.remaining_length_mm.has_value());
    }

    SECTION("Length derived from density and filament diameter when full length is absent") {
        stub_data = StubData {
            stub_field<MainField::nominal_netto_full_weight>(1000.0f),
            stub_field<AuxField::consumed_weight>(200.0f),
            stub_field<MainField::density>(1.24f), // g/cm³
            stub_field<MainField::filament_diameter>(1.75f), // mm
            // No full length present -> fall back to density/diameter
        };

        MultiReadFieldRequest<AmountsInfo::Requirements {}> req { tool_tag };
        req.issue();

        AmountsInfo info { req };

        CHECK(info.full_weight_g == 1000.0f);
        CHECK(info.remaining_weight_g == 800.0f); // 1000 - 200

        // length = weight / (density * π * r²); expressed here in independent units
        // (kg, m) to avoid tracking the implementation's exact float rounding.
        const float r_m = (1750e-6f) / 2.0f; // µm -> m, radius
        const float density_kg_per_m3 = 1.24f * 1000.0f; // g/cm³ -> kg/m³
        const float lin_density_kg_per_m = density_kg_per_m3 * std::numbers::pi_v<float> * r_m * r_m;
        const float full_length_m = (1000.0f / 1000.0f) / lin_density_kg_per_m; // 1 kg
        const float remaining_length_m = (800.0f / 1000.0f) / lin_density_kg_per_m;

        REQUIRE(info.full_length_mm.has_value());
        REQUIRE(info.remaining_length_mm.has_value());
        CHECK_THAT(*info.full_length_mm, Catch::Matchers::WithinRel(full_length_m * 1000.0f, 1e-4f));
        CHECK_THAT(*info.remaining_length_mm, Catch::Matchers::WithinRel(remaining_length_m * 1000.0f, 1e-4f));
    }

    SECTION("Density fallback agrees with the length-ratio path for a physically consistent spool") {
        // A real spool's stored length and its density/geometry describe the same filament,
        // so the two independent derivations must yield the same remaining length (up to float rounding).
        constexpr float full_weight = 1000.0f;
        constexpr float consumed = 200.0f;
        constexpr float density = 1.24f; // g/cm³
        constexpr float diameter_mm = 1.75f;

        // The length such a spool physically has, matching density+geometry.
        const float density_mm = density / 1000;
        const float r_mm = diameter_mm * 0.5f;
        const float lin_density = density_mm * std::numbers::pi_v<float> * r_mm * r_mm;
        const float consistent_full_length = full_weight / lin_density;

        // Path A: length-ratio (full_length present)
        stub_data = StubData {
            stub_field<MainField::nominal_netto_full_weight>(full_weight),
            stub_field<AuxField::consumed_weight>(consumed),
            stub_field<MainField::nominal_full_length>(consistent_full_length),
            stub_field<MainField::density>(density),
            stub_field<MainField::filament_diameter>(diameter_mm),
        };
        MultiReadFieldRequest<AmountsInfo::Requirements {}> req_ratio { tool_tag };
        req_ratio.issue();
        const AmountsInfo info_ratio { req_ratio };

        // Path B: density fallback (no full_length)
        stub_data = StubData {
            stub_field<MainField::nominal_netto_full_weight>(full_weight),
            stub_field<AuxField::consumed_weight>(consumed),
            stub_field<MainField::density>(density),
            stub_field<MainField::filament_diameter>(diameter_mm),
        };
        MultiReadFieldRequest<AmountsInfo::Requirements {}> req_density { tool_tag };
        req_density.issue();
        const AmountsInfo info_density { req_density };

        REQUIRE(info_ratio.remaining_length_mm.has_value());
        REQUIRE(info_density.remaining_length_mm.has_value());

        // Same physical answer, only float-rounding apart.
        CHECK_THAT(*info_density.remaining_length_mm,
            Catch::Matchers::WithinRel(*info_ratio.remaining_length_mm, 1e-4f));
    }

    SECTION("Malicious data does not break the density fallback") {
        // Zero / negative density or diameter must not produce inf/NaN or a division by zero.
        const auto build = [](float density, float diameter_mm) {
            stub_data = StubData {
                stub_field<MainField::nominal_netto_full_weight>(1000.0f),
                stub_field<AuxField::consumed_weight>(100.0f),
                stub_field<MainField::density>(density),
                stub_field<MainField::filament_diameter>(diameter_mm),
            };
            MultiReadFieldRequest<AmountsInfo::Requirements {}> req { tool_tag };
            req.issue();
            return AmountsInfo { req };
        };

        // Weight is still valid, so those are populated; length must simply be absent.
        for (const auto &info : {
                 build(0.0f, 1.75f), // zero density
                 build(1.24f, 0), // zero diameter
                 build(-1.24f, 1.75f), // negative density
                 build(1.24f, -1.75f), // negative diameter
             }) {
            CHECK(info.full_weight_g == 1000.0f);
            CHECK(info.remaining_weight_g == 900.0f);
            CHECK(!info.full_length_mm.has_value());
            CHECK(!info.remaining_length_mm.has_value());
        }
    }

    SECTION("Full length takes precedence over the density fallback") {
        // When both are available, the stored-length ratio path must win.
        stub_data = StubData {
            stub_field<MainField::nominal_netto_full_weight>(1000.0f),
            stub_field<AuxField::consumed_weight>(200.0f),
            stub_field<MainField::nominal_full_length>(330000.0f),
            stub_field<MainField::density>(1.24f),
            stub_field<MainField::filament_diameter>(1.75f),
        };
        MultiReadFieldRequest<AmountsInfo::Requirements {}> req { tool_tag };
        req.issue();
        const AmountsInfo info { req };

        CHECK(info.full_length_mm == 330000.0f);
        CHECK(info.remaining_length_mm == 330000.0f / 1000.0f * 800.0f);
    }

    SECTION("Density fallback needs both density and diameter") {
        const auto length_derivable = [](bool with_density, bool with_diameter) {
            StubData data {
                stub_field<MainField::nominal_netto_full_weight>(1000.0f),
                stub_field<AuxField::consumed_weight>(100.0f),
            };
            if (with_density) {
                data.insert(stub_field<MainField::density>(1.24f));
            }
            if (with_diameter) {
                data.insert(stub_field<MainField::filament_diameter>(1.75f));
            }
            stub_data = data;

            MultiReadFieldRequest<AmountsInfo::Requirements {}> req { tool_tag };
            req.issue();
            return AmountsInfo { req }.remaining_length_mm.has_value();
        };

        CHECK(length_derivable(false, false));
        CHECK(length_derivable(true, false)); // density only
        CHECK(length_derivable(false, true)); // diameter only
        CHECK(length_derivable(true, true)); // both
    }
}

TEST_CASE("buddy::openprinttag::data_utils::AbbreviationInfo") {
    // If the requirements change, we probably want to update the tests
    static_assert(AbbreviationInfo::Requirements::size == 2);

    SECTION("If material_abbreviation is not present, the algo should pull it out from the material_type") {
        stub_data = StubData {
            stub_field<MainField::material_type>(MaterialType::PLA),
        };

        MultiReadFieldRequest<AbbreviationInfo::Requirements {}> req { tool_tag };
        req.issue();

        AbbreviationInfo info { req };

        CHECK(info.abbreviation != "XXX");
        CHECK(info.abbreviation == "PLA");
    }

    SECTION("Check that abbreviation overrides the default MaterialType abbreviation") {
        stub_data = StubData {
            stub_field<MainField::material_type>(MaterialType::PETG),
            stub_field<MainField::material_abbreviation>("LOL"),
        };

        MultiReadFieldRequest<AbbreviationInfo::Requirements {}> req { tool_tag };
        req.issue();

        AbbreviationInfo info { req };

        CHECK(info.abbreviation == "LOL");
    }
}

TEST_CASE("buddy::openprinttag::data_utils::FilamentParametersInfo") {
    // If the requirements change, we probably want to update the tests
    static_assert(FilamentParametersInfo::Requirements::size == 14);

    SECTION("Temperature Averaging") {
        // The logic averages Min and Max to find the target print temperature
        stub_data = StubData {
            stub_field<MainField::material_type>(MaterialType::PETG),
            stub_field<MainField::min_print_temperature>(230),
            stub_field<MainField::max_print_temperature>(250),
            stub_field<MainField::min_bed_temperature>(80),
            stub_field<MainField::max_bed_temperature>(90),
        };

        MultiReadFieldRequest<FilamentParametersInfo::Requirements {}> req { tool_tag };
        req.issue();

        FilamentParametersInfo info { req };

        CHECK(info.parameters.name == "PETG");
        CHECK(info.parameters.nozzle_temperature == 240); // (230+250)/2
        CHECK(info.parameters.heatbed_temperature == 85); // (80+90)/2
        CHECK(info.parameters.base_preset == PresetFilamentType::PETG);
        CHECK_FALSE(info.parameters.is_flexible);
        CHECK_FALSE(info.parameters.requires_filtration);
        CHECK_FALSE(info.is_missing<&FilamentTypeParameters::nozzle_temperature>());
    }

    SECTION("Flexible materials logic (Shore Hardness)") {
        // Soft materals should not retract

        stub_data = StubData {
            stub_field<MainField::material_type>(MaterialType::TPU),
            stub_field<MainField::hardness_shore_a>(uint8_t { 85 }),
        };

        MultiReadFieldRequest<FilamentParametersInfo::Requirements {}> req { tool_tag };
        req.issue();

        FilamentParametersInfo info { req };

        CHECK(info.parameters.name == "TPU");
        CHECK(info.parameters.is_flexible == true);
        CHECK(info.parameters.base_preset == PresetFilamentType::FLEX);
    }

    SECTION("Tags") {
        // Abrasive tag should set is_abrasive

        // Must be defined statically, the StubData takes only a reference
        static constexpr std::array tags { MaterialTag::abrasive, MaterialTag::filtration_recommended };

        stub_data = StubData {
            stub_field<MainField::material_type>(MaterialType::PLA),
            stub_field<MainField::tags>(tags),
        };

        MultiReadFieldRequest<FilamentParametersInfo::Requirements {}> req { tool_tag };
        req.issue();

        FilamentParametersInfo info { req };

        CHECK(info.parameters.is_abrasive);
        CHECK_FALSE(info.is_missing<&FilamentTypeParameters::is_abrasive>());

        CHECK(info.parameters.requires_filtration);
        CHECK_FALSE(info.is_missing<&FilamentTypeParameters::requires_filtration>());
    }

    SECTION("Direct Parameter Mapping") {
        stub_data = StubData {
            stub_field<MainField::material_type>(MaterialType::PLA),
            stub_field<MainField::preheat_temperature>(170),
            stub_field<MainField::max_print_temperature>(215),
        };

        MultiReadFieldRequest<FilamentParametersInfo::Requirements {}> req { tool_tag };
        req.issue();

        FilamentParametersInfo info { req };

        CHECK(info.parameters.nozzle_preheat_temperature == 170);
        CHECK(info.parameters.nozzle_temperature == 215);
        CHECK(info.parameters.base_preset == PresetFilamentType::PLA);
    }

    SECTION("Color") {
        stub_data = StubData {
            stub_field<MainField::material_type>(MaterialType::PLA),
            stub_field<MainField::primary_color>(Color::from_rgb(0x12, 0x34, 0x56)),
        };

        MultiReadFieldRequest<FilamentParametersInfo::Requirements {}> req { tool_tag };
        req.issue();

        FilamentParametersInfo info { req };

        CHECK(info.parameters.color == Color::from_rgb(0x12, 0x34, 0x56));
        CHECK_FALSE(info.is_missing<&FilamentTypeParameters::color>());
    }

    SECTION("Chamber") {
        // Must be defined statically, the StubData takes only a reference
        static constexpr std::array tags { MaterialTag::filtration_recommended };

        stub_data = StubData {
            stub_field<MainField::material_type>(MaterialType::ASA),
            stub_field<MainField::chamber_temperature>(50),
            stub_field<MainField::min_chamber_temperature>(40),
            stub_field<MainField::max_chamber_temperature>(60),
            stub_field<MainField::tags>(tags),
        };

        MultiReadFieldRequest<FilamentParametersInfo::Requirements {}> req { tool_tag };
        req.issue();

        FilamentParametersInfo info { req };

        CHECK(info.parameters.name == "ASA");
        CHECK(info.parameters.chamber_target_temperature == 50);
        CHECK(info.parameters.chamber_min_temperature == 40);
        CHECK(info.parameters.chamber_max_temperature == 60);
        CHECK(info.parameters.requires_filtration);

        CHECK_FALSE(info.is_missing<&FilamentTypeParameters::requires_filtration>());
    }

    SECTION("ASA missing parameters") {
        stub_data = StubData {
            stub_field<MainField::material_type>(MaterialType::ASA),
        };

        MultiReadFieldRequest<FilamentParametersInfo::Requirements {}> req { tool_tag };
        req.issue();

        FilamentParametersInfo info { req };

        CHECK(info.parameters.name == "ASA");
        CHECK(info.parameters.requires_filtration);

        // ASA preset says that it should require filtration, but the tag did not state it
        CHECK(info.is_missing<&FilamentTypeParameters::requires_filtration>());

        CHECK(info.parameters.nozzle_temperature == FilamentType { PresetFilamentType::ASA }.parameters().nozzle_temperature);
        CHECK(info.is_missing<&FilamentTypeParameters::nozzle_temperature>());
    }

    SECTION("Everything reasonably filled") {
        stub_data = StubData {
            stub_field<MainField::material_type>(MaterialType::PLA),
            stub_field<MainField::chamber_temperature>(50),
            stub_field<MainField::min_chamber_temperature>(40),
            stub_field<MainField::max_chamber_temperature>(60),
            stub_field<MainField::max_print_temperature>(220),
            stub_field<MainField::max_bed_temperature>(50),
            stub_field<MainField::preheat_temperature>(170),
        };

        MultiReadFieldRequest<FilamentParametersInfo::Requirements {}> req { tool_tag };
        req.issue();

        FilamentParametersInfo info { req };

        CHECK(info.missing_parameters.to_ulong() == 0);

        CHECK(info.parameters.name == "PLA");
        CHECK_FALSE(info.parameters.requires_filtration);
        CHECK_FALSE(info.parameters.is_flexible);
        CHECK_FALSE(info.parameters.is_abrasive);
        CHECK(info.parameters.base_preset == PresetFilamentType::PLA);
        CHECK_FALSE(info.parameters.color.has_value());
    }
}
