/**
 * @file selftest_frame_loadcell.cpp
 * @author Radek Vana
 * @date 2021-12-03
 */

#include "selftest_frame_loadcell.hpp"
#include "i18n.h"
#include "img_resources.hpp"
#include <guiconfig/wizard_config.hpp>
#include "selftest_loadcell_type.hpp"
#include "marlin_client.hpp"
#include <array>

static constexpr size_t col_texts = WizardDefaults::MarginLeft;

static constexpr size_t txt_h = WizardDefaults::txt_h;
static constexpr size_t row_h = WizardDefaults::row_h;
static const size_t txt_big_h = height(Font::big);

static constexpr size_t row_2 = WizardDefaults::row_1 + WizardDefaults::progress_row_h;
static constexpr size_t row_4 = row_2 + row_h * 2;
static constexpr size_t row_6 = row_4 + row_h * 2;

static constexpr size_t top_of_changeable_area = WizardDefaults::row_1 + WizardDefaults::progress_h;
static constexpr size_t height_of_changeable_area = WizardDefaults::RectRadioButton(1).Top() - top_of_changeable_area;
static constexpr Rect16 ChangeableRect = { col_texts, top_of_changeable_area, WizardDefaults::X_space, height_of_changeable_area };

static constexpr const char *en_text_loadcell_test = N_("Loadcell Test");

// hand icon is 154x100
static constexpr int16_t hand_w = 154;
static constexpr int16_t hand_h = 100;
static constexpr int16_t hand_x_offset = -4;
static constexpr int16_t hand_y_offset = -8;
static constexpr point_i16_t hand_pos = {
    WizardDefaults::RectRadioButton(1).Left() + WizardDefaults::RectRadioButton(1).Width() - hand_w + hand_x_offset,
    WizardDefaults::RectRadioButton(1).Top() - hand_h + hand_y_offset
};

SelftestFrameLoadcell::SelftestFrameLoadcell(window_t *parent, PhasesSelftest ph, fsm::PhaseData data)
    : SelftestFrameNamedWithRadio(parent, ph, data, _(en_text_loadcell_test), 1)
    , footer(this, 0, footer::Item::nozzle, footer::Item::bed, footer::Item::axis_z) // ItemAxisZ to show Z coord while moving up
    , progress(this, WizardDefaults::row_1)
    , icon_hand(this, &img::hand_with_nozzle1_154x100, hand_pos)
    , text_phase(this, Rect16(col_texts, row_2, WizardDefaults::X_space, txt_h * 3), is_multiline::yes)
    , text_big(this, Rect16(0, row_6, hand_pos.x, txt_big_h))
    , text_result(this, ChangeableRect, is_multiline::no) {
    text_result.SetAlignment(Align_t::Center());
    text_big.set_font(Font::big);
    text_big.SetBlinkColor(COLOR_ORANGE); // Blink orange if temperature is to high
    text_big.SetAlignment(Align_t::Center());

    change();
}

void SelftestFrameLoadcell::change() {
    SelftestLoadcell_t dt(data_current);

    const char *txt_phase = nullptr; // text_phase
    const char *txt_result = nullptr; // text_result
    string_view_utf8 txt_big; // text_big
    bool txt_big_blink = false; // text_big
    const img::Resource *icon_id = nullptr; // icon_hand
    switch (phase_current) {
    case PhasesSelftest::Loadcell_prepare:
        txt_phase = N_("Validity check");
        break;
    case PhasesSelftest::Loadcell_move_away:
#if (PRINTER_IS_PRUSA_XL() || PRINTER_IS_PRUSA_iX())
        txt_phase = N_("Moving down");
#else
        txt_phase = N_("Moving up");
#endif
        break;
    case PhasesSelftest::Loadcell_tool_select:
        txt_phase = N_("Selecting tool");
        break;
    case PhasesSelftest::Loadcell_cooldown: {
        txt_phase = N_("Cooling down. Do not touch the nozzle!");
        icon_id = &img::hand_with_nozzle0_154x100;

        int16_t temperature = dt.temperature; // Make a local copy
        if ((temperature < 0) || (temperature > 999)) {
            snprintf(txt_big_buffer, std::size(txt_big_buffer), "- \xC2\xB0\x43"); // Degree Celsius
        } else {
            snprintf(txt_big_buffer, std::size(txt_big_buffer), "%u \xC2\xB0\x43", static_cast<unsigned int>(temperature));
        }
        txt_big = string_view_utf8::MakeRAM(reinterpret_cast<uint8_t *>(txt_big_buffer));
        txt_big_blink = true;

        break;
    }
    case PhasesSelftest::Loadcell_user_tap_ask_abort:
        txt_phase = dt.pressed_too_soon ? N_("You did not tap the nozzle or you tapped it too soon. Retry?\n\n ") : N_("We will need your help with this test. You will be asked to tap the nozzle. Don't worry; it is going to be cold.\n ");
        icon_id = dt.pressed_too_soon ? &img::hand_with_nozzle2_154x100 : &img::hand_with_nozzle3_154x100;
        break;
    case PhasesSelftest::Loadcell_user_tap_countdown:
        icon_id = &img::hand_with_nozzle1_154x100;

        snprintf(txt_big_buffer, std::size(txt_big_buffer), "%u s", static_cast<unsigned int>(std::clamp<uint8_t>(dt.countdown, 0, 5)));
        txt_big = string_view_utf8::MakeRAM(reinterpret_cast<uint8_t *>(txt_big_buffer));
        txt_phase = N_("Tap the nozzle on beep");

        break;
    case PhasesSelftest::Loadcell_user_tap_check:
        icon_id = &img::hand_with_nozzle4_154x100;

        txt_big = _("NOW");
        txt_phase = N_("Tap the nozzle");

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

    if (!txt_big.isNULLSTR()) {
        text_big.Show();
        if (txt_big_blink) {
            text_big.EnableBlink();
        } else {
            text_big.DisableBlink();
        }
        text_big.SetText(txt_big);
        text_big.Invalidate();
    } else {
        text_big.Hide();
    }

    if (txt_result) {
        text_result.Show();
        text_result.SetText(_(txt_result));
    } else {
        text_result.Hide();
    }

    // TODO move to main thread???
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
