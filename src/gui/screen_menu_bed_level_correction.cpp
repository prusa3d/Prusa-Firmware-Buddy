#include <screen_menu_bed_level_correction.hpp>
#include <WindowMenuSpin.hpp>
#include <ScreenHandler.hpp>

static constexpr NumericInputConfig correction_range_spin_config {
    .min_value = -100,
    .max_value = 100,
    .unit = Unit::micrometer,
};

I_MI_CORRECT::I_MI_CORRECT(CorrectionIndex index)
    : WiSpin(get_correction_value(index), correction_range_spin_config, _(correction_label(index)), nullptr, is_enabled_t::yes, is_hidden_t::no)
    , index(index) {
}

void I_MI_CORRECT::Reset() {
    SetVal(0);
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

void ScreenMenuBedLevelCorrection::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::CHILD_CLICK) {
        set_correction_value(LEFT, 0);
        set_correction_value(RIGHT, 0);
        set_correction_value(FRONT, 0);
        set_correction_value(REAR, 0);
        Item<MI_CORRECT<LEFT>>().Reset();
        Item<MI_CORRECT<RIGHT>>().Reset();
        Item<MI_CORRECT<FRONT>>().Reset();
        Item<MI_CORRECT<REAR>>().Reset();
    } else {
        ScreenMenu::windowEvent(sender, event, param);
    }
}
