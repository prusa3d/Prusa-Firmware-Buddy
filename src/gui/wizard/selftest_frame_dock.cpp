#include "selftest_frame_dock.hpp"
#include "i18n.h"
#include "img_resources.hpp"
#include <guiconfig/wizard_config.hpp>

#include <algorithm>

LOG_COMPONENT_REF(Selftest);
static constexpr size_t col_texts = WizardDefaults::MarginLeft;
static constexpr size_t txt_h = WizardDefaults::txt_h;
static constexpr size_t row_2 = WizardDefaults::row_1 + WizardDefaults::progress_row_h;
static constexpr size_t row_3 = row_2 + txt_h;
static constexpr size_t row_4 = row_3 + txt_h;
static constexpr size_t row_5 = row_4 + txt_h;
static constexpr size_t row_6 = row_5 + txt_h;
static constexpr size_t row_7 = row_6 + txt_h;
static constexpr size_t row_8 = row_7 + txt_h;
static constexpr size_t row_9 = row_8 + txt_h;

SelftestFrameDock::SelftestFrameDock(window_t *parent, PhasesSelftest ph, fsm::PhaseData data)
    : AddSuperWindow<SelftestFrameNamedWithRadio>(parent, ph, data, _("Dock Calibration"), 1)
    , footer(this, 0, footer::Item::nozzle, footer::Item::bed, footer::Item::axis_z) // ItemAxisZ to show Z coord while moving up
    , progress(this, WizardDefaults::row_1)
    , text_info(this, get_info_text_rect(), is_multiline::yes)
    , text_estimate(this, get_estimate_text_rect(), is_multiline::no)
    , icon_warning(this, &img::printer_is_moving, point_i16(col_texts, row_4))
    , text_warning(this, Rect16(col_texts + img::warning_48x48.w + 20, row_4, WizardDefaults::X_space - img::warning_48x48.w - 20, 3 * txt_h), is_multiline::yes)
    , icon_info(this, &img::parking1, text_info.GetRect().TopRight())
    , qr(this, get_info_icon_rect() + Rect16::Left_t(25), LINK)
    , text_link(this, get_link_text_rect(), is_multiline::no) {
    qr.Hide();
    text_link.Hide();
    icon_warning.Hide();
    icon_info.Hide();
    text_warning.Hide();
    change();
}

void SelftestFrameDock::change() {
    const SelftestDocks_t dock_data(data_current);
    set_name(dock_data);
    set_remaining();
    invalidate();

    switch (phase_current) {
    case PhasesSelftest::Dock_needs_calibration:
        set_prologue();
        break;

    case PhasesSelftest::Dock_move_away:
        set_warning_layout(_(MOVE_AWAY));
        break;

    case PhasesSelftest::Dock_wait_user_park1:
        set_info_layout(_(PARK1), &img::parking1);
        break;

    case PhasesSelftest::Dock_wait_user_park2:
        set_info_layout(_(PARK2), &img::parking2);
        break;

    case PhasesSelftest::Dock_wait_user_park3:
        set_info_layout(_(PARK3), &img::parking2);
        break;

    case PhasesSelftest::Dock_wait_user_remove_pins:
        set_info_layout(_(REMOVE_DOCK_PINS), &img::loosen_screws2);
        break;

    case PhasesSelftest::Dock_wait_user_loosen_pillar:
        set_info_layout(_(LOOSEN_DOCK_SCREW), &img::loosen_screws1);
        break;

    case PhasesSelftest::Dock_wait_user_lock_tool:
        set_info_layout(_(LOCK_TOOL), &img::lock_carriage);
        break;

    case PhasesSelftest::Dock_wait_user_tighten_top_screw:
        set_info_layout(_(TIGHTEN_TOP), &img::tighten_screw1);
        break;

    case PhasesSelftest::Dock_measure:
        set_warning_layout(_(MEASURING));
        break;

    case PhasesSelftest::Dock_wait_user_tighten_bottom_screw:
        set_info_layout(_(TIGHTEN_BOT), &img::tighten_screw2);
        break;

    case PhasesSelftest::Dock_wait_user_install_pins:
        set_info_layout(_(INSTALL_DOCK_PINS), &img::tighten_screw3);
        break;

    case PhasesSelftest::Dock_selftest_park_test:
        set_warning_layout(_(PARKING_TEST));
        break;

    case PhasesSelftest::Dock_selftest_failed:
        set_info_layout(_(TEST_FAILED), &img::error_white_48x48);
        break;

    case PhasesSelftest::Dock_calibration_success:
        text_info.Hide();
        icon_info.Hide();
        icon_warning.Hide();
        text_warning.SetText(_(TEST_SUCCESS));
        text_warning.Show();
        break;

    default:
        log_error(Selftest, "Unknown selftest phase %u", static_cast<unsigned>(phase_current));
        break;
    }
}

void SelftestFrameDock::set_name(SelftestDocks_t data) {
    static const char fmt2Translate[] = N_("Dock %d calibration");
    size_t buff_pos = 0;
    if (data.current_dock != std::numeric_limits<decltype(data.current_dock)>::max()) {
        char fmt[std::tuple_size_v<decltype(name_buff)>];
        _(fmt2Translate).copyToRAM(fmt, sizeof(name_buff));
        buff_pos += snprintf(name_buff.data(), name_buff.size(), fmt, data.current_dock + 1);
        buff_pos = std::min(buff_pos, name_buff.size());
    }
    if (const char *phase_name = get_phase_name(); phase_name != nullptr) {
        char phase_name_buff[50];
        _(phase_name).copyToRAM(phase_name_buff, sizeof(phase_name_buff));

        snprintf(name_buff.data() + buff_pos, name_buff.size() - buff_pos, " - %s", phase_name_buff);
    }

    SetName(string_view_utf8::MakeRAM(reinterpret_cast<const uint8_t *>(name_buff.data())));
}

