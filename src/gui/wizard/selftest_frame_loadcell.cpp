/**
 * @file selftest_frame_loadcell.cpp
 * @author Radek Vana
 * @date 2021-12-03
 */

#include "selftest_frame_loadcell.hpp"
#include "i18n.h"
#include "png_resources.hpp"
#include "wizard_config.hpp"
#include "selftest_loadcell_type.hpp"
#include "marlin_client.hpp"
#include <array>

static constexpr size_t col_texts = WizardDefaults::MarginLeft;

static constexpr size_t txt_h = WizardDefaults::txt_h;
static constexpr size_t row_h = WizardDefaults::row_h;
static constexpr size_t txt_big_h = 53;
static constexpr int16_t big_text_x_pos = col_texts + 158;

static constexpr size_t row_2 = WizardDefaults::row_1 + WizardDefaults::progress_row_h;
static constexpr size_t row_3 = row_2 + row_h + txt_h + row_h;                // double line text: row_h + txt_h, extra space row_3
static constexpr size_t row_countdown_txt4 = row_3 + (txt_big_h - txt_h) / 2; // offset to middle of big font

static constexpr size_t top_of_changeable_area = WizardDefaults::row_1 + WizardDefaults::progress_h;
static constexpr size_t height_of_changeable_area = WizardDefaults::RectRadioButton(1).Top() - top_of_changeable_area;
static constexpr Rect16 ChangeableRect = { col_texts, top_of_changeable_area, WizardDefaults::X_space, height_of_changeable_area };

static constexpr const char *en_text_loadcell_test = N_("Loadcell Test");

static constexpr const char *en_text_phase_ok = N_("Test OK");
static constexpr const char *en_text_failed = N_("Test FAILED!");

static std::array<char, 3> txt_cntdown = { "0s" };
static const string_view_utf8 txt_cntdown_view = string_view_utf8::MakeRAM((const uint8_t *)txt_cntdown.begin());

// hand icon is 154x100
static constexpr int16_t hand_w = 154;
static constexpr int16_t hand_h = 100;
static constexpr int16_t hand_x_offset = -8;
static constexpr int16_t hand_y_offset = -8;
static constexpr point_i16_t hand_pos = {
    WizardDefaults::RectRadioButton(1).Left() + WizardDefaults::RectRadioButton(1).Width() - hand_w + hand_x_offset,
    WizardDefaults::RectRadioButton(1).Top() - hand_h + hand_y_offset
};

SelftestFrameLoadcell::SelftestFrameLoadcell(window_t *parent, PhasesSelftest ph, fsm::PhaseData data)
    : AddSuperWindow<SelftestFrameNamedWithRadio>(parent, ph, data, _(en_text_loadcell_test), 1)
    , footer(this, 0, footer::Item::Nozzle, footer::Item::Bed, footer::Item::AxisZ) // ItemAxisZ to show Z coord while moving up
    , progress(this, WizardDefaults::row_1)
    , icon_hand(this, &png::hand_with_nozzle1_154x100, hand_pos)
    , text_phase(this, Rect16(col_texts, row_2, WizardDefaults::X_space, txt_h * 2), is_multiline::yes)
    , text_phase_additional(this, Rect16(col_texts, row_3, hand_pos.x - col_texts, hand_pos.y + hand_h - row_3), is_multiline::yes)
    , text_tap(this, Rect16(col_texts, row_countdown_txt4, big_text_x_pos - col_texts, txt_h), is_multiline::no)
    , text_tap_countdown(this, Rect16(big_text_x_pos, row_3, hand_pos.x - big_text_x_pos, txt_big_h), is_multiline::no)
    , text_result(this, ChangeableRect, is_multiline::no) {
    text_result.SetAlignment(Align_t::Center());
    text_tap_countdown.SetFont(resource_font(IDR_FNT_LARGE));

    change();
}

void SelftestFrameLoadcell::change() {
    SelftestLoadcell_t dt(data_current);

    const char *txt_phase = nullptr;        // text_phase
    const char *txt_result = nullptr;       // text_result
    const char *txt_tap = nullptr;          // text_tap
    const png::Resource *icon_id = nullptr; // icon_hand
    bool show_countdown = false;            // text_tap_countdown
    switch (phase_current) {
    case PhasesSelftest::Loadcell_prepare:
        txt_phase = N_("Validity check");
        break;
    case PhasesSelftest::Loadcell_move_away:
#if (PRINTER_TYPE == PRINTER_PRUSA_XL || PRINTER_TYPE == PRINTER_PRUSA_IXL)
        txt_phase = N_("Moving down");
#else
        txt_phase = N_("Moving up");
#endif
        break;
    case PhasesSelftest::Loadcell_tool_select:
        txt_phase = N_("Selecting tool");
        break;
    case PhasesSelftest::Loadcell_cooldown:
        txt_phase = N_("Cooling down.\n\nDo not touch the nozzle!");
        icon_id = &png::hand_with_nozzle0_154x100;
        break;
    case PhasesSelftest::Loadcell_user_tap_ask_abort:
        txt_phase = dt.pressed_too_soon ? N_("You did not tap the nozzle or you tapped it too soon. Retry?\n\n ") : N_("We will need your help with this test. You will be asked to tap the nozzle. Don't worry; it is going to be cold.\n ");
        icon_id = dt.pressed_too_soon ? &png::hand_with_nozzle2_154x100 : &png::hand_with_nozzle3_154x100;
        break;
    case PhasesSelftest::Loadcell_user_tap_countdown:
        icon_id = &png::hand_with_nozzle1_154x100;

        show_countdown = true;
        txt_cntdown[0] = '0' + dt.countdown;
        text_tap_countdown.SetText(txt_cntdown_view);
        text_tap_countdown.Invalidate();
        txt_tap = N_("Tap nozzle in");

        break;
    case PhasesSelftest::Loadcell_user_tap_check:
        icon_id = &png::hand_with_nozzle4_154x100;

        show_countdown = true;
        text_tap_countdown.SetText(_("NOW"));
        txt_tap = N_("Tap nozzle");

        break;
    case PhasesSelftest::Loadcell_user_tap_ok:
        txt_result = N_("Loadcell test passed OK.");
        break;
    case PhasesSelftest::Loadcell_fail:
        txt_result = N_("Loadcell test failed.");
        break;
    default:
        break;
    }

    if (txt_phase) {
        text_phase.Show();
        text_phase.SetText(_(txt_phase));
    } else {
        text_phase.Hide();
    }

    if (icon_id) {
        icon_hand.Show();
        icon_hand.SetRes(icon_id);
    } else {
        icon_hand.Hide();
    }

    if (txt_tap) {
        text_tap.Show();
        text_tap.SetText(_(txt_tap));
    } else {
        text_tap.Hide();
    }

    if (show_countdown) {
        text_tap_countdown.Show();
    } else {
        text_tap_countdown.Hide();
    }

    if (txt_result) {
        text_result.Show();
        text_result.SetText(_(txt_result));
    } else {
        text_result.Hide();
    }

    //TODO move to main thread???
    switch (phase_current) {
    case PhasesSelftest::Loadcell_user_tap_countdown:
    case PhasesSelftest::Loadcell_user_tap_check:
    case PhasesSelftest::Loadcell_user_tap_ok:
        progress.SetProgressPercent(dt.progress);
        break;
    default:
        progress.SetProgressPercent(0);
        break;
    }
};
