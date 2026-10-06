#pragma once

#include <filament.hpp>

#include <common/aggregate_arity.hpp>
#include <WindowMenuInfo.hpp>
#include <WindowMenuSpin.hpp>
#include <MItem_tools.hpp>
#include <screen_menu.hpp>
#include <numeric_input_config.hpp>
#include <tool_index.hpp>
#include <gui/menu_item/menu_item_color.hpp>
#include <window_menu_callback_item.hpp>

#include <option/has_chamber_api.h>
#include <option/has_filament_heatbreak_param.h>
#include <option/has_filament_base_preset_param.h>

#include <option/has_anfc.h>
#if HAS_ANFC()
    #include <feature/openprinttag/tool_tag.hpp>
    #include <gui/menu_item/menu_item_uuid.hpp>
#endif

namespace screen_filament_detail {

class MI_FILAMENT_NAME final : public WiInfo<32> {
public:
    static constexpr auto parameter_ptr = &FilamentTypeParameters::name;

    MI_FILAMENT_NAME();

    void set_params(const BuildFilamentNameWithInfoParams &set);

    const auto &value() const {
        return data_.name;
    }

    void set_value(const FilamentTypeParameters::Name &set);

    void click(IWindowMenu &) override;

    void update_text();

protected:
    BuildFilamentNameWithInfoParams data_;
};

class MI_FILAMENT_COLOR final : public MenuItemColor {
public:
    static constexpr auto parameter_ptr = &FilamentTypeParameters::color;
    static constexpr bool is_adhoc_only = true;

    MI_FILAMENT_COLOR();
};

#if HAS_FILAMENT_BASE_PRESET_PARAM()
class MI_FILAMENT_BASE_PRESET final : public MenuItemSelectMenu {
public:
    using T = FilamentTypeParameters::BasePreset;
    static constexpr auto parameter_ptr = &FilamentTypeParameters::base_preset;

    MI_FILAMENT_BASE_PRESET();

    T value() const;
    void set_value(T set);

    int item_count() const final;
    string_view_utf8 build_item_text(int index, StringViewUtf8ParamBase &params) const final;
};
#endif

class MI_FILAMENT_NOZZLE_TEMPERATURE final : private NumericInputConfigHolder, public WiSpin {
public:
    static constexpr auto parameter_ptr = &FilamentTypeParameters::nozzle_temperature;

    MI_FILAMENT_NOZZLE_TEMPERATURE();
};

class MI_FILAMENT_NOZZLE_PREHEAT_TEMPERATURE final : private NumericInputConfigHolder, public WiSpin {
public:
    static constexpr auto parameter_ptr = &FilamentTypeParameters::nozzle_preheat_temperature;

    MI_FILAMENT_NOZZLE_PREHEAT_TEMPERATURE();
};

class MI_FILAMENT_BED_TEMPERATURE final : public WiSpin {
public:
    static constexpr auto parameter_ptr = &FilamentTypeParameters::heatbed_temperature;

    MI_FILAMENT_BED_TEMPERATURE();
};

#if HAS_FILAMENT_HEATBREAK_PARAM()
class MI_FILAMENT_HEATBREAK_TEMPERATURE final : public WiSpin {
public:
    static constexpr auto parameter_ptr = &FilamentTypeParameters::heatbreak_temperature;

    MI_FILAMENT_HEATBREAK_TEMPERATURE();
};
#endif

#if HAS_CHAMBER_API()
class MI_FILAMENT_MIN_CHAMBER_TEMPERATURE final : public WiSpin {
public:
    static constexpr auto parameter_ptr = &FilamentTypeParameters::chamber_min_temperature;
    static constexpr bool is_chamber_item = true;

    MI_FILAMENT_MIN_CHAMBER_TEMPERATURE();
};
#endif

#if HAS_CHAMBER_API()
class MI_FILAMENT_MAX_CHAMBER_TEMPERATURE final : public WiSpin {
public:
    static constexpr auto parameter_ptr = &FilamentTypeParameters::chamber_max_temperature;
    static constexpr bool is_chamber_item = true;

