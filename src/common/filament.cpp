#include "filament.hpp"
#include "filament_list.hpp"
#include "filament_eeprom.hpp"

#include <cstring>
#include <algorithm>

#include "i18n.h"

#include <utils/enum_array.hpp>
#include <utils/string_builder.hpp>
#include <guiconfig/guiconfig.h>
#include <config_store/store_instance.hpp>
#include <common/aggregate_arity.hpp>
#include <utils/mutex_atomic.hpp>
#include <freertos/mutex.hpp>
#include <inc/MarlinConfig.h>
#include <bsod/bsod.h>
#include <str_utils.hpp>

#if HAS_ANFC()
    #include <feature/openprinttag/tool_tag.hpp>
#endif

// !!! If these value change, you need to inspect usages and possibly write up some config store migrations
static_assert(filament_name_buffer_size == 8);
static_assert(max_preset_filament_type_count == 32);
static_assert(max_user_filament_type_count == 32);
static_assert(max_total_filament_count == 64);
static_assert(sizeof(FilamentTypeParameters_EEPROM1) == 14);

#if HAS_CHAMBER_API()
static_assert(sizeof(FilamentTypeParameters_EEPROM2) == 3);
#endif

#if HAS_FILAMENT_HEATBREAK_PARAM()
static_assert(sizeof(FilamentTypeParameters_EEPROM3) == 1);
#endif

#if HAS_ANFC()
static_assert(std::is_same_v<decltype(FilamentTypeParameters::openprinttag_uid_hash), buddy::openprinttag::ToolTag::UIDHash>);
static_assert(FilamentTypeParameters().openprinttag_uid_hash == buddy::openprinttag::ToolTag::no_tag_hash);
#endif

static_assert(preset_filament_type_count <= max_preset_filament_type_count);
static_assert(user_filament_type_count <= max_user_filament_type_count);
static_assert(VirtualToolIndex::count <= adhoc_filament_type_count);

// We're storing the bed temperature in uint8_t, so make sure the bed cannot go higher
static_assert(BED_MAXTEMP <= 255);

static constexpr FilamentTypeParameters none_filament_parameters {
    .name = "---",
    .nozzle_temperature = 0,
    .nozzle_preheat_temperature = 0,
    .heatbed_temperature = 0,
};

MutexAtomic<FilamentTypeParameters, freertos::Mutex> pending_adhoc_filament_parameters_ {
    FilamentTypeParameters {
        .name = "CUSTOM",
    },
};

std::optional<FilamentType> FilamentType::from_gcode_param(const std::string_view &value) {
    if (value.length() == 0) {
        return std::nullopt;
    }

    if (value.length() > 1 && value[0] == '#') {
        uint8_t encoded;
        const auto conv_r = from_chars_light(value.begin() + 1, value.end(), encoded);
        if (conv_r.ec != std::errc {} || conv_r.ptr != value.end()) {
            return std::nullopt;
        }

        const auto result = EncodedFilamentType::from_data(encoded).decode();
        if (result == FilamentType::none) {
            return std::nullopt;
        }

        return result;
    }

    if (value == adhoc_pending_gcode_code) {
        return PendingAdHocFilamentType {};
    }

    if (const FilamentType r = from_name(value); r != FilamentType::none) {
        return r;
    }

    return std::nullopt;
}

bool FilamentType::matches(const std::string_view &name) const {
    return parameters().name == name;
}

BuildFilamentNameWithInfoParams::BuildFilamentNameWithInfoParams(FilamentType type, const FilamentTypeParameters &params)
    : name { params.name }
    , type { type }
#if HAS_ANFC()
    , opt_assigned { params.openprinttag_uid_hash != buddy::openprinttag::ToolTag::no_tag_hash }
#endif
{
}

void FilamentType::build_encoded_gcode_param(StringBuilder &builder) const {
    builder.append_printf("%c%i", encoded_gcode_code_prefix, EncodedFilamentType(*this).data);
}

