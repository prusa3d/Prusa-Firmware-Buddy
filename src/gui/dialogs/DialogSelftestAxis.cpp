#include "DialogSelftestAxis.hpp"
#include "i18n.h"
#include "wizard_config.hpp"

static constexpr size_t col_0 = WIZARD_MARGIN_LEFT;
static constexpr size_t col_0_w = 200;
static constexpr size_t col_1 = col_0_w + WIZARD_MARGIN_LEFT;

DialogSelftestAxis::DialogSelftestAxis()
    : IDialogMarlin()
    , text_checking_axis(this, Rect16(col_0, 40, WIZARD_X_SPACE, 22), is_multiline::no, is_closed_on_click_t::no, _("Checking axes"))
    , progress(this, Rect16(col_0, 62, WIZARD_X_SPACE, 8), COLOR_LIME, COLOR_BLACK)
    , text_x_axis(this, Rect16(col_0, 74, col_0_w, 22), is_multiline::no, is_closed_on_click_t::no, _("X-axis"))
    , icon_x_axis(this, { col_1, 74 })
    , text_y_axis(this, Rect16(col_0, 96, col_0_w, 22), is_multiline::no, is_closed_on_click_t::no, _("Y-axis"))
    , icon_y_axis(this, { col_1, 96 })
    , text_z_axis(this, Rect16(col_0, 120, col_0_w, 22), is_multiline::no, is_closed_on_click_t::no, _("Z-axis"))
    , icon_z_axis(this, { col_1, 120 }) {
}

bool DialogSelftestAxis::change(uint8_t phs, uint8_t progress_tot, uint8_t progress_state) {
    PhasesSelftestAxis test = GetEnumFromPhaseIndex<PhasesSelftestAxis>(phs);
    SelftestSubtestState_t test_state = SelftestSubtestState_t(progress_state);

    switch (test) {
    case PhasesSelftestAxis::Xaxis:
        icon_x_axis.SetState(test_state);
        break;
    case PhasesSelftestAxis::Yaxis:
        icon_y_axis.SetState(test_state);
        break;
    case PhasesSelftestAxis::Zaxis:
        icon_z_axis.SetState(test_state);
        break;
    }
    progress.SetProgressPercent(progress_tot);
    return true;
};
