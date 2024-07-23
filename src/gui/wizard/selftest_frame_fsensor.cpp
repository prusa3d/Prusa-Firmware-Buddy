/**
 * @file selftest_frame_fsensor.cpp
 * @author Radek Vana
 * @date 2021-12-05
 */

#include "selftest_frame_fsensor.hpp"
#include "i18n.h"
#include <guiconfig/wizard_config.hpp>
#include "selftest_fsensor_type.hpp"
#include "marlin_client.hpp"
#include "img_resources.hpp"
#include <option/has_side_fsensor.h>
#include <option/has_mmu2.h>

#if HAS_MMU2()
    #include <feature/prusa/MMU2/mmu2_mk4.h>
#endif

// hourglass wuth text
static constexpr size_t row_2 = 140;
static constexpr size_t row_3 = 200;

static constexpr size_t col_0 = WizardDefaults::MarginLeft;

static constexpr size_t text_icon_space = 24;
static constexpr size_t icon_left_width = 100; // 150x130
static constexpr size_t icon_right_width = 150; // 100x100
static constexpr size_t text_left_width = WizardDefaults::X_space - icon_right_width - text_icon_space;
static constexpr size_t text_right_width = WizardDefaults::X_space - icon_left_width - text_icon_space;

static constexpr size_t top_of_changeable_area = WizardDefaults::row_1 + WizardDefaults::progress_h;
static constexpr size_t height_of_changeable_area = WizardDefaults::RectRadioButton(1).Top() - top_of_changeable_area;
static constexpr Rect16 ChangeableRect = { col_0, top_of_changeable_area, WizardDefaults::X_space, height_of_changeable_area };

static string_view_utf8 test_name() {
#if HAS_MMU2()
    if (config_store().is_mmu_rework.get()) {
        return _("MMU filament sensor calibration");
    }
#endif

#if HAS_SIDE_FSENSOR()
    return _("Filament sensors calibration");
#else
    return _("Filament sensor calibration");
#endif
}

SelftestFrameFSensor::SelftestFrameFSensor(window_t *parent, PhasesSelftest ph, fsm::PhaseData data)
    : SelftestFrameNamedWithRadio(parent, ph, data, ::test_name(), 1)

    , footer(this, 0,
#if HAS_SIDE_FSENSOR()
          footer::Item::f_sensor_side,
#endif /*HAS_SIDE_FSENSOR()*/
          footer::Item::f_sensor)
    , progress(this, WizardDefaults::row_1)
    , text_left(this, Rect16(col_0, top_of_changeable_area, text_left_width, height_of_changeable_area), is_multiline::yes)
    , text_right(this, Rect16(col_0 + icon_left_width + text_icon_space, top_of_changeable_area, text_right_width, height_of_changeable_area), is_multiline::yes)

    , icon_left(this, Rect16(col_0, top_of_changeable_area, icon_left_width, height_of_changeable_area), &img::prusament_spool_white_100x100)
    , icon_right(this, Rect16(col_0 + text_left_width + text_icon_space, top_of_changeable_area, icon_right_width, height_of_changeable_area), 0)

    , animation(this, { int16_t(GuiDefaults::ScreenWidth / 2), int16_t(row_2) })
    , text_animation(this, Rect16(WizardDefaults::col_0, row_3, WizardDefaults::X_space, WizardDefaults::row_h), is_multiline::no, is_closed_on_click_t::no, _("in progress"))
    , text_result(this, ChangeableRect, is_multiline::no) {
    progress.SetProgressPercent(100); // just orange line
    animation.SetRect(animation.GetRect() - Rect16::Left_t(animation.GetRect().Width() / 2));
    text_animation.SetAlignment(Align_t::Center());
    text_left.SetAlignment(Align_t::LeftCenter());
    text_right.SetAlignment(Align_t::LeftCenter());
    text_result.SetAlignment(Align_t::Center());

    change();
}

void SelftestFrameFSensor::change() {
    SelftestFSensor_t dt(data_current);

    const char *txt_left = nullptr;
    const char *txt_right = nullptr;
    const char *txt_result = nullptr;
    const img::Resource *right_icon_id = nullptr; // hand ok hand with checkmark
    bool show_left_icon = false; // spool
    bool show_hourglass = false;

    // texts
    switch (phase_current) {
    case PhasesSelftest::FSensor_wait_tool_pick:
        txt_right = N_("Please wait until a tool is picked");
        show_left_icon = true;
        break;
    case PhasesSelftest::FSensor_ask_unload:
#if PRINTER_IS_PRUSA_XL()
        txt_right = N_("Please make sure there is no filament in the tool and side filament sensors.\n\nYou will need filament to finish this test later.");
#else
        txt_right = N_("We need to start without the filament in the extruder. Please make sure there is no filament in the filament sensor.");
#endif
        show_left_icon = true;
        break;
    case PhasesSelftest::FSensor_unload_confirm:
#if PRINTER_IS_PRUSA_XL()
        txt_right = N_("Is there any filament in the tool or side filament sensors?");
#else
        txt_right = N_("Is filament in the filament sensor?");
#endif
        show_left_icon = true;
        break;
    case PhasesSelftest::FSensor_calibrate:
        show_hourglass = true;
        break;
    case PhasesSelftest::FSensor_insertion_wait:
#if PRINTER_IS_PRUSA_XL()
        txt_left = N_("Insert the filament through the side filament sensor into the extruder until the tool filament sensor detects the filament.");
#else
        txt_left = N_("Insert the filament into the extruder until the sensor detects the filament.");
#endif
        right_icon_id = &img::hand_with_filament_150x130;
        break;
    case PhasesSelftest::FSensor_insertion_ok:
        txt_left = N_("Filament inserted, press continue.");

        right_icon_id = &img::hand_with_filament_ok_150x130;
        break;
    case PhasesSelftest::FSensor_insertion_calibrate:
        txt_result = N_("Calibrating, do not remove filament.");
        break;
    case PhasesSelftest::Fsensor_enforce_remove:
        txt_left = N_("Remove filament to finish.");
        right_icon_id = &img::hand_with_filament_150x130;
        break;
    case PhasesSelftest::FSensor_done:
#if PRINTER_IS_PRUSA_XL()
        txt_right = N_("Filament sensors calibrated.");
#else
        txt_right = N_("Filament sensor calibrated.");
#endif
        show_left_icon = true;
        break;
    case PhasesSelftest::FSensor_fail:
        txt_result = N_("Test FAILED!");
        break;

    default:
        break;
    }

    if (txt_left) {
        text_left.Show();
        text_left.SetText(_(txt_left));
    } else {
        text_left.Hide();
    }

    if (txt_right) {
        text_right.Show();
        text_right.SetText(_(txt_right));
    } else {
        text_right.Hide();
    }

    if (txt_result) {
        text_result.Show();
        text_result.SetText(_(txt_result));
    } else {
        text_result.Hide();
    }

    if (right_icon_id) {
        icon_right.Show();
        icon_right.SetRes(right_icon_id);
    } else {
        icon_right.Hide();
    }

    if (show_left_icon) {
        icon_left.Show();
    } else {
        icon_left.Hide();
    }

    if (show_hourglass) {
        animation.Show();
        text_animation.Show();
    } else {
        animation.Hide();
        text_animation.Hide();
    }
};
