#include "DialogSelftestTemp.hpp"
#include "i18n.h"
#include "wizard_config.hpp"
#include "selftest_heaters_type.hpp"

static constexpr size_t icon_w = 20 + WizardDefaults::MarginRight;
static constexpr size_t text_w = WizardDefaults::X_space - icon_w;
static constexpr size_t col_0 = WizardDefaults::MarginLeft;
static constexpr size_t col_1 = col_0 + text_w;

static constexpr size_t txt_h = WizardDefaults::txt_h;
static constexpr size_t row_h = WizardDefaults::row_h;

//noz
static constexpr size_t row_noz_0 = WizardDefaults::row_0;
static constexpr size_t row_noz_1 = row_noz_0 + row_h;
static constexpr size_t row_noz_2 = row_noz_1 + WizardDefaults::progress_row_h;
static constexpr size_t row_noz_3 = row_noz_2 + row_h;
//bed
static constexpr size_t row_bed_0 = row_noz_3 + row_h + 16;
static constexpr size_t row_bed_1 = row_bed_0 + row_h;
static constexpr size_t row_bed_2 = row_bed_1 + WizardDefaults::progress_row_h;
static constexpr size_t row_bed_3 = row_bed_2 + row_h;

static constexpr const char *en_text_noz = N_("Nozzle heater check");
static constexpr const char *en_text_bed = N_("Heatbed heater check");
static constexpr const char *en_text_prep = N_("Preparing");
static constexpr const char *en_text_heat = N_("Heater testing");

DialogSelftestTemp::DialogSelftestTemp()
    : IDialogMarlin()
    //noz
    , text_noz(this, Rect16(col_0, row_noz_0, WizardDefaults::X_space, txt_h), is_multiline::no, is_closed_on_click_t::no, _(en_text_noz))
    , progress_noz(this, row_noz_1)
    , text_noz_prep(this, Rect16(col_0, row_noz_2, text_w, txt_h), is_multiline::no, is_closed_on_click_t::no, _(en_text_prep))
    , icon_noz_prep(this, { col_1, row_noz_2 })
    , text_noz_heat(this, Rect16(col_0, row_noz_3, text_w, txt_h), is_multiline::no, is_closed_on_click_t::no, _(en_text_heat))
    , icon_noz_heat(this, { col_1, row_noz_3 })
    //bed
    , text_bed(this, Rect16(col_0, row_bed_0, WizardDefaults::X_space, txt_h), is_multiline::no, is_closed_on_click_t::no, _(en_text_bed))
    , progress_bed(this, row_bed_1)
    , text_bed_prep(this, Rect16(col_0, row_bed_2, text_w, txt_h), is_multiline::no, is_closed_on_click_t::no, _(en_text_prep))
    , icon_bed_prep(this, { col_1, row_bed_2 })
    , text_bed_heat(this, Rect16(col_0, row_bed_3, text_w, txt_h), is_multiline::no, is_closed_on_click_t::no, _(en_text_heat))
    , icon_bed_heat(this, { col_1, row_bed_3 }) {
}

bool DialogSelftestTemp::change(uint8_t phase, fsm::PhaseData data) {
    SelftestHeaters_t dt(data);

    icon_noz_prep.SetState(dt.noz_prep_state);
    icon_noz_heat.SetState(dt.noz_heat_state);
    icon_bed_prep.SetState(dt.bed_prep_state);
    icon_bed_heat.SetState(dt.bed_heat_state);

    progress_noz.SetProgressPercent(dt.noz_progress);
    progress_bed.SetProgressPercent(dt.bed_progress);

    return true;
};
