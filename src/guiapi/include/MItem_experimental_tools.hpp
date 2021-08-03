/**
 * @file MItem_experimental_tools.hpp
 * @author Radek Vana
 * @brief tools used in experimental menus
 * @date 2021-08-03
 */

#pragma once
#include "WindowMenuItems.hpp"
#include "WindowMenuSpinExponential.hpp"
#include "i18n.h"

enum class ClickCommand : intptr_t { Return,
    Reset_Z,
    Reset_steps,
    Reset_microsteps,
    Reset_currents };

class MI_Z_AXIS_LEN : public WiSpinInt {
    constexpr static const char *const label = N_("Z axis length");

public:
    MI_Z_AXIS_LEN();
};

class MI_RESET_Z_AXIS_LEN : public WI_LABEL_t {
    static constexpr const char *const label = N_("Reset Z axis length to default");

public:
    MI_RESET_Z_AXIS_LEN();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_STEPS_PER_UNIT_X : public WiSpinInt {
    constexpr static const char *const label = N_("X axis steps per unit");

public:
    MI_STEPS_PER_UNIT_X();
};

class MI_STEPS_PER_UNIT_Y : public WiSpinInt {
    constexpr static const char *const label = N_("Y axis steps per unit");

public:
    MI_STEPS_PER_UNIT_Y();
};

class MI_STEPS_PER_UNIT_Z : public WiSpinInt {
    constexpr static const char *const label = N_("Z axis steps per unit");

public:
    MI_STEPS_PER_UNIT_Z();
};

class MI_STEPS_PER_UNIT_E : public WiSpinInt {
    constexpr static const char *const label = N_("Extruder steps per unit");

public:
    MI_STEPS_PER_UNIT_E();
};

class MI_RESET_STEPS_PER_UNIT : public WI_LABEL_t {
    static constexpr const char *const label = N_("Reset steps per unit to default");

public:
    MI_RESET_STEPS_PER_UNIT();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_MICROSTEPS_X : public WiSpinExp {
    constexpr static const char *const label = N_("X axis microsteps");

public:
    MI_MICROSTEPS_X();
};

class MI_MICROSTEPS_Y : public WiSpinExp {
    constexpr static const char *const label = N_("Y axis microsteps");

public:
    MI_MICROSTEPS_Y();
};

class MI_MICROSTEPS_Z : public WiSpinExp {
    constexpr static const char *const label = N_("Z axis microsteps");

public:
    MI_MICROSTEPS_Z();
};

class MI_MICROSTEPS_E : public WiSpinExp {
    constexpr static const char *const label = N_("Extruder microsteps");

public:
    MI_MICROSTEPS_E();
};

class MI_RESET_MICROSTEPS : public WI_LABEL_t {
    static constexpr const char *const label = N_("Reset microsteps to default");

public:
    MI_RESET_MICROSTEPS();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_CURRENT_X : public WiSpinInt {
    constexpr static const char *const label = N_("X current");

public:
    MI_CURRENT_X();
};

class MI_CURRENT_Y : public WiSpinInt {
    constexpr static const char *const label = N_("Y current");

public:
    MI_CURRENT_Y();
};

class MI_CURRENT_Z : public WiSpinInt {
    constexpr static const char *const label = N_("Z current");

public:
    MI_CURRENT_Z();
};

class MI_CURRENT_E : public WiSpinInt {
    constexpr static const char *const label = N_("Extruder current");

public:
    MI_CURRENT_E();
};

class MI_RESET_CURRENTS : public WI_LABEL_t {
    static constexpr const char *const label = N_("Reset currents to default");

public:
    MI_RESET_CURRENTS();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_SAVE_AND_RETURN : public WI_LABEL_t {
    static constexpr const char *const label = N_("Save and return");

public:
    MI_SAVE_AND_RETURN();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};
