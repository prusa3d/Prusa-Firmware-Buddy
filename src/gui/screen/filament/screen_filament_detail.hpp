#pragma once

#include <filament.hpp>

#include <WindowMenuInfo.hpp>
#include <WindowMenuSpin.hpp>
#include <MItem_tools.hpp>
#include <screen_menu.hpp>

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

class MI_FILAMENT_NAME final : public MI_COMMON<MI_FILAMENT_NAME, WI_INFO_t> {
public:
    MI_FILAMENT_NAME();
    void update();
    void click(IWindowMenu &) override;
};

class MI_FILAMENT_NOZZLE_TEMPERATURE final : public MI_COMMON<MI_FILAMENT_NOZZLE_TEMPERATURE, WiSpin> {
public:
    MI_FILAMENT_NOZZLE_TEMPERATURE();
    void update();
    void OnClick() override;
};

class MI_FILAMENT_NOZZLE_PREHEAT_TEMPERATURE final : public MI_COMMON<MI_FILAMENT_NOZZLE_PREHEAT_TEMPERATURE, WiSpin> {
public:
    MI_FILAMENT_NOZZLE_PREHEAT_TEMPERATURE();
    void update();
    void OnClick() override;
};

class MI_FILAMENT_BED_TEMPERATURE final : public MI_COMMON<MI_FILAMENT_BED_TEMPERATURE, WiSpin> {
public:
    MI_FILAMENT_BED_TEMPERATURE();
    void update();
    void OnClick() override;
};

class MI_FILAMENT_REQUIRES_FILTRATION final : public MI_COMMON<MI_FILAMENT_REQUIRES_FILTRATION, WI_ICON_SWITCH_OFF_ON_t> {
public:
    MI_FILAMENT_REQUIRES_FILTRATION();
    void update();
    void OnChange(size_t) override;
};

class MI_FILAMENT_IS_ABRASIVE final : public MI_COMMON<MI_FILAMENT_IS_ABRASIVE, WI_ICON_SWITCH_OFF_ON_t> {
public:
    MI_FILAMENT_IS_ABRASIVE();
    void update();
    void OnChange(size_t) override;
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
    MI_FILAMENT_REQUIRES_FILTRATION,
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
        EncodedFilamentType filament_type;
        Mode mode = Mode::standard;
    };

public:
    ScreenFilamentDetail(Params params);

    ScreenFilamentDetail(FilamentType filament_type)
        : ScreenFilamentDetail(Params { .filament_type = filament_type }) {}
};

}; // namespace screen_filament_detail

using ScreenFilamentDetail = screen_filament_detail::ScreenFilamentDetail;
