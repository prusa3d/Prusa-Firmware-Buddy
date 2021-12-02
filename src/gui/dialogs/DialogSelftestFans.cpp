#include "DialogSelftestFans.hpp"
#include "i18n.h"
#include "wizard_config.hpp"
#include "selftest_fans_type.hpp"

static constexpr size_t col_0 = WizardDefaults::MarginLeft;
static constexpr size_t col_1 = WizardDefaults::status_icon_X_pos;
static constexpr size_t col_0_w = col_1 - col_0;

static constexpr size_t txt_h = WizardDefaults::txt_h;
static constexpr size_t row_h = WizardDefaults::row_h;

static constexpr size_t row_0 = WizardDefaults::row_0;
static constexpr size_t row_1 = row_0 + row_h;
static constexpr size_t row_2 = row_1 + WizardDefaults::progress_row_h;
static constexpr size_t row_3 = row_2 + row_h;

static constexpr const char *en_text_fan_test = N_("Fan Test");
static constexpr const char *en_text_hotend_fan = N_("Hotend Fan");
static constexpr const char *en_text_print_fan = N_("Print Fan");

DialogSelftestFans::DialogSelftestFans()
    : IDialogMarlin()
    , text_fan_test(this, Rect16(col_0, row_0, WizardDefaults::X_space, txt_h), is_multiline::no, is_closed_on_click_t::no, _(en_text_fan_test))
    , progress(this, row_1)
    , text_hotend_fan(this, Rect16(col_0, row_2, col_0_w, txt_h), is_multiline::no, is_closed_on_click_t::no, _(en_text_hotend_fan))
    , icon_hotend_fan(this, { col_1, row_2 })
    , text_print_fan(this, Rect16(col_0, row_3, col_0_w, txt_h), is_multiline::no, is_closed_on_click_t::no, _(en_text_print_fan))
    , icon_print_fan(this, { col_1, row_3 }) {
}

bool DialogSelftestFans::change(uint8_t phase, fsm::PhaseData data) {
    SelftestFans_t dt(data);

    icon_print_fan.SetState(dt.print_fan_state);
    icon_hotend_fan.SetState(dt.heatbreak_fan_state);
    progress.SetProgressPercent(dt.tot_progress);
    return true;
};
