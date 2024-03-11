#include <screen_menu_bed_level_correction.hpp>
#include <menu_spin_config.hpp>
#include <ScreenHandler.hpp>

I_MI_CORRECT::I_MI_CORRECT(CorrectionIndex index)
    : WiSpinInt(get_correction_value(index), SpinCnf::correction_range, _(correction_label(index)), nullptr, is_enabled_t::yes, is_hidden_t::no)
    , index(index) {
}

void I_MI_CORRECT::OnClick() {
    set_correction_value(index, GetVal());
}

MI_RESET::MI_RESET()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}

void MI_RESET::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->WindowEvent(GUI_event_t::CHILD_CLICK, (void *)this);
}

ScreenMenuBedLevelCorrection::ScreenMenuBedLevelCorrection()
    : ScreenMenu(_(label)) {}

void ScreenMenuBedLevelCorrection::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::CHILD_CLICK) {
        set_correction_value(LEFT, 0);
        set_correction_value(RIGHT, 0);
        set_correction_value(FRONT, 0);
        set_correction_value(REAR, 0);
        Item<MI_CORRECT<LEFT>>().SetVal(0);
        Item<MI_CORRECT<RIGHT>>().SetVal(0);
        Item<MI_CORRECT<FRONT>>().SetVal(0);
        Item<MI_CORRECT<REAR>>().SetVal(0);
    } else {
        SuperWindowEvent(sender, event, param);
    }
}
