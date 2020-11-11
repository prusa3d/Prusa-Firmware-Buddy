#include "DialogSelftestFans.hpp"
#include "i18n.h"
#include "wizard_config.hpp"

static constexpr size_t col_0 = WIZARD_MARGIN_LEFT;
static constexpr size_t col_0_w = 200;
static constexpr size_t col_1 = col_0_w + WIZARD_MARGIN_LEFT;

DialogSelftestFans::DialogSelftestFans()
    : IDialogMarlin()
    , text_fan_test(this, Rect16(col_0, 40, WIZARD_X_SPACE, 22), is_multiline::no, is_closed_on_click_t::no, _("Fan Test"))
    , progress(this, Rect16(col_0, 62, WIZARD_X_SPACE, 8), COLOR_LIME, COLOR_BLACK)
    , text_hotend_fan(this, Rect16(col_0, 74, col_0_w, 22), is_multiline::no, is_closed_on_click_t::no, _("Hotend Fan"))
    , icon_hotend_fan(this, { col_1, 74 })
    , text_print_fan(this, Rect16(col_0, 96, col_0_w, 22), is_multiline::no, is_closed_on_click_t::no, _("Print Fan"))
    , icon_print_fan(this, { col_1, 96 }) {
}

bool DialogSelftestFans::change(uint8_t phs, uint8_t progress_tot, uint8_t progress_state) {
    PhasesSelftestFans test = GetEnumFromPhaseIndex<PhasesSelftestFans>(phs);
    SelftestSubtestState_t test_state = SelftestSubtestState_t(progress_state);

    switch (test) {
    case PhasesSelftestFans::TestFan0:
        icon_print_fan.SetState(test_state);
        break;
    case PhasesSelftestFans::TestFan1:
        icon_hotend_fan.SetState(test_state);
        break;
    }
    progress.SetProgressPercent(progress_tot);
    return true;
};