void SelftestFrameDock::set_remaining() {
    // Get translated message into a standard char array as the string_view_utf8 dows not allow direct access to underlying memory.
    char temp_remaining_buff[50];
    _(REMAINING).copyToRAM(temp_remaining_buff, std::size(temp_remaining_buff));

    // Format the resulting string, build a string view on top of the static memory, and set the text of the gui element
    snprintf(remaining_buff.data(), std::size(remaining_buff), temp_remaining_buff, get_phase_remaining_minutes());
    text_estimate.SetText(string_view_utf8::MakeRAM(reinterpret_cast<const uint8_t *>(remaining_buff.data())));
}

void SelftestFrameDock::set_warning_layout(string_view_utf8 txt) {
    qr.Hide();
    text_link.Hide();
    text_info.Hide();
    icon_info.Hide();
    text_estimate.Hide();
    icon_warning.Show();
    text_warning.SetText(txt);
    text_warning.Show();
}

void SelftestFrameDock::set_info_layout(string_view_utf8 txt, const img::Resource *res) {
    qr.Hide();
    text_link.Hide();
    text_warning.Hide();
    icon_warning.Hide();
    text_info.SetRect(get_info_text_rect());
    text_info.SetText(txt);
    text_info.Show();
    icon_info.SetRect(get_info_icon_rect());
    icon_info.SetAlignment(Align_t::LeftTop());
    icon_info.SetRes(res);
    icon_info.Show();
    text_estimate.Show();
}

void SelftestFrameDock::set_prologue() {
    text_info.SetRect(Rect16(col_texts, row_2, WizardDefaults::X_space * 1 / 2, txt_h * 6));
    text_info.SetText(_(PROLOGUE));
    icon_info.SetRect(Rect16(text_info.GetRect().TopRight(), text_info.GetRect().TopRight() + point_i16_t { (int16_t)59, (int16_t)72 }) + Rect16::Top_t(25));
    icon_info.SetRes(&img::hand_qr_59x72);
    icon_info.Show();
    qr.Show();
    text_link.SetText(_(LINK));
    text_link.SetAlignment(Align_t::Right());
    text_link.Show();
    text_estimate.Show();
}

const char *SelftestFrameDock::get_phase_name() {
    switch (phase_current) {
    case PhasesSelftest::Dock_needs_calibration:
        return nullptr;

    case PhasesSelftest::Dock_move_away:
        return nullptr;

    case PhasesSelftest::Dock_wait_user_park1:
    case PhasesSelftest::Dock_wait_user_park2:
    case PhasesSelftest::Dock_wait_user_park3:
        return N_("Parking tool");

    case PhasesSelftest::Dock_wait_user_loosen_pillar:
        return N_("Loosen screws");
    case PhasesSelftest::Dock_wait_user_remove_pins:
        return N_("Loosen pins");
    case PhasesSelftest::Dock_wait_user_lock_tool:
        return N_("Lock the tool");

    case PhasesSelftest::Dock_wait_user_tighten_bottom_screw:
    case PhasesSelftest::Dock_wait_user_tighten_top_screw:
        return N_("Tighten screw");

    case PhasesSelftest::Dock_measure:
        return N_("Aligning tool");

    case PhasesSelftest::Dock_wait_user_install_pins:
        return N_("Install pins");

    case PhasesSelftest::Dock_selftest_park_test:
    case PhasesSelftest::Dock_selftest_failed:
        return N_("Parking test");

    case PhasesSelftest::Dock_calibration_success:
        return N_("End of test");

    default:
        return nullptr;
    }
}

int SelftestFrameDock::get_phase_remaining_minutes() {
    switch (phase_current) {
    case PhasesSelftest::Dock_needs_calibration:
    case PhasesSelftest::Dock_move_away:
    case PhasesSelftest::Dock_wait_user_park1:
    case PhasesSelftest::Dock_wait_user_park2:
    case PhasesSelftest::Dock_wait_user_park3:
        return 3;
    case PhasesSelftest::Dock_wait_user_remove_pins:
    case PhasesSelftest::Dock_wait_user_loosen_pillar:
    case PhasesSelftest::Dock_wait_user_lock_tool:
    case PhasesSelftest::Dock_wait_user_tighten_top_screw:
        return 2;
    case PhasesSelftest::Dock_measure:
    case PhasesSelftest::Dock_wait_user_tighten_bottom_screw:
    case PhasesSelftest::Dock_wait_user_install_pins:
        return 1;
    case PhasesSelftest::Dock_selftest_park_test:
    case PhasesSelftest::Dock_selftest_failed:
    case PhasesSelftest::Dock_calibration_success:
    default:
        return 0;
    }
}

constexpr Rect16 SelftestFrameDock::get_info_icon_rect() {
    return Rect16(col_texts + WizardDefaults::X_space * 2 / 3 - 30, row_2, WizardDefaults::X_space * 1 / 3, txt_h * 8);
};

constexpr Rect16 SelftestFrameDock::get_info_text_rect() {
    return Rect16(col_texts, row_2, WizardDefaults::X_space * 2 / 3 - 30, txt_h * 7);
}

constexpr Rect16 SelftestFrameDock::get_estimate_text_rect() {
    return Rect16(col_texts, row_9, 7 * WizardDefaults::X_space / 16, txt_h);
}

constexpr Rect16 SelftestFrameDock::get_link_text_rect() {
    return Rect16(get_estimate_text_rect().Right(), row_2 + txt_h * 7, GuiDefaults::ScreenWidth - WizardDefaults::MarginRight, txt_h);
}
