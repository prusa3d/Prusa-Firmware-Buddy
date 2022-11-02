/**
 * @file selftest_frame_fans.cpp
 * @author Radek Vana
 * @date 2021-12-03
 */

#include "selftest_frame_fans.hpp"
#include "i18n.h"
#include "wizard_config.hpp"
#include "selftest_fans_type.hpp"
#include "png_resources.hpp"

static constexpr size_t col_texts = WizardDefaults::col_after_icon;
static constexpr size_t col_results = WizardDefaults::status_icon_X_pos;
static constexpr size_t col_texts_w = col_results - col_texts;

static constexpr size_t row_2 = WizardDefaults::row_1 + WizardDefaults::progress_row_h;
static constexpr size_t row_3 = row_2 + WizardDefaults::row_h;
static constexpr size_t row_4 = row_3 + WizardDefaults::row_h + 20;

static constexpr Rect16 rc = Rect16(WizardDefaults::col_0, row_4, WizardDefaults::X_space, WizardDefaults::RectSelftestFrame.Height() - WizardDefaults::RectSelftestFrame.Top() - row_4);

static constexpr const char *en_text_fan_test = N_("Fan Test");
static constexpr const char *en_text_hotend_fan = N_("Hotend Fan");
static constexpr const char *en_text_print_fan = N_("Print Fan");
static constexpr const char *en_text_info = N_("During the test, the expected fan speeds are checked for 30 seconds."); // TODO calculate time, fine test takes longer

SelftestFrameFans::SelftestFrameFans(window_t *parent, PhasesSelftest ph, fsm::PhaseData data)
    : AddSuperWindow<SelftestFrameNamed>(parent, ph, data, _(en_text_fan_test))
    , footer(this, 0, footer::items::ItemPrintFan, footer::items::ItemHeatbreakFan)
    , progress(this, WizardDefaults::row_1)
    , icon_hotend_fan(this, &png::fan_16x16, point_i16_t({ WizardDefaults::col_0, row_2 }))
    , text_hotend_fan(this, Rect16(col_texts, row_2, col_texts_w, WizardDefaults::txt_h), is_multiline::no, is_closed_on_click_t::no, _(en_text_hotend_fan))
    , icon_hotend_fan_state(this, { col_results, row_2 })
    , icon_print_fan(this, &png::turbine_16x16, point_i16_t({ WizardDefaults::col_0, row_3 }))
    , text_print_fan(this, Rect16(col_texts, row_3, col_texts_w, WizardDefaults::txt_h), is_multiline::no, is_closed_on_click_t::no, _(en_text_print_fan))
    , icon_print_fan_state(this, { col_results, row_3 })
    , text_info(this, Rect16(col_texts, row_4, WizardDefaults::X_space, GetRect().Height() - GetRect().Top() - row_4), is_multiline::yes, is_closed_on_click_t::no, _(en_text_info)) {

    change();
}

void SelftestFrameFans::change() {
    SelftestFans_t dt(data_current);

    icon_print_fan_state.SetState(dt.print_fan_state);
    icon_hotend_fan_state.SetState(dt.heatbreak_fan_state);
    progress.SetProgressPercent(dt.tot_progress);
};
