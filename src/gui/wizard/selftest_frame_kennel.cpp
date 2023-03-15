#include "selftest_frame_kennel.hpp"
#include "i18n.h"
#include "png_resources.hpp"
#include "wizard_config.hpp"

LOG_COMPONENT_REF(Selftest);
static constexpr size_t col_texts = WizardDefaults::MarginLeft;
static constexpr size_t txt_h = WizardDefaults::txt_h;
static constexpr size_t row_2 = WizardDefaults::row_1 + WizardDefaults::progress_row_h;
static constexpr size_t row_3 = row_2 + txt_h;
static constexpr size_t row_4 = row_3 + txt_h;

SelftestFrameKennel::SelftestFrameKennel(window_t *parent, PhasesSelftest ph, fsm::PhaseData data)
    : AddSuperWindow<SelftestFrameNamedWithRadio>(parent, ph, data, _("Kennel Calibration"), 1)
    , footer(this, 0, footer::items::ItemNozzle, footer::items::ItemBed, footer::items::ItemAxisZ) // ItemAxisZ to show Z coord while moving up
    , progress(this, WizardDefaults::row_1)
    , text_info(this, get_info_text_rect(), is_multiline::yes)
    , icon_warning(this, &png::printer_is_moving, point_i16(col_texts, row_4))
    , text_warning(this, Rect16(col_texts + png::warning_48x48.w + 20, row_4, WizardDefaults::X_space - png::warning_48x48.w - 20, 2 * txt_h), is_multiline::yes)
    , icon_info(this, &png::parking1, text_info.GetRect().TopRight())
    , qr(this, get_info_icon_rect() + Rect16::Left_t(25), "help.prusa3d.com/12201") {
    qr.Hide();
    icon_warning.Hide();
    icon_info.Hide();
    text_warning.Hide();
    change();
}

