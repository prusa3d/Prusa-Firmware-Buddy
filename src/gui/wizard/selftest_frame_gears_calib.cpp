#include "selftest_frame_gears_calib.hpp"
#include "i18n.h"
#include "wizard_config.hpp"
#include "selftest_gears_result.hpp"
#include "img_resources.hpp"

static constexpr const img::Resource &right_icon = img::transmission_loose_187x175;

static constexpr size_t content_top_y = WizardDefaults::row_1 + WizardDefaults::progress_row_h;
static constexpr size_t text_icon_space = 14;
static constexpr size_t text_left_width = WizardDefaults::X_space - right_icon.w - text_icon_space;

static constexpr const char *en_text_test_name = N_("Gearbox alignment");

SelftestFrameGearsCalib::SelftestFrameGearsCalib(window_t *parent, PhasesSelftest ph, fsm::PhaseData data)
    : AddSuperWindow<SelftestFrameNamedWithRadio>(parent, ph, data, _(en_text_test_name), 1)

    , footer(this, 0, footer::Item::nozzle, footer::Item::bed, footer::Item::f_sensor)
    , progress(this, WizardDefaults::row_1)
    , text(this, Rect16(WizardDefaults::col_0, content_top_y, WizardDefaults::RectSelftestFrame.Width() - WizardDefaults::MarginRight - WizardDefaults::MarginLeft, right_icon.h), is_multiline::yes)
    , text_left(this, Rect16(WizardDefaults::col_0, content_top_y, text_left_width, right_icon.h), is_multiline::yes)
    , icon(this, &right_icon, point_i16_t(WizardDefaults::RectSelftestFrame.Width() - WizardDefaults::MarginRight - right_icon.w, content_top_y))

{
    progress.SetProgressPercent(100); // just orange line
    change();
}

void SelftestFrameGearsCalib::change() {
    SelftestGearsResult result(data_current);

    const char *txt = nullptr;
    const img::Resource *icon_res = nullptr;
    const char *txt_left = nullptr;

    switch (phase_current) {
    case PhasesSelftest::GearsCalib_filament_check:
        txt = N_("The gearbox calibration is only necessary for user-assembled or serviced gearboxes. In all other cases, you can skip this step.");
        break;
    case PhasesSelftest::GearsCalib_filament_loaded_ask_unload:
        txt = N_("We need to start without the filament in the extruder. Please unload it.");
        break;
    case PhasesSelftest::GearsCalib_filament_unknown_ask_unload:
        txt = N_("Before you proceed, make sure filament is unloaded from the Nextruder.");
        break;
    case PhasesSelftest::GearsCalib_release_screws:
        icon_res = &img::transmission_loose_187x175;
        txt_left = N_("Rotate each screw counter-clockwise by 1.5 turns. The screw heads should be flush with the cover. Unlock and open the idler.");
        break;
    case PhasesSelftest::GearsCalib_alignment:
        icon_res = &img::transmission_gears_187x175;
        txt_left = N_("Gearbox alignment in progress, please wait (approx. 20 seconds)");
        break;
    case PhasesSelftest::GearsCalib_tighten:
        icon_res = &img::transmission_tight_187x175;
        txt_left = N_("Tighten the M3 screws firmly in the correct order, they should be slightly below the surface. Do not over-tighten.");
        break;
    case PhasesSelftest::GearsCalib_done:
        icon_res = &img::transmission_close_187x175;
        txt_left = N_("Close the idler door and secure it with the swivel. The calibration is done!");
        break;
    default:
        break;
    }

    if (txt) {
        text.Show();
        text.SetText(_(txt));
    } else {
        text.Hide();
    }

    if (txt_left) {
        text_left.Show();
        text_left.SetText(_(txt_left));
    } else {
        text_left.Hide();
    }

    if (icon_res) {
        icon.SetRes(icon_res);
        icon.Show();
    } else {
        icon.Hide();
    }
};
