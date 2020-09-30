#include "DialogSelftestTemp.hpp"
#include "i18n.h"
#include "wizard_config.hpp"

static constexpr size_t col_0 = WIZARD_MARGIN_LEFT;
static constexpr size_t col_0_w = 200;
static constexpr size_t col_1 = col_0_w + WIZARD_MARGIN_LEFT;
static constexpr size_t text_w = 80;

DialogSelftestTemp::DialogSelftestTemp()
    : IDialogMarlin()
    , text_checking_temp(this, Rect16(col_0, 40, WIZARD_X_SPACE, 44), is_multiline::yes, is_closed_on_click_t::no, _("Checking hotend and heatbed heaters"))
    , progress(this, Rect16(col_0, 84, WIZARD_X_SPACE, 8), COLOR_LIME, COLOR_BLACK)
    , text_noz(this, Rect16(col_0, 96, text_w, 22), is_multiline::no, is_closed_on_click_t::no, _("Hotend"))
    , text_noz_cool(this, Rect16(col_0, 118, text_w, 22), is_multiline::no, is_closed_on_click_t::no, _("Cooling"))
    , icon_noz_cool(this, { col_0, 118 })
    , text_noz_heat(this, Rect16(col_1, 118, text_w, 22), is_multiline::no, is_closed_on_click_t::no, _("Heating"))
    , icon_noz_heat(this, { col_1, 118 })
    , text_bed(this, Rect16(col_0, 142, text_w, 22), is_multiline::no, is_closed_on_click_t::no, _("Heatbed"))
    , text_bed_cool(this, Rect16(col_0, 142, text_w, 22), is_multiline::no, is_closed_on_click_t::no, _("Cooling"))
    , icon_bed_cool(this, { col_0, 142 })
    , text_bed_heat(this, Rect16(col_1, 142, text_w, 22), is_multiline::no, is_closed_on_click_t::no, _("Heating"))
    , icon_bed_heat(this, { col_1, 142 }) {
}

bool DialogSelftestTemp::change(uint8_t phs, uint8_t progress_tot, uint8_t progress_state) {
    PhasesSelftestHeat test = GetEnumFromPhaseIndex<PhasesSelftestHeat>(phs);
    SelftestSubtestState_t test_state = SelftestSubtestState_t(progress_state);

    switch (test) {
    case PhasesSelftestHeat::noz_cool:
        icon_noz_cool.SetState(test_state);
        break;
    case PhasesSelftestHeat::noz_heat:
        icon_noz_heat.SetState(test_state);
        break;
    case PhasesSelftestHeat::bed_cool:
        icon_bed_cool.SetState(test_state);
        break;
    case PhasesSelftestHeat::bed_heat:
        icon_bed_heat.SetState(test_state);
        break;
    }
    progress.SetProgress(progress_tot);
    return true;
};
