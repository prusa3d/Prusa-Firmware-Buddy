#include <catch2/catch_test_macros.hpp>

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

Request::SerializeResult TagRequest::serialize(ManagerNoLockBadge badge, RequestID request_id, anfc::modbus::Request &request) {
    return std::nullopt;
}

void ReadEnumArrayRequestBase::serialize(RequestID, TagID, anfc::modbus::Request &) {}
void ReadEnumFieldRequestBase::serialize(RequestID, TagID, anfc::modbus::Request &) {}
void ReadFloatFieldRequest::serialize(RequestID, TagID, anfc::modbus::Request &) {}
void ReadInt32FieldRequest::serialize(RequestID, TagID, anfc::modbus::Request &) {}
void ReadStringRequestBase::serialize(RequestID, TagID, anfc::modbus::Request &) {}

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
    static_assert(AmountsInfo::Requirements::size == 5);

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
    static_assert(FilamentParametersInfo::Requirements::size == 13);

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
            stub_field<MainField::shore_hardness_a>(uint8_t { 85 }),
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
    }
}
