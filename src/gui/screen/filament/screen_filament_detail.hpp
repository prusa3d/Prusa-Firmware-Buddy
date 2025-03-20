#pragma once

#include <type_traits>
#include <filament.hpp>

#include <WindowMenuInfo.hpp>
#include <WindowMenuSpin.hpp>
#include <MItem_tools.hpp>
#include <screen_menu.hpp>
#include <numeric_input_config.hpp>

#include <option/has_chamber_api.h>

namespace screen_filament_detail {

template <typename T>
class MI_SPIN : public WiSpin {

public:
    using Parameter = T FilamentTypeParameters::*;

    MI_SPIN(Parameter param, const NumericInputConfig &config, const char *label)
        : WiSpin(0, config, _(label))
        , param_(param) {
    }

    void set_filament_type(FilamentType set) {
        filament_type = set;
        set_value(filament_type.parameters().*param_);
        set_enabled(filament_type.is_customizable());
    }

    void OnClick() override {
        if constexpr (std::is_arithmetic_v<T>) {
            filament_type.modify_parameters([&](auto &p) { p.*param_ = this->value(); });
        } else {
            // If T is std::optional, we use utilize the optional support of the spin as well
            filament_type.modify_parameters([&](auto &p) { p.*param_ = this->value_opt(); });
        }
    }

private:
    Parameter param_;
    FilamentType filament_type;
};

class MI_TOGGLE : public WI_ICON_SWITCH_OFF_ON_t {

public:
    using Parameter = bool FilamentTypeParameters::*;

    MI_TOGGLE(Parameter param, const char *label);

    void set_filament_type(FilamentType set);
    void OnChange(size_t) override;

private:
    Parameter param_;
    FilamentType filament_type;
};

class MI_FILAMENT_NAME final : public WI_INFO_t {
public:
    MI_FILAMENT_NAME();
    void set_filament_type(FilamentType set);
    void click(IWindowMenu &) override;

protected:
    FilamentType filament_type;
};

class MI_FILAMENT_NOZZLE_TEMPERATURE final : public MI_SPIN<decltype(FilamentTypeParameters::nozzle_temperature)> {
public:
    MI_FILAMENT_NOZZLE_TEMPERATURE();
};

class MI_FILAMENT_NOZZLE_PREHEAT_TEMPERATURE final : public MI_SPIN<decltype(FilamentTypeParameters::nozzle_preheat_temperature)> {
public:
    MI_FILAMENT_NOZZLE_PREHEAT_TEMPERATURE();
};

class MI_FILAMENT_BED_TEMPERATURE final : public MI_SPIN<decltype(FilamentTypeParameters::heatbed_temperature)> {
public:
    MI_FILAMENT_BED_TEMPERATURE();
};

#if HAS_CHAMBER_API()
class MI_FILAMENT_MIN_CHAMBER_TEMPERATURE final : public MI_SPIN<decltype(FilamentTypeParameters::chamber_min_temperature)> {
public:
    MI_FILAMENT_MIN_CHAMBER_TEMPERATURE();
};
#endif

#if HAS_CHAMBER_API()
class MI_FILAMENT_MAX_CHAMBER_TEMPERATURE final : public MI_SPIN<decltype(FilamentTypeParameters::chamber_max_temperature)> {
public:
    MI_FILAMENT_MAX_CHAMBER_TEMPERATURE();
};
#endif

#if HAS_CHAMBER_API()
class MI_FILAMENT_TARGET_CHAMBER_TEMPERATURE final : public MI_SPIN<decltype(FilamentTypeParameters::chamber_target_temperature)> {
public:
    MI_FILAMENT_TARGET_CHAMBER_TEMPERATURE();
};
#endif

#if HAS_CHAMBER_API()
class MI_FILAMENT_REQUIRES_FILTRATION final : public MI_TOGGLE {
public:
    MI_FILAMENT_REQUIRES_FILTRATION();
};
#endif

class MI_FILAMENT_IS_ABRASIVE final : public MI_TOGGLE {
public:
    MI_FILAMENT_IS_ABRASIVE();
};

class MI_FILAMENT_IS_FLEXIBLE final : public MI_TOGGLE {
public:
    MI_FILAMENT_IS_FLEXIBLE();
};

class MI_FILAMENT_VISIBLE final : public WI_ICON_SWITCH_OFF_ON_t {
public:
    MI_FILAMENT_VISIBLE();
    void set_filament_type(FilamentType set);
    void OnChange(size_t) final;

protected:
    FilamentType filament_type;
};

class MI_PREHEAT_CONFIRM final : public IWindowMenuItem {
public:
    MI_PREHEAT_CONFIRM();
    void set_filament_type(FilamentType set);
    void click(IWindowMenu &) override;

protected:
    FilamentType filament_type;
};

using ScreenFilamentDetail_ = ScreenMenu<EFooter::Off,
    MI_RETURN,
    MI_FILAMENT_NAME,
    MI_FILAMENT_VISIBLE,
    MI_FILAMENT_NOZZLE_TEMPERATURE,
    MI_FILAMENT_NOZZLE_PREHEAT_TEMPERATURE,
    MI_FILAMENT_BED_TEMPERATURE,
#if HAS_CHAMBER_API()
    MI_FILAMENT_TARGET_CHAMBER_TEMPERATURE,
    MI_FILAMENT_MIN_CHAMBER_TEMPERATURE,
    MI_FILAMENT_MAX_CHAMBER_TEMPERATURE,
#endif
    MI_FILAMENT_IS_ABRASIVE,
    MI_FILAMENT_IS_FLEXIBLE,
#if HAS_CHAMBER_API()
    MI_FILAMENT_REQUIRES_FILTRATION,
#endif
    MI_PREHEAT_CONFIRM //
    >;

/// Management of a specified filament type
class ScreenFilamentDetail final : public ScreenFilamentDetail_ {
public:
    enum class Mode : uint8_t {
        /// Standard filament detail screen, as accessed from menu
        standard,

        /// When the detail screen is opened from within the preheat menu.
        /// Adds a "Confirm" button that sends the filament as a response to the preheat FSM
        preheat,
    };

    struct Params {
        FilamentType filament_type;
        Mode mode = Mode::standard;
    };

public:
    ScreenFilamentDetail(Params params);

    ScreenFilamentDetail(FilamentType filament_type)
        : ScreenFilamentDetail(Params { .filament_type = filament_type }) {}
};

}; // namespace screen_filament_detail

using ScreenFilamentDetail = screen_filament_detail::ScreenFilamentDetail;
