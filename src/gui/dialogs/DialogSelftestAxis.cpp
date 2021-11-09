#include "DialogSelftestAxis.hpp"
#include "i18n.h"
#include "wizard_config.hpp"
#include "selftest_axis_type.hpp"

static constexpr size_t X_AXIS_PERCENT = 33;
static constexpr size_t Y_AXIS_PERCENT = 33;
static constexpr size_t Z_AXIS_PERCENT = 34;
//would be cleaner to calculate total progress in main thread, but cannot pass so much data

static constexpr size_t col_0 = WizardDefaults::MarginLeft;
static constexpr size_t col_1 = WizardDefaults::status_icon_X_pos;
static constexpr size_t col_0_w = col_1 - col_0;

static constexpr size_t txt_h = WizardDefaults::txt_h;
static constexpr size_t row_h = WizardDefaults::row_h;

static constexpr size_t row_0 = WizardDefaults::row_0;
static constexpr size_t row_1 = row_0 + row_h;
static constexpr size_t row_2 = row_1 + WizardDefaults::progress_row_h;
static constexpr size_t row_3 = row_2 + row_h;
static constexpr size_t row_4 = row_3 + row_h;

static constexpr const char *en_text_axis_test = N_("Checking axes");
static constexpr const char *en_text_X_axis = N_("X-axis");
static constexpr const char *en_text_Y_axis = N_("Y-axis");
static constexpr const char *en_text_Z_axis = N_("Z-axis");

DialogSelftestAxis::DialogSelftestAxis()
    : IDialogMarlin()
    , text_checking_axis(this, Rect16(col_0, row_0, WizardDefaults::X_space, txt_h), is_multiline::no, is_closed_on_click_t::no, _(en_text_axis_test))
    , progress(this, row_1)
    , text_x_axis(this, Rect16(col_0, row_2, col_0_w, txt_h), is_multiline::no, is_closed_on_click_t::no, _(en_text_X_axis))
    , icon_x_axis(this, { col_1, row_2 })
    , text_y_axis(this, Rect16(col_0, row_3, col_0_w, txt_h), is_multiline::no, is_closed_on_click_t::no, _(en_text_Y_axis))
    , icon_y_axis(this, { col_1, row_3 })
    , text_z_axis(this, Rect16(col_0, row_4, col_0_w, txt_h), is_multiline::no, is_closed_on_click_t::no, _(en_text_Z_axis))
    , icon_z_axis(this, { col_1, row_4 }) {
}

bool DialogSelftestAxis::change(uint8_t phase, fsm::PhaseData data) {
    SelftestAxis_t dt(data);

    icon_x_axis.SetState(dt.x_state);
    icon_y_axis.SetState(dt.y_state);
    icon_z_axis.SetState(dt.z_state);
    progress.SetProgressPercent(
        (
            dt.x_progress * X_AXIS_PERCENT + dt.y_progress * Y_AXIS_PERCENT + dt.z_progress * Z_AXIS_PERCENT)
        / 100);
    return true;
};
