#include "data_utils.hpp"

#include <numbers>
#include <cinttypes>
#include <algorithm>

#include <uuid/uuid_constexpr.hpp>
#include <mbedtls/sha1.h>

#include <option/has_chamber_filtration_api.h>

namespace buddy::openprinttag {

AmountsInfo::AmountsInfo(const RequestRef &req) {
    if (!req.are_results_valid()) {
        // Cannot reliably process data - failing to read a specific fields might yield unwanted results
        return;
    }

    if (auto val = req.result<MainField::actual_netto_full_weight>()) {
        full_weight_g = *val;
        full_weight_source = ValueSource::actual;

    } else if (auto val = req.result<MainField::nominal_netto_full_weight>()) {
        full_weight_g = *val;
        full_weight_source = ValueSource::nominal;
    }

    // Protect against division by zero on malicious data
    if (full_weight_g.has_value() && full_weight_g.value() <= 0) {
        full_weight_g = std::nullopt;
        full_weight_source = ValueSource::no_value;
    }

    if (auto val = req.result<MainField::actual_full_length>()) {
        full_length_mm = *val;
        full_length_source = ValueSource::actual;

    } else if (auto val = req.result<MainField::nominal_full_length>()) {
        full_length_mm = *val;
        full_length_source = ValueSource::nominal;
    }

    // Protect against division by zero on malicious data
    if (full_length_mm.has_value() && full_length_mm.value() <= 0) {
        full_length_mm = std::nullopt;
        full_length_source = ValueSource::no_value;
    }

    if (full_weight_g.has_value()) {
        const auto consumed_weight = req.result<AuxField::consumed_weight>();

        remaining_weight_g = std::max<float>(*full_weight_g - consumed_weight.value_or(0), 0);

        const float filament_diameter_mm = req.result<MainField::filament_diameter>().value_or(1.75f);
        const float density_g_cm3 = req.result<MainField::density>().value_or(1.24f);

        if (full_length_mm.has_value()) {
            remaining_length_mm = *full_length_mm / *full_weight_g * *remaining_weight_g;

        } else if (density_g_cm3 > 0.f && filament_diameter_mm > 0) {
            // fallback when full_length is not available

            // 10^-3 (g/cm3 to g/mm3) * 0.5^2 (diameter to radius mm) * pi
            constexpr float k_factor = 2.5e-4f * std::numbers::pi_v<float>;
            const float lin_density = density_g_cm3 * filament_diameter_mm * filament_diameter_mm * k_factor;

            if (lin_density > 0) {
                full_length_mm = *full_weight_g / lin_density;
                full_length_source = ValueSource::heuristic;

                remaining_length_mm = *remaining_weight_g / lin_density;
            }
        }
    }
}

void AmountsInfo::build_weight_str(StringBuilder &sb) const {
    if (full_weight_g.has_value() && remaining_weight_g.has_value()) {
        sb.append_printf("%.0f/%.0f g", (double)std::roundf(*remaining_weight_g), (double)std::roundf(*full_weight_g));
    }
    // If full_weight_g has value, then remaining_weight_g always has value, too
}

void AmountsInfo::build_length_str(StringBuilder &sb) const {
    if (full_length_mm.has_value() && remaining_length_mm.has_value()) {
        sb.append_printf("%.0f/%.0f m", (double)std::roundf(*remaining_length_mm / 1000), (double)std::roundf(*full_length_mm / 1000));

    } else if (full_length_mm.has_value()) {
        sb.append_printf("%.0f m", (double)std::roundf(*full_length_mm / 1000));
    }
}

AbbreviationInfo::AbbreviationInfo(const RequestRef &req) {
    if (!req.are_results_valid()) {
        // Cannot reliably process data - failing to read a specific fields might yield unwanted results
        return;
    }

    if (auto abbr = req.result<MainField::material_abbreviation>()) {
        abbreviation = std::string_view { abbreviation_buffer.begin(), abbr->copy(abbreviation_buffer.begin(), abbreviation_buffer.size()) };

    } else if (const auto type = req.result<MainField::material_type>()) {
        abbreviation = ::openprinttag::enum_item_name(*type);
    }
}

UUIDInfo::UUIDInfo(const RequestRef &req, const TagUID &tag_uid) {
    if (!req.are_results_valid()) {
        // Cannot reliably process data - failing to read a specific fields might yield unwanted results
        return;
    }

    static constexpr auto make_uuid = [](const UUID &namespace_, std::span<const std::byte> a, std::span<const std::byte> b = {}) {
        mbedtls_sha1_context ctx {};
        mbedtls_sha1_init(&ctx);
        mbedtls_sha1_starts_ret(&ctx);
        mbedtls_sha1_update_ret(&ctx, (const uint8_t *)namespace_.bytes.data(), namespace_.bytes.size());
        mbedtls_sha1_update_ret(&ctx, (const uint8_t *)a.data(), a.size());
        mbedtls_sha1_update_ret(&ctx, (const uint8_t *)b.data(), b.size());

        std::array<std::byte, 20> sha1;
        mbedtls_sha1_finish_ret(&ctx, (uint8_t *)sha1.data());
        mbedtls_sha1_free(&ctx);

        return UUID::make_v5_from_sha1(sha1);
    };

    static constexpr auto str_as_bytes = [](std::string_view str) {
        return std::as_bytes(std::span { str });
    };

    if (auto val = req.result<MainField::brand_uuid>()) {
        brand_uuid = OptionalUUID { *val, OptionalUUID::NulloptCheck::no };

    } else if (auto val = req.result<MainField::brand_name>()) {
        brand_uuid = make_uuid("5269dfb7-1559-440a-85be-aba5f3eff2d2"_uuid, str_as_bytes(*val));
    }

    if (auto val = req.result<MainField::material_uuid>()) {
        material_uuid = OptionalUUID { *val, OptionalUUID::NulloptCheck::no };

    } else if (auto val = req.result<MainField::material_name>(); val.has_value() && brand_uuid.has_value()) {
        material_uuid = make_uuid("616fc86d-7d99-4953-96c7-46d2836b9be9"_uuid, brand_uuid->bytes, str_as_bytes(*val));
    }

    if (auto val = req.result<MainField::package_uuid>()) {
        package_uuid = OptionalUUID { *val, OptionalUUID::NulloptCheck::no };

    } else if (auto val = req.result<MainField::gtin>(); val.has_value() && brand_uuid.has_value()) {
        ArrayStringBuilder<24> gtin_str;
        gtin_str.append_printf("%" PRIi64, *val);
        package_uuid = make_uuid("6f7d485e-db8d-4979-904e-a231cd6602b2"_uuid, brand_uuid->bytes, str_as_bytes(gtin_str.string_view()));
    }

    if (auto val = req.result<MainField::instance_uuid>()) {
        instance_uuid = OptionalUUID { *val, OptionalUUID::NulloptCheck::no };

    } else {
        // The spec hashes the UID MSB-first, TagUID is LSB-first
        decltype(tag_uid.data) uid_msb_first;
        std::ranges::reverse_copy(tag_uid.data, uid_msb_first.begin());
        instance_uuid = make_uuid("31062f81-b5bd-4f86-a5f8-46367e841508"_uuid, std::as_bytes(std::span { uid_msb_first }));
    }
}

FilamentParametersInfo::FilamentParametersInfo(const RequestRef &req, const TagUID &tag_uid) {
    using Params = FilamentTypeParameters;

    // Mark all parameters as missing, we will clear that as we get them
    missing_parameters = ~MissingParameters {};

    // We need to fill something valid otherwise we could produce invalid parameters if everything failed
    parameters.name = "XXX";

    if (!req.are_results_valid()) {
        // Cannot reliably process data - failing to read a specific fields might yield unwanted results
        return;
    }

    const auto set = [&]<auto Params::*mem_ptr>(auto value) {
        parameters.*mem_ptr = value;
        missing_parameters.set(filament_type_parameter_index<mem_ptr>, false);
    };

    const auto load_to = [&]<auto field, auto Params::*mem_ptr>() {
        if (auto val = req.result<field>()) {
            set.operator()<mem_ptr>(*val);
        }
    };

    /// @deprecated set.operator() should be used, because it clears the missing_parameters bitset when setting
    auto &parameters_unsafe = parameters;

    /// Shadow the parameters variable so that noone uses it accidentally
    /// If you want to set a parameter, use the @p set function that also clears missing_parameters
    [[maybe_unused]] const auto &parameters = parameters_unsafe;

    static_assert(
        filament_type_parameter_count
            == 11
                + HAS_CHAMBER_API() * 4
                + HAS_FILAMENT_HEATBREAK_PARAM() * 1
                + HAS_FILAMENT_BASE_PRESET_PARAM() * 1
                // requires_ht_idler_door is a preset-only hack
                + HAS_HT_HOTEND() * 1
        //
        ,
        "We probably need to implement something here");

    // Abbreviation & inherit from base type
    {
        AbbreviationInfo abbreviation { req };

        auto base_type = FilamentType::from_name(abbreviation.abbreviation);

        // Necessary compatibility workaround. OPT uses actual chemistries for abbreviations,
        // while the Buddy is stuck with the "FLEX" preset name for TPU
        if (base_type == FilamentType::none && abbreviation.abbreviation == "TPU") {
            base_type = PresetFilamentType::FLEX;
        }

        // Use preset parameters as a basis to prefill/suggest missing data
        if (base_type != FilamentType::none) {
            parameters_unsafe = base_type.parameters();

            // We have a base dataset that we can use, consider it safe
            data_safe_to_use = true;

#if HAS_FILAMENT_BASE_PRESET_PARAM()
            if (auto base_preset = std::get_if<PresetFilamentType>(&base_type)) {
                set.operator()<&Params::base_preset>(*base_preset);
            }
#endif
        }

        // Check validity of the abbreviation
        if (FilamentType { PendingAdHocFilamentType {} }.can_be_renamed_to(abbreviation.abbreviation)) {
            set.operator()<&Params::name>(abbreviation.abbreviation);

        } else {
            // Set the name, but do not clear the missing flag
            parameters_unsafe.name = abbreviation.abbreviation;

            // Also we cannot use the dataset
            data_safe_to_use = false;
        }
    }

    // Grab tag UID from any request
    set.operator()<&Params::openprinttag_uid_hash>(tag_uid.hash());

    // UUIDs
    {
        const UUIDInfo uuids { req, tag_uid };

        // Note: set also sets the parameters as not missing
        // That is fine, the user should never be asked to fill in the UUIDs
        set.operator()<&Params::material_uuid>(uuids.material_uuid);
        set.operator()<&Params::package_uuid>(uuids.package_uuid);
        set.operator()<&Params::instance_uuid>(uuids.instance_uuid);
    }

    // Color
    load_to.operator()<MainField::primary_color, &Params::color>();

    // Nozzle temp
    {
        stdext::inplace_vector<int16_t, 2> nozzle_temps;
        if (auto val = req.result<MainField::min_print_temperature>()) {
            nozzle_temps.push_back(*val);
        }
        if (auto val = req.result<MainField::max_print_temperature>()) {
            nozzle_temps.push_back(*val);
        }

        if (!nozzle_temps.empty()) {
            // Pick average of the temps
            const auto temp = std::ranges::fold_left(nozzle_temps, 0, std::plus {}) / nozzle_temps.size();
            set.operator()<&Params::nozzle_temperature>(temp);
        }
    }

    // Bed temp
    {
        stdext::inplace_vector<int16_t, 2> bed_temps;
        if (auto val = req.result<MainField::min_bed_temperature>()) {
            bed_temps.push_back(*val);
        }
        if (auto val = req.result<MainField::max_bed_temperature>()) {
            bed_temps.push_back(*val);
        }

        if (!bed_temps.empty()) {
            // Pick average from the temps
            const auto temp = std::ranges::fold_left(bed_temps, 0, std::plus {}) / bed_temps.size();
            set.operator()<&Params::heatbed_temperature>(temp);
        }
    }

    load_to.operator()<MainField::preheat_temperature, &Params::nozzle_preheat_temperature>();

#if HAS_CHAMBER_API()
    load_to.operator()<MainField::chamber_temperature, &Params::chamber_target_temperature>();
    load_to.operator()<MainField::min_chamber_temperature, &Params::chamber_min_temperature>();
    load_to.operator()<MainField::max_chamber_temperature, &Params::chamber_max_temperature>();
#endif

    // Tags
    using Tag = ::openprinttag::MaterialTag;
    const auto tags = req.result<MainField::tags>().value_or(std::span<Tag> {});
    for (Tag tag : tags) {
        switch (tag) {

        case Tag::abrasive:
            set.operator()<&Params::is_abrasive>(true);
            break;

#if HAS_CHAMBER_FILTRATION_API()
        case Tag::filtration_recommended:
            set.operator()<&Params::requires_filtration>(true);
            break;
#endif

        default:
            break;
        }
    }

    // Flexibility
    {
        bool is_flexible = false;
        if (auto val = req.result<MainField::hardness_shore_a>(); val.has_value() && *val < 95) {
            is_flexible = true;

        } else if (auto val = req.result<MainField::hardness_shore_d>(); val.has_value() && *val < 50) {
            is_flexible = true;
        }

        if (is_flexible) {
            set.operator()<&Params::is_flexible>(true);
        }
    }

    const auto unset_missing = [&]<auto Params::*mem_ptr>() {
        missing_parameters.set(filament_type_parameter_index<mem_ptr>, false);
    };

    const auto unset_missing_if_equals = [&]<auto Params::*mem_ptr>(auto default_val) {
        if (parameters_unsafe.*mem_ptr == default_val) {
            unset_missing.operator()<mem_ptr>();
        }
    };

    // If these values are "missing" but are at their default values (based on FilamentTypePreset),
    // it likely means that them not being present just means that they are on their defaults.

    // But for example in our printer presets, ASA is set as "requires filtration",
    // so if requires_filtration == true from the preset and the tag did not indicate it, the missing flag is kept
    unset_missing_if_equals.operator()<&Params::is_abrasive>(false);
    unset_missing_if_equals.operator()<&Params::is_flexible>(false);

#if HAS_CHAMBER_FILTRATION_API()
    unset_missing_if_equals.operator()<&Params::requires_filtration>(false);
#endif

#if HAS_CHAMBER_API()
    if (is_missing<&FilamentTypeParameters::chamber_min_temperature>() && is_missing<&FilamentTypeParameters::chamber_max_temperature>()) {
        // Chamber target temperature is not required if neither chamber_min_temperature or chamber_max_temperature are present
        unset_missing.operator()<&FilamentTypeParameters::chamber_target_temperature>();
    }

    // Chamber min max parameters are never required
    unset_missing.operator()<&FilamentTypeParameters::chamber_min_temperature>();
    unset_missing.operator()<&FilamentTypeParameters::chamber_max_temperature>();
#endif

    // We never miss the color
    unset_missing.operator()<&FilamentTypeParameters::color>();

    data_safe_to_use |= missing_parameters.none();
}

} // namespace buddy::openprinttag
