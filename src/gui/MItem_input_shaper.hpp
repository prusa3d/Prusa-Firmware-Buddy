#pragma once

#include <Marlin/src/feature/input_shaper/input_shaper_config.hpp>
#include <Marlin/src/feature/input_shaper/input_shaper.hpp>
#include "WindowMenuItems.hpp"

enum class InputShaperMenuItemChildClickParam {
    request_gui_update,
    enable_editing,
};

class MI_IS_X_ONOFF : public WI_ICON_SWITCH_OFF_ON_t {
    static constexpr const char *const label = N_("X-axis");

public:
    MI_IS_X_ONOFF();

protected:
    void OnChange(size_t old_index) override;
};

class MI_IS_Y_ONOFF : public WI_ICON_SWITCH_OFF_ON_t {
    static constexpr const char *const label = N_("Y-axis");

public:
    MI_IS_Y_ONOFF();

protected:
    void OnChange(size_t old_index) override;
};

class MI_IS_X_TYPE : public WI_SWITCH_t<6> {
    static constexpr const char *const label = N_("X-axis Filter");

public:
    MI_IS_X_TYPE();

protected:
    void OnChange(size_t old_index) override;
};

class MI_IS_Y_TYPE : public WI_SWITCH_t<6> {
    static constexpr const char *const label = N_("Y-axis Filter");

public:
    MI_IS_Y_TYPE();

protected:
    void OnChange(size_t old_index) override;
};

class MI_IS_X_FREQUENCY : public WiSpinInt {
    static constexpr const char *const label = N_("X-axis Freq.");

public:
    MI_IS_X_FREQUENCY();

    virtual void OnClick() override;
};

class MI_IS_Y_FREQUENCY : public WiSpinInt {
    static constexpr const char *const label = N_("Y-axis Freq.");

public:
    MI_IS_Y_FREQUENCY();
    virtual void OnClick() override;
};

class MI_IS_Y_COMPENSATION : public WI_ICON_SWITCH_OFF_ON_t {
    static constexpr const char *const label = N_("Y Weight Compensation");

public:
    MI_IS_Y_COMPENSATION();

protected:
    void OnChange(size_t old_index) override;
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

class MI_IS_CALIB : public IWindowMenuItem {
    static constexpr const char *const label = N_("Calibration");

public:
    MI_IS_CALIB();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};