void FilamentType::build_name_with_info(const BuildFilamentNameWithInfoParams &params, StringBuilder &builder) {
    builder.append_std_string_view(params.name);

    const char *suffix =
#if HAS_MINI_DISPLAY()
        // The suffix doesn't fit on the mini display
        nullptr;
#else
        [&] -> const char * {
    #if HAS_ANFC()
        if (params.opt_assigned) {
            // Indicates that the filament data has been loaded from the OpenPrintTag
            return N_("Tag");
        }
    #endif

        return std::visit([&]<typename T>(const T &) -> const char * {
            if constexpr (std::is_same_v<T, NoFilamentType>) {
                return nullptr;

            } else if constexpr (std::is_same_v<T, PresetFilamentType>) {
                // Used in filament type context, for example: "PLA (Preset)". Please try to fit the text in 6 characters.
                return N_("Preset");

            } else if constexpr (std::is_same_v<T, UserFilamentType>) {
                // Used in filament type context (user-defined preset), for example: "PCCF (User)". Please try to fit the text in 6 characters.
                return N_("User");

            } else if constexpr (std::is_same_v<T, AdHocFilamentType> || std::is_same_v<T, PendingAdHocFilamentType>) {
                // Used in filament type context (user-defined ad-hoc), for example: "XX (User)". Please try to fit the text in 6 characters.
                return N_("Custom");

            } else {
                static_assert(false);
            }
        },
            params.type);
    }();
#endif

    if (suffix) {
        builder.append_string(" (");
        builder.append_string_view(_(suffix));
        builder.append_char(')');
    }
}

void FilamentType::build_name_with_info(StringBuilder &builder) const {
    build_name_with_info(BuildFilamentNameWithInfoParams(*this, parameters()), builder);
}

FilamentTypeParameters FilamentType::parameters() const {
    static const auto build_eeprom = [](const FilamentTypeParameters_EEPROM1 &e1,
#if HAS_CHAMBER_API()
                                         const FilamentTypeParameters_EEPROM2 &e2,
#endif
#if HAS_FILAMENT_HEATBREAK_PARAM()
                                         const FilamentTypeParameters_EEPROM3 &e3,
#endif
#if HAS_FILAMENT_BASE_PRESET_PARAM()
                                         const FilamentTypeParameters_EEPROM4 &e4,
#endif
                                         const FilamentTypeParameters_EEPROM5 &e5,
#if HAS_ANFC()
                                         buddy::openprinttag::ToolTag::UIDHash openprinttag_uid_hash,
                                         const FilamentTypeParameters_EEPROM6 &e6,
#endif
                                         std::monostate) {
        return FilamentTypeParameters {
            .name = e1.name,
            .color = e5.color(),
            .nozzle_temperature = static_cast<int16_t>(e1.nozzle_temperature),
            .nozzle_preheat_temperature = static_cast<int16_t>(e1.nozzle_preheat_temperature),
            .heatbed_temperature = e1.heatbed_temperature,
#if HAS_ANFC()
            .openprinttag_uid_hash = openprinttag_uid_hash,
            .material_uuid = e6.decode_uuid(e6.material_uuid),
            .package_uuid = e6.decode_uuid(e6.package_uuid),
            .instance_uuid = e6.decode_uuid(e6.instance_uuid),
#endif
#if HAS_FILAMENT_BASE_PRESET_PARAM()
            .base_preset = e4.decode_base_preset(),
#endif
#if HAS_FILAMENT_HEATBREAK_PARAM()
            .heatbreak_temperature = e3.heatbreak_temperature,
#endif
#if HAS_CHAMBER_API()
            .chamber_min_temperature = e2.decode_chamber_temp(e2.chamber_min_temperature),
            .chamber_max_temperature = e2.decode_chamber_temp(e2.chamber_max_temperature),
            .chamber_target_temperature = e2.decode_chamber_temp(e2.chamber_target_temperature),
#endif
#if HAS_CHAMBER_FILTRATION_API()
            .requires_filtration = e1.requires_filtration,
#endif
            .is_abrasive = e1.is_abrasive,
            .is_flexible = e1.is_flexible,
        };
        static_assert(
            aggregate_arity<FilamentTypeParameters>()
                == 7
                    + HAS_FILAMENT_HEATBREAK_PARAM() * 1
                    + HAS_CHAMBER_API() * 3
                    + HAS_CHAMBER_FILTRATION_API() * 1
                    + HAS_FILAMENT_BASE_PRESET_PARAM() * 1
                    + HAS_ANFC() * 4
                    + HAS_HT_HOTEND() * 1
            //
            ,
            "Revise the initializer");
    };

    return std::visit([]<typename T>(const T &v) -> FilamentTypeParameters {
        if constexpr (std::is_same_v<T, PresetFilamentType>) {
            return preset_filament_parameters(v);

        } else if constexpr (std::is_same_v<T, UserFilamentType>) {
            return build_eeprom(
                config_store().user_filament_parameters.get(v.index),
#if HAS_CHAMBER_API()
                config_store().user_filament_parameters_2.get(v.index),
#endif
#if HAS_FILAMENT_HEATBREAK_PARAM()
                config_store().user_filament_parameters_3.get(v.index),
#endif
#if HAS_FILAMENT_BASE_PRESET_PARAM()
                config_store().user_filament_parameters_4.get(v.index),
#endif
                // EEPROM5 params (color) is adhoc-only
                FilamentTypeParameters_EEPROM5 {},
#if HAS_ANFC()
                // It should only be possible to link OPT with AdHoc filaments
                buddy::openprinttag::ToolTag::no_tag_hash,

                // EEPROM6 (UUIDs) is adhoc-only
                FilamentTypeParameters_EEPROM6 {},
#endif
                std::monostate());

        } else if constexpr (std::is_same_v<T, AdHocFilamentType>) {
            const auto i = v.tool.to_raw();

#if HAS_ANFC()
            static_assert(decltype(config_store().adhoc_filament_parameters_6)::item_count == 1);
#endif

            return build_eeprom(
                config_store().adhoc_filament_parameters.get(i),
#if HAS_CHAMBER_API()
                config_store().adhoc_filament_parameters_2.get(i),
#endif
#if HAS_FILAMENT_HEATBREAK_PARAM()
                config_store().adhoc_filament_parameters_3.get(i),
#endif
#if HAS_FILAMENT_BASE_PRESET_PARAM()
                config_store().adhoc_filament_parameters_4.get(i),
#endif
                config_store().adhoc_filament_parameters_5.get(i),
#if HAS_ANFC()
                config_store().adhoc_filament_assigned_openprinttag.get(i),

                // Silently don't get for i > 0 - only first slot UUIDs are stored in EEPROM
                i == 0 ? config_store().adhoc_filament_parameters_6.get(i) : FilamentTypeParameters_EEPROM6 {},
#endif
                std::monostate());

        } else if constexpr (std::is_same_v<T, PendingAdHocFilamentType>) {
            return pending_adhoc_filament_parameters_.load();

        } else if constexpr (std::is_same_v<T, NoFilamentType>) {
            return none_filament_parameters;
        }
    },
        *this);
}

