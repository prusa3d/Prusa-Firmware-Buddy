/**
 * @file screen_menu_experimental_settings.cpp
 * @author Radek Vana
 * @brief experimental settings
 * @date 2021-07-28
 */

#include "gui.hpp"
#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_menus.hpp"
#include "screen_menus.hpp"
#include "eeprom.h"
#include "menu_spin_config.hpp"
#include "ScreenHandler.hpp"
#include "window_msgbox.hpp"
#include "sys.h"
#include "string.h" // memcmp
#include "WindowMenuSpinExponential.hpp"

enum class ClickCommand : intptr_t { Return,
    Reset_Z,
    Reset_steps,
    Reset_microsteps,
    Reset_currents };

/*****************************************************************************/
//MI_Z_AXIS_LEN
class MI_Z_AXIS_LEN : public WiSpinInt {
    constexpr static const char *const label = N_("Z axis length");

public:
    MI_Z_AXIS_LEN()
        : WiSpinInt(get_z_max_pos_mm_rounded(), SpinCnf::axis_z_max_range, _(label)) {}
};

/*****************************************************************************/
//MI_RESET_Z_AXIS_LEN
class MI_RESET_Z_AXIS_LEN : public WI_LABEL_t {
    static constexpr const char *const label = N_("Reset Z axis length to default");

public:
    MI_RESET_Z_AXIS_LEN()
        : WI_LABEL_t(_(label)) {}

protected:
    virtual void click(IWindowMenu &window_menu) override {
        Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, (void *)ClickCommand::Reset_Z);
    }
};

/*****************************************************************************/
//MI_STEPS_PER_UNIT_X
class MI_STEPS_PER_UNIT_X : public WiSpinInt {
    constexpr static const char *const label = N_("X axis steps per unit");

public:
    MI_STEPS_PER_UNIT_X()
        : WiSpinInt(get_steps_per_unit_x_rounded(), SpinCnf::steps_per_unit, _(label)) {}
};
/*****************************************************************************/
//MI_STEPS_PER_UNIT_Y
class MI_STEPS_PER_UNIT_Y : public WiSpinInt {
    constexpr static const char *const label = N_("Y axis steps per unit");

public:
    MI_STEPS_PER_UNIT_Y()
        : WiSpinInt(get_steps_per_unit_y_rounded(), SpinCnf::steps_per_unit, _(label)) {}
};
/*****************************************************************************/
//MI_STEPS_PER_UNIT_Z
class MI_STEPS_PER_UNIT_Z : public WiSpinInt {
    constexpr static const char *const label = N_("Z axis steps per unit");

public:
    MI_STEPS_PER_UNIT_Z()
        : WiSpinInt(get_steps_per_unit_z_rounded(), SpinCnf::steps_per_unit, _(label)) {}
};
/*****************************************************************************/
//MI_STEPS_PER_UNIT_E
class MI_STEPS_PER_UNIT_E : public WiSpinInt {
    constexpr static const char *const label = N_("Extruder steps per unit");

public:
    MI_STEPS_PER_UNIT_E()
        : WiSpinInt(get_steps_per_unit_e_rounded(), SpinCnf::steps_per_unit, _(label)) {}
};
/*****************************************************************************/
//MI_RESET_STEPS_PER_UNIT
class MI_RESET_STEPS_PER_UNIT : public WI_LABEL_t {
    static constexpr const char *const label = N_("Reset steps per unit to default");

public:
    MI_RESET_STEPS_PER_UNIT()
        : WI_LABEL_t(_(label)) {}

protected:
    virtual void click(IWindowMenu &window_menu) override {
        Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, (void *)ClickCommand::Reset_steps);
    }
};

/*****************************************************************************/
//MI_MICROSTEPS_X
class MI_MICROSTEPS_X : public WiSpinExp {
    constexpr static const char *const label = N_("X axis microsteps");

public:
    MI_MICROSTEPS_X()
        : WiSpinExp(get_microsteps_x(), SpinCnf::microstep_exponential, _(label)) {}
};
/*****************************************************************************/
//MI_MICROSTEPS_Y
class MI_MICROSTEPS_Y : public WiSpinExp {
    constexpr static const char *const label = N_("Y axis microsteps");

public:
    MI_MICROSTEPS_Y()
        : WiSpinExp(get_microsteps_y(), SpinCnf::microstep_exponential, _(label)) {}
};
/*****************************************************************************/
//MI_MICROSTEPS_Z
class MI_MICROSTEPS_Z : public WiSpinExp {
    constexpr static const char *const label = N_("Z axis microsteps");

public:
    MI_MICROSTEPS_Z()
        : WiSpinExp(get_microsteps_z(), SpinCnf::microstep_exponential, _(label)) {}
};
/*****************************************************************************/
//MI_MICROSTEPS_E
class MI_MICROSTEPS_E : public WiSpinExp {
    constexpr static const char *const label = N_("Extruder microsteps");

public:
    MI_MICROSTEPS_E()
        : WiSpinExp(get_microsteps_e(), SpinCnf::microstep_exponential, _(label)) {}
};
/*****************************************************************************/
//MI_RESET_MICROSTEPS
class MI_RESET_MICROSTEPS : public WI_LABEL_t {
    static constexpr const char *const label = N_("Reset microsteps to default");

public:
    MI_RESET_MICROSTEPS()
        : WI_LABEL_t(_(label)) {}

protected:
    virtual void click(IWindowMenu &window_menu) override {
        Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, (void *)ClickCommand::Reset_microsteps);
    }
};