    MI_FILAMENT_MAX_CHAMBER_TEMPERATURE();
};
#endif

#if HAS_CHAMBER_API()
class MI_FILAMENT_TARGET_CHAMBER_TEMPERATURE final : public WiSpin {
public:
    static constexpr auto parameter_ptr = &FilamentTypeParameters::chamber_target_temperature;
    static constexpr bool is_chamber_item = true;

    MI_FILAMENT_TARGET_CHAMBER_TEMPERATURE();
};
#endif

#if HAS_CHAMBER_FILTRATION_API()
class MI_FILAMENT_REQUIRES_FILTRATION final : public WI_ICON_SWITCH_OFF_ON_t {
public:
    static constexpr auto parameter_ptr = &FilamentTypeParameters::requires_filtration;

    MI_FILAMENT_REQUIRES_FILTRATION();
};
#endif

#if HAS_ANFC()
class MI_FILAMENT_ASSIGNED_OPENPRINTTAG final : public WI_ICON_SWITCH_OFF_ON_t {
public:
    static constexpr auto parameter_ptr = &FilamentTypeParameters::openprinttag_uid_hash;
    static constexpr bool is_adhoc_only = true;

    MI_FILAMENT_ASSIGNED_OPENPRINTTAG();

    using Value = buddy::openprinttag::ToolTag::UIDHash;
    static constexpr Value no_tag_hash = buddy::openprinttag::ToolTag::no_tag_hash;

    Value value() const {
        return WI_ICON_SWITCH_OFF_ON_t::value() ? original_uid_hash_ : no_tag_hash;
    }

    void set_value(Value set);

protected:
    void OnChange(size_t) override;

private:
    Value original_uid_hash_ = no_tag_hash;
};

class MI_FILAMENT_MATERIAL_UUID final : public MenuItemUUID {
public:
    static constexpr auto parameter_ptr = &FilamentTypeParameters::material_uuid;
    static constexpr bool is_adhoc_only = true;

    MI_FILAMENT_MATERIAL_UUID()
        : MenuItemUUID(_("Material UUID")) {}
};

class MI_FILAMENT_PACKAGE_UUID final : public MenuItemUUID {
public:
    static constexpr auto parameter_ptr = &FilamentTypeParameters::package_uuid;
    static constexpr bool is_adhoc_only = true;

    MI_FILAMENT_PACKAGE_UUID()
        : MenuItemUUID(_("Package UUID")) {}
};

class MI_FILAMENT_INSTANCE_UUID final : public MenuItemUUID {
public:
    static constexpr auto parameter_ptr = &FilamentTypeParameters::instance_uuid;
    static constexpr bool is_adhoc_only = true;

    MI_FILAMENT_INSTANCE_UUID()
        : MenuItemUUID(_("Instance UUID")) {}
};
#endif

class MI_FILAMENT_IS_ABRASIVE final : public WI_ICON_SWITCH_OFF_ON_t {
public:
    static constexpr auto parameter_ptr = &FilamentTypeParameters::is_abrasive;

    MI_FILAMENT_IS_ABRASIVE();
};

class MI_FILAMENT_IS_FLEXIBLE final : public WI_ICON_SWITCH_OFF_ON_t {
public:
    static constexpr auto parameter_ptr = &FilamentTypeParameters::is_flexible;

