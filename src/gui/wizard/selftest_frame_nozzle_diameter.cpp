#include "selftest_frame_nozzle_diameter.hpp"

static constexpr size_t col_texts = WizardDefaults::MarginLeft;

static constexpr size_t txt_h = WizardDefaults::txt_h;
static constexpr size_t row_h = WizardDefaults::row_h;
static const size_t txt_big_h = resource_font(IDR_FNT_LARGE)->h;

static constexpr size_t row_2 = WizardDefaults::row_1 + WizardDefaults::progress_row_h;
static constexpr size_t row_4 = row_2 + row_h * 2;
static constexpr size_t row_6 = row_4 + row_h * 2;

static constexpr size_t top_of_changeable_area = WizardDefaults::row_1 + WizardDefaults::progress_h;
static constexpr size_t height_of_changeable_area = WizardDefaults::RectRadioButton(1).Top() - top_of_changeable_area;
static constexpr Rect16 ChangeableRect = { col_texts, top_of_changeable_area, WizardDefaults::X_space, height_of_changeable_area };

static constexpr const char *en_title = N_("Nozzle Diameter Config");

SelftestFrameNozzleDiameter::SelftestFrameNozzleDiameter(window_t *parent, PhasesSelftest ph, fsm::PhaseData data)
    : AddSuperWindow<SelftestFrameNamedWithRadio>(parent, ph, data, _(en_title), 1)
    , footer(this, 0, footer::Item::nozzle, footer::Item::bed, footer::Item::axis_z) // ItemAxisZ to show Z coord while moving up
    , text_header(this, Rect16(col_texts, row_2, display::GetW() - col_texts, txt_h), is_multiline::no)
    , text_details(this, Rect16(col_texts, row_4, display::GetW() - col_texts, txt_h), is_multiline::yes) {
    change();
}

void SelftestFrameNozzleDiameter::change() {
    const char *header = nullptr;
    const char *details = nullptr;

    switch (phase_current) {
    case PhasesSelftest::NozzleDiameter_prepare:
        header = N_("Preparing");
        break;
    case PhasesSelftest::NozzleDiameter_ask_user_for_type:
        header = N_("Select correct nozzle");
        details = N_("Which nozzle do you have? ...");
        break;
    case PhasesSelftest::NozzleDiameter_set_default_nozzle_type:
        header = N_("Saving data");
        break;
    default:
        break;
    }

    if (header) {
        text_header.Show();
        text_header.SetText(_(header));
    } else {
        text_header.Hide();
    }

    if (details) {
        text_details.Show();
        text_details.SetText(_(details));
    } else {
        text_details.Hide();
    }
}