/*****************************************************************************/
//MI_CURRENT_X
class MI_CURRENT_X : public WiSpinInt {
    constexpr static const char *const label = N_("X current");

public:
    MI_CURRENT_X()
        : WiSpinInt(get_rms_current_ma_x(), SpinCnf::rms_current, _(label)) {}
};
/*****************************************************************************/
//MI_CURRENT_Y
class MI_CURRENT_Y : public WiSpinInt {
    constexpr static const char *const label = N_("Y current");

public:
    MI_CURRENT_Y()
        : WiSpinInt(get_rms_current_ma_y(), SpinCnf::rms_current, _(label)) {}
};
/*****************************************************************************/
//MI_CURRENT_Z
class MI_CURRENT_Z : public WiSpinInt {
    constexpr static const char *const label = N_("Z current");

public:
    MI_CURRENT_Z()
        : WiSpinInt(get_rms_current_ma_z(), SpinCnf::rms_current, _(label)) {}
};
/*****************************************************************************/
//MI_CURRENT_E
class MI_CURRENT_E : public WiSpinInt {
    constexpr static const char *const label = N_("Extruder current");

public:
    MI_CURRENT_E()
        : WiSpinInt(get_rms_current_ma_e(), SpinCnf::rms_current, _(label)) {}
};
/*****************************************************************************/
//MI_RESET_CURRENTS
class MI_RESET_CURRENTS : public WI_LABEL_t {
    static constexpr const char *const label = N_("Reset currents to default");

public:
    MI_RESET_CURRENTS()
        : WI_LABEL_t(_(label)) {}

protected:
    virtual void click(IWindowMenu &window_menu) override {
        Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, (void *)ClickCommand::Reset_currents);
    }
};

/*****************************************************************************/
//MI_SAVE_AND_RETURN
class MI_SAVE_AND_RETURN : public WI_LABEL_t {
    static constexpr const char *const label = N_("Save and return");

public:
    MI_SAVE_AND_RETURN()
        : WI_LABEL_t(_(label), IDR_PNG_folder_up_16px, is_enabled_t::yes, is_hidden_t::no) {}

protected:
    virtual void click(IWindowMenu &window_menu) override {
        Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, (void *)ClickCommand::Return);
    }
};

/*****************************************************************************/
//Screen
using Screen = ScreenMenu<EHeader::Off, EFooter::On, HelpLines_None, MI_SAVE_AND_RETURN, MI_Z_AXIS_LEN, MI_RESET_Z_AXIS_LEN, MI_STEPS_PER_UNIT_X, MI_STEPS_PER_UNIT_Y, MI_STEPS_PER_UNIT_Z, MI_STEPS_PER_UNIT_E, MI_RESET_STEPS_PER_UNIT, MI_MICROSTEPS_X, MI_MICROSTEPS_Y, MI_MICROSTEPS_Z, MI_MICROSTEPS_E, MI_RESET_MICROSTEPS, MI_CURRENT_X, MI_CURRENT_Y, MI_CURRENT_Z, MI_CURRENT_E, MI_RESET_CURRENTS>;

