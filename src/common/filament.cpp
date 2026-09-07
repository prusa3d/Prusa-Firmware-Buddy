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
    if (const FilamentType r = from_name(value); r != FilamentType::none) {
        return r;
    }

    if (value == adhoc_pending_gcode_code) {
        return PendingAdHocFilamentType {};
    }

    return std::nullopt;
}

bool FilamentType::matches(const std::string_view &name) const {
    return parameters().name == name;
}

void FilamentType::build_name_with_info(std::string_view filament_name, StringBuilder &builder) const {
    builder.append_std_string_view(filament_name);

    const char *suffix =
#if HAS_MINI_DISPLAY()
        // The suffix doesn't fit on the mini display
        nullptr;
#else
        std::visit([&]<typename T>(const T &) -> const char * {
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
            *this);
#endif

    if (suffix) {
        builder.append_string(" (");
        builder.append_string_view(_(suffix));
        builder.append_char(')');
    }
}

void FilamentType::build_name_with_info(StringBuilder &builder) const {
    build_name_with_info(parameters().name, builder);
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
#if HAS_ANFC()
                                         buddy::openprinttag::ToolTag::UIDHash openprinttag_uid_hash,
#endif
                                         std::monostate) {
        return FilamentTypeParameters {
            .name = e1.name,
            .nozzle_temperature = static_cast<int16_t>(e1.nozzle_temperature),
            .nozzle_preheat_temperature = static_cast<int16_t>(e1.nozzle_preheat_temperature),
            .heatbed_temperature = e1.heatbed_temperature,
#if HAS_ANFC()
            .openprinttag_uid_hash = openprinttag_uid_hash,
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
                == 6
                    + HAS_FILAMENT_HEATBREAK_PARAM() * 1
                    + HAS_CHAMBER_API() * 3
                    + HAS_CHAMBER_FILTRATION_API() * 1
                    + HAS_FILAMENT_BASE_PRESET_PARAM() * 1
                    + HAS_ANFC() * 1
                    + HAS_HT_HOTEND() * 1
            //
            ,
            "Revise the initializer");
    };

    return std::visit([]<typename T>(const T &v) -> FilamentTypeParameters {
        if constexpr (std::is_same_v<T, PresetFilamentType>) {
            return preset_filament_parameters[v];

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
#if HAS_ANFC()
                // It should only be possible to link OPT with AdHoc filaments
                buddy::openprinttag::ToolTag::no_tag_hash,
#endif
                std::monostate());

        } else if constexpr (std::is_same_v<T, AdHocFilamentType>) {
            return build_eeprom(
                config_store().adhoc_filament_parameters.get(v.tool),
#if HAS_CHAMBER_API()
                config_store().adhoc_filament_parameters_2.get(v.tool),
#endif
#if HAS_FILAMENT_HEATBREAK_PARAM()
                config_store().adhoc_filament_parameters_3.get(v.tool),
#endif
#if HAS_FILAMENT_BASE_PRESET_PARAM()
                config_store().adhoc_filament_parameters_4.get(v.tool),
#endif
#if HAS_ANFC()
                config_store().adhoc_filament_assigned_openprinttag.get(v.tool),
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
            == 6
                + HAS_FILAMENT_HEATBREAK_PARAM() * 1
                + HAS_CHAMBER_API() * 3
                + HAS_CHAMBER_FILTRATION_API() * 1
                + HAS_FILAMENT_BASE_PRESET_PARAM() * 1
                + HAS_ANFC() * 1
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
#if HAS_ANFC()
            // It should only be possible to link OPT with AdHoc filaments
            debug_assert(set.openprinttag_uid_hash == buddy::openprinttag::ToolTag::no_tag_hash);
#endif

        } else if constexpr (std::is_same_v<T, AdHocFilamentType>) {
            config_store().adhoc_filament_parameters.set(v.tool, e1);
#if HAS_CHAMBER_API()
            config_store().adhoc_filament_parameters_2.set(v.tool, e2);
#endif
#if HAS_FILAMENT_HEATBREAK_PARAM()
            config_store().adhoc_filament_parameters_3.set(v.tool, e3);
#endif
#if HAS_FILAMENT_BASE_PRESET_PARAM()
            config_store().adhoc_filament_parameters_4.set(v.tool, e4);
#endif
#if HAS_ANFC()
            config_store().adhoc_filament_assigned_openprinttag.set(v.tool, set.openprinttag_uid_hash);
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
