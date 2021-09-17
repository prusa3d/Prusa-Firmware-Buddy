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
    Reset_directions,
    Reset_microsteps,
    Reset_currents };

class MI_Z_AXIS_LEN : public WiSpinInt {
    constexpr static const char *const label = "Z-axis length";

public:
    MI_Z_AXIS_LEN();
    void Store();
};

class MI_RESET_Z_AXIS_LEN : public WI_LABEL_t {
    static constexpr const char *const label = "Default Z-length";

public:
    MI_RESET_Z_AXIS_LEN();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_STEPS_PER_UNIT_X : public WiSpinInt {
    constexpr static const char *const label = "X-axis steps per unit";

public:
    MI_STEPS_PER_UNIT_X();
    void Store();
};

class MI_STEPS_PER_UNIT_Y : public WiSpinInt {
    constexpr static const char *const label = "Y-axis steps per unit";

public:
    MI_STEPS_PER_UNIT_Y();
    void Store();
};

class MI_STEPS_PER_UNIT_Z : public WiSpinInt {
    constexpr static const char *const label = "Z-axis steps per unit";

public:
    MI_STEPS_PER_UNIT_Z();
    void Store();
};

class MI_STEPS_PER_UNIT_E : public WiSpinInt {
    constexpr static const char *const label = "Extruder steps per unit";

public:
    MI_STEPS_PER_UNIT_E();
    void Store();
};

class MI_RESET_STEPS_PER_UNIT : public WI_LABEL_t {
    static constexpr const char *const label = "Default steps per unit";

public:
    MI_RESET_STEPS_PER_UNIT();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class WiSwitchDirection : public WI_SWITCH_t<2> {
    constexpr static const char *const str_prusa = "Prusa";
    constexpr static const char *const str_wrong = "Wrong";

public:
    WiSwitchDirection(bool current_direction_negative, string_view_utf8 label_view);
};

class MI_DIRECTION_X : public WiSwitchDirection {
    constexpr static const char *const label = "X-axis direction";

public:
    MI_DIRECTION_X();
    void Store();
};

class MI_DIRECTION_Y : public WiSwitchDirection {
    constexpr static const char *const label = "Y-axis direction";

public:
    MI_DIRECTION_Y();
    void Store();
};

class MI_DIRECTION_Z : public WiSwitchDirection {
    constexpr static const char *const label = "Z-axis direction";

public:
    MI_DIRECTION_Z();
    void Store();
};

class MI_DIRECTION_E : public WiSwitchDirection {
    constexpr static const char *const label = "Extruder direction";

public:
    MI_DIRECTION_E();
    void Store();
};

class MI_RESET_DIRECTION : public WI_LABEL_t {
    static constexpr const char *const label = "Default directions";

public:
    MI_RESET_DIRECTION();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_MICROSTEPS_X : public WiSpinExp {
    constexpr static const char *const label = "X-axis microsteps";

public:
    MI_MICROSTEPS_X();
    void Store();
};

class MI_MICROSTEPS_Y : public WiSpinExp {
    constexpr static const char *const label = "Y-axis microsteps";

public:
    MI_MICROSTEPS_Y();
    void Store();
};

class MI_MICROSTEPS_Z : public WiSpinExp {
    constexpr static const char *const label = "Z-axis microsteps";

public:
    MI_MICROSTEPS_Z();
    void Store();
};

class MI_MICROSTEPS_E : public WiSpinExp {
    constexpr static const char *const label = "Extruder microsteps";

public:
    MI_MICROSTEPS_E();
    void Store();
};

class MI_RESET_MICROSTEPS : public WI_LABEL_t {
    static constexpr const char *const label = "Reset microsteps";

public:
    MI_RESET_MICROSTEPS();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_CURRENT_X : public WiSpinInt {
    constexpr static const char *const label = "X current";

public:
    MI_CURRENT_X();
    void Store();
};

class MI_CURRENT_Y : public WiSpinInt {
    constexpr static const char *const label = "Y current";

public:
    MI_CURRENT_Y();
    void Store();
};

class MI_CURRENT_Z : public WiSpinInt {
    constexpr static const char *const label = "Z current";

public:
    MI_CURRENT_Z();
    void Store();
};

class MI_CURRENT_E : public WiSpinInt {
    constexpr static const char *const label = "Extruder current";

public:
    MI_CURRENT_E();
    void Store();
};

class MI_RESET_CURRENTS : public WI_LABEL_t {
    static constexpr const char *const label = "Reset currents";

public:
    MI_RESET_CURRENTS();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_SAVE_AND_RETURN : public WI_LABEL_t {
    static constexpr const char *const label = "Save and return";

public:
    MI_SAVE_AND_RETURN();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};
