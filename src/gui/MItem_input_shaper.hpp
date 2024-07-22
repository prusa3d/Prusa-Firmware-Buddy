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

#if HAS_INPUT_SHAPER_CALIBRATION()
class MI_IS_CALIB : public IWindowMenuItem {
    static constexpr const char *const label = N_("Calibration");

public:
    MI_IS_CALIB();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_IS_CALIB_RESULT : public IWindowMenuItem {
private:
    const char *gcode;

protected:
    MI_IS_CALIB_RESULT(const char *label, const char *gcode);

    virtual void click(IWindowMenu &) final;
};

class MI_IS_CALIB_RESULT_UNKNOWN final : public MI_IS_CALIB_RESULT {
public:
    MI_IS_CALIB_RESULT_UNKNOWN()
        : MI_IS_CALIB_RESULT("Set Result Unknown", "M1959 W0") {}
};

class MI_IS_CALIB_RESULT_SKIPPED final : public MI_IS_CALIB_RESULT {
public:
    MI_IS_CALIB_RESULT_SKIPPED()
        : MI_IS_CALIB_RESULT("Set Result Skipped", "M1959 W1") {}
};

class MI_IS_CALIB_RESULT_PASSED final : public MI_IS_CALIB_RESULT {
public:
    MI_IS_CALIB_RESULT_PASSED()
        : MI_IS_CALIB_RESULT("Set Result Passed", "M1959 W2") {}
};

class MI_IS_CALIB_RESULT_FAILED final : public MI_IS_CALIB_RESULT {
public:
    MI_IS_CALIB_RESULT_FAILED()
        : MI_IS_CALIB_RESULT("Set Result Failed", "M1959 W3") {}
};

#endif
