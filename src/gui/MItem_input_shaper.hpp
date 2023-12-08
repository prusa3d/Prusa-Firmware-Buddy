#pragma once

#include <Marlin/src/feature/input_shaper/input_shaper_config.hpp>
#include <Marlin/src/feature/input_shaper/input_shaper.hpp>
#include "WindowMenuItems.hpp"

enum class input_shaper_param {
    set_values,
    change_x,
    change_y
};

class MI_IS_X_ONOFF : public WI_ICON_SWITCH_OFF_ON_t {
    static constexpr const char *const label = N_("X-axis");
    input_shaper_param param = input_shaper_param::change_x;

public:
    MI_IS_X_ONOFF();

protected:
    void OnChange(size_t old_index) override;
};

class MI_IS_Y_ONOFF : public WI_ICON_SWITCH_OFF_ON_t {
    static constexpr const char *const label = N_("Y-axis");
    input_shaper_param param = input_shaper_param::change_y;

public:
    MI_IS_Y_ONOFF();

protected:
    void OnChange(size_t old_index) override;
};

class MI_IS_X_TYPE : public WI_SWITCH_t<6> {
    static constexpr const char *const label = N_("X-axis filter");

public:
    MI_IS_X_TYPE();

protected:
    void OnChange(size_t old_index) override;
};

class MI_IS_Y_TYPE : public WI_SWITCH_t<6> {
    static constexpr const char *const label = N_("Y-axis filter");

public:
    MI_IS_Y_TYPE();

protected:
    void OnChange(size_t old_index) override;
};

class MI_IS_X_FREQUENCY : public WiSpinInt {
    static constexpr const char *const label = N_("X-axis freq.");

public:
    MI_IS_X_FREQUENCY();

    virtual void OnClick() override;
};

class MI_IS_Y_FREQUENCY : public WiSpinInt {
    static constexpr const char *const label = N_("Y-axis freq.");

public:
    MI_IS_Y_FREQUENCY();
    virtual void OnClick() override;
};

class MI_IS_Y_COMPENSATION : public WI_ICON_SWITCH_OFF_ON_t {
    static constexpr const char *const label = N_("Y weight compensation");

public:
    MI_IS_Y_COMPENSATION();

protected:
    void OnChange(size_t old_index) override;
};

class MI_IS_SET : public IWindowMenuItem {
    static constexpr const char *const label = N_("Set up values");
    input_shaper_param param = input_shaper_param::set_values;

public:
    MI_IS_SET();

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
