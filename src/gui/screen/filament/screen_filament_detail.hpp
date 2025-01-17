#pragma once

#include <type_traits>

#include <filament.hpp>

#include <WindowMenuInfo.hpp>
#include <WindowMenuSpin.hpp>
#include <MItem_tools.hpp>
#include <screen_menu.hpp>

#include <option/has_chamber_api.h>

namespace screen_filament_detail {

template <typename Child, typename Parent>
class MI_COMMON : public Parent {

public:
    using Parent::Parent;

    void set_filament_type(FilamentType set) {
        filament_type = set;
        static_cast<Child *>(this)->update();
    }

protected:
    FilamentType filament_type;
};

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

class MI_FILAMENT_NAME final : public MI_COMMON<MI_FILAMENT_NAME, WI_INFO_t> {
public:
    MI_FILAMENT_NAME();
    void update();
    void click(IWindowMenu &) override;
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
class MI_FILAMENT_REQUIRES_FILTRATION final : public MI_TOGGLE {
public:
    MI_FILAMENT_REQUIRES_FILTRATION();
};
#endif

class MI_FILAMENT_IS_ABRASIVE final : public MI_TOGGLE {
public:
    MI_FILAMENT_IS_ABRASIVE();
};

class MI_FILAMENT_VISIBLE final : public MI_COMMON<MI_FILAMENT_VISIBLE, WI_ICON_SWITCH_OFF_ON_t> {
public:
    MI_FILAMENT_VISIBLE();
    void update();
    void OnChange(size_t) final;
};

class MI_PREHEAT_CONFIRM final : public MI_COMMON<MI_PREHEAT_CONFIRM, IWindowMenuItem> {
public:
    MI_PREHEAT_CONFIRM();
    void update();
    void click(IWindowMenu &) override;
};

using ScreenFilamentDetail_ = ScreenMenu<EFooter::Off,
    MI_RETURN,
    MI_FILAMENT_NAME,
    MI_FILAMENT_VISIBLE,
    MI_FILAMENT_NOZZLE_TEMPERATURE,
    MI_FILAMENT_NOZZLE_PREHEAT_TEMPERATURE,
    MI_FILAMENT_BED_TEMPERATURE,
    MI_FILAMENT_IS_ABRASIVE,
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
