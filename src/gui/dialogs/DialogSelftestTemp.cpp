#include "DialogSelftestTemp.hpp"
#include "i18n.h"
#include "wizard_config.hpp"

static constexpr size_t text_w = 85;

static constexpr size_t col_0 = WIZARD_MARGIN_LEFT;
static constexpr size_t col_1 = col_0 + text_w;
static constexpr size_t col_2 = GuiDefaults::RectScreen.Width() / 2;
static constexpr size_t col_3 = col_2 + text_w;

static constexpr size_t txt_h = 22;
static constexpr size_t row_h = 24;

static constexpr size_t row_0 = 40;
static constexpr size_t row_1 = 84;
static constexpr size_t row_2 = 96;
static constexpr size_t row_3 = row_2 + row_h;
static constexpr size_t row_4 = row_3 + row_h + 5;
static constexpr size_t row_5 = row_4 + row_h;

static constexpr const char *en_text_checking_temp = N_("Checking hotend and heatbed heaters");
static constexpr const char *en_text_noz = N_("Hotend");
static constexpr const char *en_text_bed = N_("Heatbed");
static constexpr const char *en_text_cool = N_("Cooling");
static constexpr const char *en_text_heat = N_("Heating");

DialogSelftestTemp::DialogSelftestTemp()
    : IDialogMarlin()
    , text_checking_temp(this, Rect16(col_0, row_0, WIZARD_X_SPACE, txt_h * 2), is_multiline::yes, is_closed_on_click_t::no, _(en_text_checking_temp))
    , progress(this, Rect16(col_0, row_1, WIZARD_X_SPACE, 8), COLOR_LIME, COLOR_BLACK)
    , text_noz(this, Rect16(col_0, row_2, text_w, txt_h), is_multiline::no, is_closed_on_click_t::no, _(en_text_noz))
    , text_noz_cool(this, Rect16(col_0, row_3, text_w, txt_h), is_multiline::no, is_closed_on_click_t::no, _(en_text_cool))
    , icon_noz_cool(this, { col_1, row_3 })
    , text_noz_heat(this, Rect16(col_2, row_3, text_w, txt_h), is_multiline::no, is_closed_on_click_t::no, _(en_text_heat))
    , icon_noz_heat(this, { col_3, row_3 })
    , text_bed(this, Rect16(col_0, row_4, text_w, txt_h), is_multiline::no, is_closed_on_click_t::no, _(en_text_bed))
    , text_bed_cool(this, Rect16(col_0, row_5, text_w, txt_h), is_multiline::no, is_closed_on_click_t::no, _(en_text_cool))
    , icon_bed_cool(this, { col_1, row_5 })
    , text_bed_heat(this, Rect16(col_2, row_5, text_w, txt_h), is_multiline::no, is_closed_on_click_t::no, _(en_text_heat))
    , icon_bed_heat(this, { col_3, row_5 }) {
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
    progress.SetProgressPercent(progress_tot);
    return true;
};
