/**
 * @file selftest_frame_axis.cpp
 * @author Radek Vana
 * @date 2021-12-03
 */

#include "selftest_frame_axis.hpp"
#include "i18n.h"
#include "wizard_config.hpp"
#include "selftest_axis_type.hpp"
#include "img_resources.hpp"

static constexpr size_t X_AXIS_PERCENT = 33;
static constexpr size_t Y_AXIS_PERCENT = 33;
static constexpr size_t Z_AXIS_PERCENT = 34;
// would be cleaner to calculate total progress in main thread, but cannot pass so much data

static constexpr size_t col_texts = WizardDefaults::col_after_icon;
static constexpr size_t col_results = WizardDefaults::status_icon_X_pos;
static constexpr size_t col_texts_w = col_results - col_texts;

static constexpr size_t row_2 = WizardDefaults::row_1 + WizardDefaults::progress_row_h;
static constexpr size_t row_3 = row_2 + WizardDefaults::row_h;
static constexpr size_t row_4 = row_3 + WizardDefaults::row_h;
static constexpr size_t row_5 = row_4 + WizardDefaults::row_h + 20;

static constexpr const char *en_text_axis_test = N_("Checking axes");
static constexpr const char *en_text_X_axis = N_("axis");
static constexpr const char *en_text_Y_axis = N_("axis");
static constexpr const char *en_text_Z_axis = N_("axis");
static constexpr const char *en_text_info = N_("During the test, the heatbed, and extruder will move in full range.\n\nPrinter may vibrate and be noisier during homing.");

SelftestFrametAxis::SelftestFrametAxis(window_t *parent, PhasesSelftest ph, fsm::PhaseData data)
    : AddSuperWindow<SelftestFrameNamed>(parent, ph, data, _(en_text_axis_test))
    , footer(this, 0, footer::Item::axis_x, footer::Item::axis_y, footer::Item::axis_z)
    , progress(this, WizardDefaults::row_1)
    , icon_x_axis(this, &img::x_axis_16x16, point_i16_t({ WizardDefaults::col_0, row_2 }))
    , text_x_axis(this, Rect16(col_texts, row_2, col_texts_w, WizardDefaults::txt_h), is_multiline::no, is_closed_on_click_t::no, _(en_text_X_axis))
    , icon_x_axis_state(this, { col_results, row_2 })
    , icon_y_axis(this, &img::y_axis_16x16, point_i16_t({ WizardDefaults::col_0, row_3 }))
    , text_y_axis(this, Rect16(col_texts, row_3, col_texts_w, WizardDefaults::txt_h), is_multiline::no, is_closed_on_click_t::no, _(en_text_Y_axis))
    , icon_y_axis_state(this, { col_results, row_3 })
    , icon_z_axis(this, &img::z_axis_16x16, point_i16_t({ WizardDefaults::col_0, row_4 }))
    , text_z_axis(this, Rect16(col_texts, row_4, col_texts_w, WizardDefaults::txt_h), is_multiline::no, is_closed_on_click_t::no, _(en_text_Z_axis))
    , icon_z_axis_state(this, { col_results, row_4 })
    , text_info(this, Rect16(col_texts, row_5, WizardDefaults::X_space, GetRect().Height() - GetRect().Top() - row_4), is_multiline::yes, is_closed_on_click_t::no, _(en_text_info)) {

#ifdef USE_ST7789
    text_info.set_font(GuiDefaults::FontMenuSpecial);
#endif

    change();
}

void SelftestFrametAxis::change() {
    SelftestAxis_t dt(data_current);

    bool show_x = false;
    bool show_y = false;
    bool show_z = false;

    switch (dt.axis) {
    case X_AXIS:
        show_x = true;
        progress.SetProgressPercent(dt.x_progress);
        break;
    case Y_AXIS:
        show_y = true;
        progress.SetProgressPercent(dt.y_progress);
        icon_y_axis.Reposition(Rect16::Top_t(row_2));
        text_y_axis.Reposition(Rect16::Top_t(row_2));
        icon_y_axis_state.Reposition(Rect16::Top_t(row_2));
        break;
    case Z_AXIS:
        show_z = true;
        progress.SetProgressPercent(dt.z_progress);
        icon_z_axis.Reposition(Rect16::Top_t(row_2));
        text_z_axis.Reposition(Rect16::Top_t(row_2));
        icon_z_axis_state.Reposition(Rect16::Top_t(row_2));
        break;
    default: // Show joined progress when testing all 3 axes together
        show_x = true;
        show_y = true;
        show_z = true;
        progress.SetProgressPercent((dt.x_progress * X_AXIS_PERCENT + dt.y_progress * Y_AXIS_PERCENT + dt.z_progress * Z_AXIS_PERCENT) / 100);
        icon_y_axis.Reposition(Rect16::Top_t(row_3));
        text_y_axis.Reposition(Rect16::Top_t(row_3));
        icon_y_axis_state.Reposition(Rect16::Top_t(row_3));
        icon_z_axis.Reposition(Rect16::Top_t(row_4));
        text_z_axis.Reposition(Rect16::Top_t(row_4));
        icon_z_axis_state.Reposition(Rect16::Top_t(row_4));
        break;
    }

    if (show_x) {
        icon_x_axis_state.SetState(dt.x_state);
        icon_x_axis.Show();
        icon_x_axis_state.Show();
        text_x_axis.Show();
    } else {
        icon_x_axis.Hide();
        icon_x_axis_state.Hide();
        text_x_axis.Hide();
    }

    if (show_y) {
        icon_y_axis_state.SetState(dt.y_state);
        icon_y_axis.Show();
        icon_y_axis_state.Show();
        text_y_axis.Show();
    } else {
        icon_y_axis.Hide();
        icon_y_axis_state.Hide();
        text_y_axis.Hide();
    }

    if (show_z) {
        icon_z_axis_state.SetState(dt.z_state);
        icon_z_axis.Show();
        icon_z_axis_state.Show();
        text_z_axis.Show();
    } else {
        icon_z_axis.Hide();
        icon_z_axis_state.Hide();
        text_z_axis.Hide();
    }
};