void FilamentType::set_parameters(const FilamentTypeParameters &set) const {
    debug_assert(can_be_renamed_to(set.name));
    static_assert(
        aggregate_arity<FilamentTypeParameters>()
            == 7
                + HAS_FILAMENT_HEATBREAK_PARAM() * 1
                + HAS_CHAMBER_API() * 3
                + HAS_CHAMBER_FILTRATION_API() * 1
                + HAS_FILAMENT_BASE_PRESET_PARAM() * 1
                + HAS_ANFC() * 4
                + HAS_HT_HOTEND() * 1
        //
        ,
        "Revise FilamentType::set_parameters");

    const FilamentTypeParameters_EEPROM1 e1 {
        .name = set.name,
        .nozzle_temperature = static_cast<uint16_t>(set.nozzle_temperature),
        .nozzle_preheat_temperature = static_cast<uint16_t>(set.nozzle_preheat_temperature),
        .heatbed_temperature = static_cast<uint8_t>(set.heatbed_temperature),
#if HAS_CHAMBER_FILTRATION_API()
        .requires_filtration = set.requires_filtration,
#endif
        .is_abrasive = set.is_abrasive,
        .is_flexible = set.is_flexible,
    };
    // Note - even though we're not setting requires_filtration without HAS_CHAMBER_FILTRATION_API, it is still in the EEPROM struct to provide binary compatibility
    static_assert(aggregate_arity<FilamentTypeParameters_EEPROM1>() == 7 + 1 /* _unused */, "Revise the initializer");
    static_assert(requires { FilamentTypeParameters_EEPROM1::_unused; });

#if HAS_CHAMBER_API()
    const FilamentTypeParameters_EEPROM2 e2 {
        .chamber_min_temperature = e2.encode_chamber_temp(set.chamber_min_temperature),
        .chamber_max_temperature = e2.encode_chamber_temp(set.chamber_max_temperature),
        .chamber_target_temperature = e2.encode_chamber_temp(set.chamber_target_temperature),
    };
    static_assert(aggregate_arity<FilamentTypeParameters_EEPROM2>() == 3, "Revise the initializer");
#endif

#if HAS_FILAMENT_HEATBREAK_PARAM()
    const FilamentTypeParameters_EEPROM3 e3 {
        .heatbreak_temperature = set.heatbreak_temperature,
    };
    static_assert(aggregate_arity<FilamentTypeParameters_EEPROM3>() == 1, "Revise the initializer");
#endif

#if HAS_FILAMENT_BASE_PRESET_PARAM()
    const FilamentTypeParameters_EEPROM4 e4 {
        .base_preset = FilamentTypeParameters_EEPROM4::encode_base_preset(set.base_preset),
    };
    static_assert(aggregate_arity<FilamentTypeParameters_EEPROM4>() == 1, "Revise the initializer");
#endif

    const FilamentTypeParameters_EEPROM5 e5 {
        .encoded_color = FilamentTypeParameters_EEPROM5::encode_color(set.color),
    };
    static_assert(aggregate_arity<FilamentTypeParameters_EEPROM5>() == 1, "Revise the initializer");

#if HAS_ANFC()
    const FilamentTypeParameters_EEPROM6 e6 {
        .material_uuid = FilamentTypeParameters_EEPROM6::encode_uuid(set.material_uuid),
        .package_uuid = FilamentTypeParameters_EEPROM6::encode_uuid(set.package_uuid),
        .instance_uuid = FilamentTypeParameters_EEPROM6::encode_uuid(set.instance_uuid),
    };
#endif

    std::visit([&]<typename T>(const T &v) {
        if constexpr (std::is_same_v<T, PresetFilamentType>) {
            debug_assert(false);

        } else if constexpr (std::is_same_v<T, UserFilamentType>) {
            config_store().user_filament_parameters.set(v.index, e1);
#if HAS_CHAMBER_API()
            config_store().user_filament_parameters_2.set(v.index, e2);
#endif
#if HAS_FILAMENT_HEATBREAK_PARAM()
            config_store().user_filament_parameters_3.set(v.index, e3);
#endif
#if HAS_FILAMENT_BASE_PRESET_PARAM()
            config_store().user_filament_parameters_4.set(v.index, e4);
#endif
            // EEPROM5 (color) is adhoc-only
            debug_assert(set.color == std::nullopt);
#if HAS_ANFC()
            // It should only be possible to link OPT with AdHoc filaments
            debug_assert(set.openprinttag_uid_hash == buddy::openprinttag::ToolTag::no_tag_hash);
            debug_assert(set.instance_uuid == std::nullopt);
            debug_assert(set.package_uuid == std::nullopt);
            debug_assert(set.material_uuid == std::nullopt);
#endif

        } else if constexpr (std::is_same_v<T, AdHocFilamentType>) {
            const auto i = v.tool.to_raw();

            config_store().adhoc_filament_parameters.set(i, e1);
#if HAS_CHAMBER_API()
            config_store().adhoc_filament_parameters_2.set(i, e2);
#endif
#if HAS_FILAMENT_HEATBREAK_PARAM()
            config_store().adhoc_filament_parameters_3.set(i, e3);
#endif
#if HAS_FILAMENT_BASE_PRESET_PARAM()
            config_store().adhoc_filament_parameters_4.set(i, e4);
#endif
            config_store().adhoc_filament_parameters_5.set(i, e5);
#if HAS_ANFC()
            config_store().adhoc_filament_assigned_openprinttag.set(i, set.openprinttag_uid_hash);

            if (i == 0) {
                static_assert(decltype(config_store().adhoc_filament_parameters_6)::item_count == 1);
                config_store().adhoc_filament_parameters_6.set(i, e6);
            } else {
                // Silently don't store, we don't have space for it
                debug_assert(false);
            }
#endif

        } else if constexpr (std::is_same_v<T, PendingAdHocFilamentType>) {
            pending_adhoc_filament_parameters_.store(set);

        } else if constexpr (std::is_same_v<T, NoFilamentType>) {
            debug_assert(false);

        } else {
            static_assert(false);
        }
    },
        *this);
}