    MI_FILAMENT_IS_FLEXIBLE();
};

class MI_FILAMENT_VISIBLE final : public WI_ICON_SWITCH_OFF_ON_t {
public:
    MI_FILAMENT_VISIBLE();
};

/// Utility button, hidden by default, used for various purposes
using MI_CALLBACK = WithConstructorArgs<WindowMenuCallbackItem, nullptr, nullptr>;

using ScreenFilamentDetail_ = ScreenMenu<EFooter::Off,
    MI_RETURN,
    MI_FILAMENT_NAME,
    MI_FILAMENT_VISIBLE,
    MI_FILAMENT_COLOR,
#if HAS_FILAMENT_BASE_PRESET_PARAM()
    MI_FILAMENT_BASE_PRESET,
#endif
    MI_FILAMENT_NOZZLE_TEMPERATURE,
    MI_FILAMENT_NOZZLE_PREHEAT_TEMPERATURE,
    MI_FILAMENT_BED_TEMPERATURE,
#if HAS_FILAMENT_HEATBREAK_PARAM()
    MI_FILAMENT_HEATBREAK_TEMPERATURE,
#endif
#if HAS_CHAMBER_API()
    MI_FILAMENT_TARGET_CHAMBER_TEMPERATURE,
    MI_FILAMENT_MIN_CHAMBER_TEMPERATURE,
    MI_FILAMENT_MAX_CHAMBER_TEMPERATURE,
#endif
    MI_FILAMENT_IS_ABRASIVE,
    MI_FILAMENT_IS_FLEXIBLE,
#if HAS_CHAMBER_FILTRATION_API()
    MI_FILAMENT_REQUIRES_FILTRATION,
#endif
#if HAS_ANFC()
    MI_FILAMENT_ASSIGNED_OPENPRINTTAG,
    MI_FILAMENT_MATERIAL_UUID,
    MI_FILAMENT_PACKAGE_UUID,
    MI_FILAMENT_INSTANCE_UUID,
#endif
    MI_CALLBACK //
    >;

static_assert(
    aggregate_arity<FilamentTypeParameters>()
        == 7
            + HAS_FILAMENT_HEATBREAK_PARAM() * 1
            + HAS_CHAMBER_API() * 4
            + HAS_FILAMENT_BASE_PRESET_PARAM() * 1
            + HAS_ANFC() * 4
            // requires_ht_idler_door - preset-only hack
            + HAS_HT_HOTEND() * 1
    //
    ,
    "Revise ScreenFilamentDetail");

template <typename T>
inline constexpr bool is_utility_button = std::is_same_v<T, MI_RETURN> || std::is_same_v<T, MI_CALLBACK>;

/// Management of a specified filament type
class ScreenFilamentDetail : public ScreenFilamentDetail_ {
public:
    /// When the detail screen is opened from within the preheat menu.
    /// Adds a "Confirm" button that sends the filament as a response to the preheat FSM
    struct PreheatModeParams {
        using ToolIndex = std::variant<VirtualToolIndex, AllTools>;

        ToolIndex tool = AllTools {};
        PreheatMode mode;
    };

    struct LoadedFilamentMode {
        VirtualToolIndex tool;
    };

public:
    ScreenFilamentDetail(FilamentType filament_type);

    /// Shows the screen in the PendingAdHocFilament mode for preheat
    /// The added "Confirm" button sends the response to the Preheat FSM
    ScreenFilamentDetail(PreheatModeParams preheat_mode);

    /// Shows details for the filament loaded to the specified tool
    ScreenFilamentDetail(LoadedFilamentMode loaded_filament_mode);

    ~ScreenFilamentDetail();

public:
    struct SetupArgs {
        bool force_disable_customizable : 1 = false;
    };

    /// Fills the screen with the provided data
    void setup(FilamentType filament_type, const FilamentTypeParameters &params, SetupArgs args);
    void setup(FilamentType filament_type, SetupArgs args);

    void save_changes();

protected:
    ScreenFilamentDetail(const char *title);

    void setup_preheat_mode_confirm(PreheatModeParams params);

protected:
    FilamentType filament_type_;
    StringViewUtf8Parameters<4> title_params_;
};

}; // namespace screen_filament_detail

using ScreenFilamentDetail = screen_filament_detail::ScreenFilamentDetail;
