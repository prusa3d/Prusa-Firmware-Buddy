#pragma once

#include <option/has_input_shaper_calibration.h>
#include <Marlin/src/feature/input_shaper/input_shaper_config.hpp>
#include <Marlin/src/feature/input_shaper/input_shaper.hpp>
#include "WindowMenuItems.hpp"
#include <gui/menu_item/menu_item_select_menu.hpp>
#include <meta_utils.hpp>

enum class InputShaperMenuItemChildClickParam {
    request_gui_update,
};

class MI_IS_TYPE : public MenuItemSelectMenu {

public:
    MI_IS_TYPE(AxisEnum axis);

    void update();

    int item_count() const final;
    void build_item_text(int index, const std::span<char> &buffer) const final;

protected:
    bool on_item_selected(int old_index, int new_index) override;

private:
    const AxisEnum axis_;
};

using MI_IS_X_TYPE = WithConstructorArgs<MI_IS_TYPE, X_AXIS>;
using MI_IS_Y_TYPE = WithConstructorArgs<MI_IS_TYPE, Y_AXIS>;

class MI_IS_X_FREQUENCY : public WiSpin {
    static constexpr const char *const label = N_("X-axis Freq.");

public:
    MI_IS_X_FREQUENCY();

    virtual void OnClick() override;
};

class MI_IS_Y_FREQUENCY : public WiSpin {
    static constexpr const char *const label = N_("Y-axis Freq.");

public:
    MI_IS_Y_FREQUENCY();
    virtual void OnClick() override;
};

/// Resets input shaper config to defaults on click
class MI_IS_RESTORE_DEFAULTS : public IWindowMenuItem {
    static constexpr const char *const label = N_("Restore Defaults");

public:
    MI_IS_RESTORE_DEFAULTS();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

#if HAS_INPUT_SHAPER_CALIBRATION()
class MI_IS_CALIB : public IWindowMenuItem {
    static constexpr const char *const label = N_("Calibration");

public:
    MI_IS_CALIB();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};
#endif
