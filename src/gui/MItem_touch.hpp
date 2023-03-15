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
