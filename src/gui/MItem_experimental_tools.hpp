/// @file
#pragma once

#include <WindowMenuItems.hpp>
#include <guiconfig/guiconfig.h>

#include <option/has_extra_experimental_settings.h>

enum class ClickCommand : intptr_t {
    Return,
    Reset_Z,
    Reset_steps,
    Reset_directions,
#if HAS_EXTRA_EXPERIMENTAL_SETTINGS()
    Reset_currents,
#endif
};

#if PRINTER_IS_PRUSA_MK3_5()
// Option to switch off PWM correction to make Alte fans quiet. As of now, only MK3.5 has to deal with this issue
class MI_ALT_FAN : public WI_ICON_SWITCH_OFF_ON_t {
    static bool init_index();

public:
    MI_ALT_FAN()
        : WI_ICON_SWITCH_OFF_ON_t(init_index(), _("Alt fan correction"), nullptr, is_enabled_t::yes, is_hidden_t::no) {}

protected:
    virtual void OnChange(size_t old_index) override;
};
#endif

class MI_Z_AXIS_LEN : public WiSpin {
public:
    MI_Z_AXIS_LEN();
    void Store();
};

class MI_RESET_Z_AXIS_LEN : public IWindowMenuItem {
public:
    MI_RESET_Z_AXIS_LEN();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

#if HAS_EXTRA_EXPERIMENTAL_SETTINGS()
class MI_DIRECTION_X;
class MI_DIRECTION_Y;

class MI_STEPS_PER_UNIT_X : public WiSpin {
public:
    MI_STEPS_PER_UNIT_X();
    float value_to_store(const MI_DIRECTION_X &wrong_direction) const;
    void Store(const MI_DIRECTION_X &wrong_direction);
};

class MI_STEPS_PER_UNIT_Y : public WiSpin {
public:
    MI_STEPS_PER_UNIT_Y();
    float value_to_store(const MI_DIRECTION_Y &wrong_direction) const;
    void Store(const MI_DIRECTION_Y &wrong_direction);
};

class MI_STEPS_PER_UNIT_Z : public WiSpin {
public:
    MI_STEPS_PER_UNIT_Z();
    void Store();
};
#endif

class MI_STEPS_PER_UNIT_E : public WiSpin {
public:
    MI_STEPS_PER_UNIT_E();
    void Store();
};

class MI_RESET_STEPS_PER_UNIT : public IWindowMenuItem {
public:
    MI_RESET_STEPS_PER_UNIT();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class WiSwitchDirection : public MenuItemSwitch {
public:
    WiSwitchDirection(bool current_direction_negative, const string_view_utf8 &label_view);
};

#if HAS_EXTRA_EXPERIMENTAL_SETTINGS()
class MI_DIRECTION_X : public WiSwitchDirection {
public:
    MI_DIRECTION_X();
};

class MI_DIRECTION_Y : public WiSwitchDirection {
public:
    MI_DIRECTION_Y();
};

class MI_DIRECTION_Z : public WiSwitchDirection {
public:
    MI_DIRECTION_Z();
    void Store();
};
#endif

class MI_DIRECTION_E : public WiSwitchDirection {
public:
    MI_DIRECTION_E();
    void Store();
};

class MI_RESET_DIRECTION : public IWindowMenuItem {
public:
    MI_RESET_DIRECTION();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

#if HAS_EXTRA_EXPERIMENTAL_SETTINGS()
class MI_CURRENT_X : public WiSpin {
public:
    MI_CURRENT_X();
    void Store();
};

class MI_CURRENT_Y : public WiSpin {
public:
    MI_CURRENT_Y();
    void Store();
};

class MI_CURRENT_Z : public WiSpin {
public:
    MI_CURRENT_Z();
    void Store();
};

class MI_CURRENT_E : public WiSpin {
public:
    MI_CURRENT_E();
    void Store();
};

class MI_RESET_CURRENTS : public IWindowMenuItem {
public:
    MI_RESET_CURRENTS();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};
#endif

class MI_SAVE_AND_RETURN : public IWindowMenuItem {
public:
    MI_SAVE_AND_RETURN();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

#if HAS_ILI9488_DISPLAY()
class MI_FAST_DRAW_ENABLE final : public WI_ICON_SWITCH_OFF_ON_t {
public:
    MI_FAST_DRAW_ENABLE();

    void Store();
};
#endif