class ScreenMenuExperimentalSettings : public Screen {
    static constexpr const char *const save_and_reboot = N_("Do you want to save changes and reboot the printer?");
    constexpr static const char *label = N_("Experimental Settings");
    struct values_t {
        values_t(ScreenMenuExperimentalSettings &parent)
            : z_len(parent.Item<MI_Z_AXIS_LEN>().GetVal())
            , steps_per_unit_x(parent.Item<MI_STEPS_PER_UNIT_X>().GetVal())
            , steps_per_unit_y(parent.Item<MI_STEPS_PER_UNIT_Y>().GetVal())
            , steps_per_unit_z(parent.Item<MI_STEPS_PER_UNIT_Z>().GetVal())
            , steps_per_unit_e(parent.Item<MI_STEPS_PER_UNIT_E>().GetVal())
            , microsteps_x(parent.Item<MI_MICROSTEPS_X>().GetVal())
            , microsteps_y(parent.Item<MI_MICROSTEPS_Y>().GetVal())
            , microsteps_z(parent.Item<MI_MICROSTEPS_Z>().GetVal())
            , microsteps_e(parent.Item<MI_MICROSTEPS_E>().GetVal())
            , rms_current_ma_x(parent.Item<MI_CURRENT_X>().GetVal())
            , rms_current_ma_y(parent.Item<MI_CURRENT_Y>().GetVal())
            , rms_current_ma_z(parent.Item<MI_CURRENT_Z>().GetVal())
            , rms_current_ma_e(parent.Item<MI_CURRENT_E>().GetVal()) {}

        int32_t z_len;
        int32_t steps_per_unit_x;
        int32_t steps_per_unit_y;
        int32_t steps_per_unit_z;
        int32_t steps_per_unit_e;
        int32_t microsteps_x;
        int32_t microsteps_y;
        int32_t microsteps_z;
        int32_t microsteps_e;
        int32_t rms_current_ma_x;
        int32_t rms_current_ma_y;
        int32_t rms_current_ma_z;
        int32_t rms_current_ma_e;

        // this is only safe as long as there are no gaps between variabes
        // all variables re 32bit now, so it is safe
        constexpr bool operator==(const values_t &other) const {
            return memcmp(this, &other, sizeof(values_t)) == 0;
        }
        constexpr bool operator!=(const values_t &other) const {
            return !(*this == other);
        }
    } initial;

    void clicked_return() {
        values_t current(*this); //ctor will handle load of values
        //unchanged
        if (current == initial) {
            Screens::Access()->Close();
            return;
        }

        switch (MsgBoxQuestion(_(save_and_reboot), Responses_YesNoCancel)) {
        case Response::Yes:
            set_z_max_pos_mm(Item<MI_Z_AXIS_LEN>().GetVal());
            sys_reset();
        case Response::No:
            Screens::Access()->Close();
            return;
        default:
            return; //do nothing
        }
    }

public:
    ScreenMenuExperimentalSettings()
        : Screen(_(label))
        , initial(*this) {}

    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t ev, void *param) override {
        if (ev != GUI_event_t::CHILD_CLICK) {
            SuperWindowEvent(sender, ev, param);
            return;
        }

        switch (ClickCommand(intptr_t(param))) {
        case ClickCommand::Return:
            clicked_return();
            break;
        case ClickCommand::Reset_Z:
            Item<MI_Z_AXIS_LEN>().SetVal(default_Z_max_pos);
            menu.Invalidate(); // its broken, does not work
            break;
        case ClickCommand::Reset_steps:
            Item<MI_STEPS_PER_UNIT_X>().SetVal(MenuVars::default_steps_per_unit[0]);
            Item<MI_STEPS_PER_UNIT_Y>().SetVal(MenuVars::default_steps_per_unit[1]);
            Item<MI_STEPS_PER_UNIT_Z>().SetVal(MenuVars::default_steps_per_unit[2]);
            Item<MI_STEPS_PER_UNIT_E>().SetVal(MenuVars::default_steps_per_unit[3]);
            menu.Invalidate(); // its broken, does not work
            break;
        case ClickCommand::Reset_microsteps:
            Item<MI_MICROSTEPS_X>().SetVal(MenuVars::default_microsteps[0]);
            Item<MI_MICROSTEPS_Y>().SetVal(MenuVars::default_microsteps[4]);
            Item<MI_MICROSTEPS_Z>().SetVal(MenuVars::default_microsteps[2]);
            Item<MI_MICROSTEPS_E>().SetVal(MenuVars::default_microsteps[3]);
            menu.Invalidate(); // its broken, does not work
            break;
        case ClickCommand::Reset_currents:
            Item<MI_CURRENT_X>().SetVal(MenuVars::default_currents[0]);
            Item<MI_CURRENT_Y>().SetVal(MenuVars::default_currents[1]);
            Item<MI_CURRENT_Z>().SetVal(MenuVars::default_currents[2]);
            Item<MI_CURRENT_E>().SetVal(MenuVars::default_currents[3]);
            menu.Invalidate(); // its broken, does not work
            break;
        }
    }
};

ScreenFactory::UniquePtr GetScreenMenuExperimentalSettings() {
    return ScreenFactory::Screen<ScreenMenuExperimentalSettings>();
}
