#pragma once

#include <option/has_input_shaper_calibration.h>
#include <Marlin/src/feature/input_shaper/input_shaper_config.hpp>
#include <Marlin/src/feature/input_shaper/input_shaper.hpp>
#include "WindowMenuItems.hpp"
#include <window_menu_enum_switch.hpp>

enum class InputShaperMenuItemChildClickParam {
    request_gui_update,
    enable_editing,
};

class MI_IS_X_TYPE : public WiEnumSwitch {
    static constexpr const char *const label = N_("X-axis Filter");

public:
    MI_IS_X_TYPE();

protected:
    void OnChange(size_t old_index) override;
};

class MI_IS_Y_TYPE : public WiEnumSwitch {
    static constexpr const char *const label = N_("Y-axis Filter");

public:
    MI_IS_Y_TYPE();

protected:
    void OnChange(size_t old_index) override;
};

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

class MI_IS_ENABLE_EDITING : public IWindowMenuItem {
    static constexpr const char *const label = N_("Enable Editing");

public:
    MI_IS_ENABLE_EDITING();

protected:
    virtual void click(IWindowMenu &window_menu) override;
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