void SelftestFrameKennel::change() {
    const SelftestKennels_t kennel_data(data_current);
    set_name(kennel_data);

    switch (phase_current) {
    case PhasesSelftest::Kennel_needs_calibration:
        set_prologue();
        break;

    case PhasesSelftest::Kennel_wait_user_park1:
        set_info_layout(_(PARK1), &png::parking1);
        break;

    case PhasesSelftest::Kennel_wait_user_park2:
        set_info_layout(_(PARK2), &png::parking2);
        break;

    case PhasesSelftest::Kennel_wait_user_park3:
        set_info_layout(_(PARK3), &png::parking2);
        break;

    case PhasesSelftest::Kennel_pin_remove_prepare:
        text_info.SetText(_("Preparing for kennel pin removal."));
        break;

    case PhasesSelftest::Kennel_wait_user_remove_pins:
        set_info_layout(_(REMOVE_KENNEL_PINS), &png::loosen_screws2);
        break;

    case PhasesSelftest::Kennel_wait_user_loosen_pillar:
        set_info_layout(_(LOOSEN_KENNEL_SCREW), &png::loosen_screws1);
        break;

    case PhasesSelftest::Kennel_wait_user_lock_tool:
        set_info_layout(_(LOCK_TOOL), &png::lock_carriage);
        break;

    case PhasesSelftest::Kennel_wait_user_tighten_top_screw:
        set_info_layout(_(TIGHTEN_TOP), &png::tighten_screw1);
        break;

    case PhasesSelftest::Kennel_measure:
        set_warning_layout(_(MEASURING));
        break;

    case PhasesSelftest::Kennel_wait_user_install_pins:
        set_info_layout(_(INSTALL_KENNEL_PINS), &png::tighten_screw3);
        break;

    case PhasesSelftest::Kennel_wait_user_tighten_bottom_screw:
        set_info_layout(_(TIGHTEN_BOT), &png::tighten_screw2);
        break;

    case PhasesSelftest::Kennel_selftest_park_test:
        set_warning_layout(_(PARKING_TEST));
        break;

    default:
        log_error(Selftest, "Unknown selftest phase %u", phase_current);
        break;
    }
}
void SelftestFrameKennel::set_name(SelftestKennels_t data) {
    static const char fmt2Translate[] = N_("Kennel %d calibration");
    size_t buff_pos = 0;
    if (data.current_kennel != std::numeric_limits<decltype(data.current_kennel)>::max()) {
        char fmt[name_buff.size()];
        _(fmt2Translate).copyToRAM(fmt, sizeof(name_buff));
        buff_pos += snprintf(name_buff.data(), name_buff.size(), fmt, data.current_kennel + 1);
    }
    if (const char *phase_name = get_phase_name(); phase_name != nullptr) {
        char phase_name_buff[50];
        _(phase_name).copyToRAM(phase_name_buff, sizeof(phase_name_buff));

        snprintf(name_buff.data() + buff_pos, name_buff.size() - buff_pos, " - %s", phase_name_buff);
    }

    SetName(string_view_utf8::MakeRAM(reinterpret_cast<const uint8_t *>(name_buff.data())));
}
void SelftestFrameKennel::set_warning_layout(string_view_utf8 txt) {
    text_info.Hide();
    icon_info.Hide();
    icon_warning.Show();
    text_warning.SetText(txt);
    text_warning.Show();
}
void SelftestFrameKennel::set_info_layout(string_view_utf8 txt, const png::Resource *res) {
    qr.Hide();
    text_warning.Hide();
    icon_warning.Hide();
    text_info.SetRect(get_info_text_rect());
    text_info.SetText(txt);
    text_info.Show();
    icon_info.SetRect(get_info_icon_rect());
    icon_info.SetAlignment(Align_t::LeftTop());
    icon_info.SetRes(res);
    icon_info.Show();
}
void SelftestFrameKennel::set_prologue() {
    text_info.SetRect(Rect16(col_texts, row_2, WizardDefaults::X_space * 1 / 2, txt_h * 6));
    text_info.SetText(_(PROLOGUE));
    icon_info.SetRect(Rect16(text_info.GetRect().TopRight(), text_info.GetRect().TopRight() + point_i16_t { (int16_t)59, (int16_t)72 }) + Rect16::Top_t(25));
    icon_info.SetRes(&png::hand_qr_59x72);
    icon_info.Show();
    qr.Show();
}
const char *SelftestFrameKennel::get_phase_name() {
    switch (phase_current) {
    case PhasesSelftest::Kennel_needs_calibration:
        return nullptr;

    case PhasesSelftest::Kennel_wait_user_park1:
    case PhasesSelftest::Kennel_wait_user_park2:
    case PhasesSelftest::Kennel_wait_user_park3:
        return N_("Parking tool");

    case PhasesSelftest::Kennel_pin_remove_prepare:
    case PhasesSelftest::Kennel_wait_user_loosen_pillar:
    case PhasesSelftest::Kennel_wait_user_remove_pins:
        return N_("Loosen screws");
    case PhasesSelftest::Kennel_wait_user_lock_tool:
        return N_("Lock tool");

    case PhasesSelftest::Kennel_wait_user_tighten_bottom_screw:
    case PhasesSelftest::Kennel_wait_user_tighten_top_screw:
        return N_("Tighten screw");

    case PhasesSelftest::Kennel_measure:
        return N_("Aligning tool");

    case PhasesSelftest::Kennel_wait_user_install_pins:
        return N_("Install pins");

    case PhasesSelftest::Kennel_selftest_park_test:
        return N_("Parking test");

    default:
        return nullptr;
    }
}
constexpr Rect16 SelftestFrameKennel::get_info_icon_rect() {
    return Rect16(col_texts + WizardDefaults::X_space * 2 / 3 - 30, row_2, WizardDefaults::X_space * 1 / 3, txt_h * 8);
};
constexpr Rect16 SelftestFrameKennel::get_info_text_rect() {
    return Rect16(col_texts, row_2, WizardDefaults::X_space * 2 / 3 - 30, txt_h * 8);
}
