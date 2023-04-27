/**
 * @file MItem_touch.hpp
 * @brief Touch related menu items
 */
#pragma once
#include "WindowMenuItems.hpp"

class MI_SAVE_TOUCH : public WI_LABEL_t {
    static constexpr const char *const label = N_("Save Touch Registers to Flash Disc");

public:
    MI_SAVE_TOUCH();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_LOAD_TOUCH : public WI_LABEL_t {
    static constexpr const char *const label = N_("Load Touch Registers from Flash Disc");

public:
    MI_LOAD_TOUCH();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_RESET_TOUCH : public WI_LABEL_t {
    static constexpr const char *const label = N_("Reset Touch Registers");

public:
    MI_RESET_TOUCH();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_DISP_RST : public WI_LABEL_t {
    static constexpr const char *const label = N_("Reset Display");

public:
    MI_DISP_RST();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_ENABLE_TOUCH : public WI_ICON_SWITCH_OFF_ON_t {
    constexpr static const char *const label = N_("Touch (Experimental)");

public:
    MI_ENABLE_TOUCH();

    virtual void OnChange(size_t old_index) override;
};

class MI_TOUCH_ERR_COUNT : public WiSpinInt {
    constexpr static const char *const label = "Touch error count"; // Intentionally not translated

public:
    MI_TOUCH_ERR_COUNT();
};

class MI_I2C_WORKAROUND : public WI_LABEL_t {
    constexpr static const char *const label = "I2C workaround"; // Intentionally not translated

public:
    MI_I2C_WORKAROUND();

    virtual void click(IWindowMenu &window_menu) override;
};

class MI_I2C_FORCE_RESET : public WI_LABEL_t {
    constexpr static const char *label = "I2C force reset"; // Intentionally not translated

public:
    MI_I2C_FORCE_RESET();

    virtual void click(IWindowMenu &window_menu) override;
};

class MI_I2C_RELEASE_FORCE_RESET : public WI_LABEL_t {
    constexpr static const char *label = "I2C release force reset"; // Intentionally not translated

public:
    MI_I2C_RELEASE_FORCE_RESET();

    virtual void click(IWindowMenu &window_menu) override;
};

class MI_DISPI2C_RST : public WI_LABEL_t {
    static constexpr const char *const label = "Reset I2C"; // Intentionally not translated

public:
    MI_DISPI2C_RST();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};