bool FilamentType::is_visible() const {
    return std::visit([]<typename T>(const T &v) -> bool {
        if constexpr (std::is_same_v<T, PresetFilamentType>) {
            return config_store().visible_preset_filament_types.get().test(static_cast<size_t>(v));

        } else if constexpr (std::is_same_v<T, UserFilamentType>) {
            return config_store().visible_user_filament_types.get().test(v.index);

        } else if constexpr (std::is_same_v<T, AdHocFilamentType>) {
            return false;

        } else if constexpr (std::is_same_v<T, PendingAdHocFilamentType>) {
            return false;

        } else if constexpr (std::is_same_v<T, NoFilamentType>) {
            return false;
        }
    },
        *this);
}

void FilamentType::set_visible(bool set) const {
    return std::visit([set]<typename T>(const T &v) -> void {
        if constexpr (std::is_same_v<T, PresetFilamentType>) {
            config_store().visible_preset_filament_types.apply([v, set](auto &value) {
                value.set(static_cast<size_t>(v), set);
            });

        } else if constexpr (std::is_same_v<T, UserFilamentType>) {
            config_store().visible_user_filament_types.apply([v, set](auto &value) {
                value.set(v.index, set);
            });

        } else if constexpr (std::is_same_v<T, AdHocFilamentType>) {
            // Should never happen
            debug_assert(0);

        } else if constexpr (std::is_same_v<T, PendingAdHocFilamentType>) {
            // Should never happen
            debug_assert(0);

        } else if constexpr (std::is_same_v<T, NoFilamentType>) {
            // Do nothing

        } else {
            static_assert(0);
        }
    },
        *this);
}
