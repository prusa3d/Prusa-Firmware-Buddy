#include "selftest_frame_nozzle_diameter.hpp"

namespace {

constexpr size_t col_texts = WizardDefaults::MarginLeft;

constexpr size_t txt_h = WizardDefaults::txt_h;
constexpr size_t row_h = WizardDefaults::row_h;

constexpr size_t row_2 = WizardDefaults::row_1 + WizardDefaults::progress_row_h;
constexpr size_t row_3_5 = row_2 + row_h + (row_h / 2);

constexpr const char *en_title = N_("Nozzle Diameter Confirmation");
} // namespace

SelftestFrameNozzleDiameter::SelftestFrameNozzleDiameter(window_t *parent, PhasesSelftest ph, fsm::PhaseData data)
    : AddSuperWindow<SelftestFrameNamedWithRadio>(parent, ph, data, _(en_title), 1)
    , footer(this, 0, footer::Item::nozzle, footer::Item::bed, footer::Item::axis_z) // ItemAxisZ to show Z coord while moving up
    , text_header(this, Rect16(col_texts, row_3_5, display::GetW() - (2 * col_texts), txt_h), is_multiline::no)
    , text_details(this, Rect16(col_texts, row_2, display::GetW() - (2 * col_texts), 7 * txt_h), is_multiline::yes) {
    text_details.set_font(Font::small);
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
        header = nullptr;
        details = N_("Select the correct nozzle diameter by counting the markings (dots) on the nozzle:\n"
                     "  0.40 mm nozzle: 3 dots\n"
                     "  0.60 mm nozzle: 4 dots\n\n"
                     "For more information, visit prusa.io/nozzle-types");
        break;
    case PhasesSelftest::NozzleDiameter_save_selected_value:
        header = N_("Saving selected value");
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
