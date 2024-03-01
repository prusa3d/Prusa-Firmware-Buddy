#include "selftest_frame_phase_stepping.hpp"
#include "frame_qr_layout.hpp"
#include "i18n.h"
#include "str_utils.hpp"
#include <guiconfig/wizard_config.hpp>
#include "selftest_phase_stepping_result.hpp"
#include "img_resources.hpp"

namespace {
#if PRINTER_IS_PRUSA_XL
constexpr const char *QR_ADDR = "prusa.io/xl-phstep-qr";
constexpr const char *ADDR_IN_TEXT = "prusa.io/xl-phstep";
#else
    #error
#endif
constexpr const char *txt_picking_tool { N_("Picking Tool") };
constexpr const char *txt_calibrating_x { N_("Calibrating X") };
constexpr const char *txt_calibrating_y { N_("Calibrating Y") };
constexpr const char *txt_calibration_nok { N_("Calibration of axis %c failed.\nParameter 1: forward %3d%%, backward %3d%%\nParameter 2: forward %3d%%, backward %3d%%") };
constexpr const char *txt_calibration_error { N_("Calibration failed with error.") };
constexpr const char *txt_enabling { N_("Finishing") };
constexpr const char *txt_calibration_ok { N_("Axis X vibration A reduced by %2d%%\nAxis X vibration B reduced by %2d%%\nAxis Y vibration A reduced by %2d%%\nAxis Y vibration B reduced by %2d%%") };

string_view_utf8 fmt_calibration_nok(char axis, auto &text_buffer, const fsm::PhaseData &data) {
    std::array<char, 150> fmt;
    _(txt_calibration_nok).copyToRAM(fmt.data(), fmt.size());
    snprintf(text_buffer.data(), text_buffer.size(), fmt.data(), axis, data[0], data[1], data[2], data[3]);
    return string_view_utf8::MakeRAM((const uint8_t *)text_buffer.data());
}

string_view_utf8 fmt_calibration_ok(auto &text_buffer, const fsm::PhaseData &data) {
    std::array<char, 150> fmt;
    _(txt_calibration_ok).copyToRAM(fmt.data(), fmt.size());
    snprintf(text_buffer.data(), text_buffer.size(), fmt.data(), data[0], data[1], data[2], data[3]);
    return string_view_utf8::MakeRAM((const uint8_t *)text_buffer.data());
}

} // namespace

SelftestFramePhaseStepping::SelftestFramePhaseStepping(window_t *parent, PhasesSelftest ph, fsm::PhaseData data)
    : AddSuperWindow<SelftestFrameWithRadio>(parent, ph, data, 0)
    , text(this, FrameQRLayout::text_rect(), is_multiline::yes)
    , link(this, FrameQRLayout::link_rect(), is_multiline::no)
    , icon_phone(this, FrameQRLayout::phone_icon_rect(), &img::hand_qr_59x72)
    , qr(this, FrameQRLayout::qrcode_rect(), QR_ADDR) {
    link.SetText(string_view_utf8::MakeCPUFLASH(reinterpret_cast<const uint8_t *>(ADDR_IN_TEXT)));
    text.SetText(_("To learn more about the phase stepping calibration process, read the article:"));
    change();
}

void SelftestFramePhaseStepping::change() {
    switch (phase_current) {
    case PhasesSelftest::PhaseStepping_intro:
        break;
    case PhasesSelftest::PhaseStepping_pick_tool:
        flip_layout();
        text.SetText(_(txt_picking_tool));
        break;
    case PhasesSelftest::PhaseStepping_calib_x:
        flip_layout();
        text.SetText(_(txt_calibrating_x));
        break;
    case PhasesSelftest::PhaseStepping_calib_y:
        flip_layout();
        text.SetText(_(txt_calibrating_y));
        break;
    case PhasesSelftest::PhaseStepping_calib_x_nok:
        flip_layout();
        text.SetText(fmt_calibration_nok('X', text_buffer, data_current));
        break;
    case PhasesSelftest::PhaseStepping_calib_y_nok:
        flip_layout();
        text.SetText(fmt_calibration_nok('Y', text_buffer, data_current));
        break;
    case PhasesSelftest::PhaseStepping_calib_error:
        flip_layout();
        text.SetText(_(txt_calibration_error));
        break;
    case PhasesSelftest::PhaseStepping_calib_ok:
        flip_layout();
        text.SetText(fmt_calibration_ok(text_buffer, data_current));
        break;
    case PhasesSelftest::PhaseStepping_enabling:
        flip_layout();
        text.SetText(_(txt_enabling));
        break;
    default:
        break;
    }
}

// TODO: This is quite hackish way to do this but oh well.
//       Proper way would be to have actual frames with the individual GUI elements.
//       This will be changed eventually after we add progress bars and what not.
void SelftestFramePhaseStepping::flip_layout() {
    icon_phone.Hide();
    qr.Hide();
    link.Hide();
    text.SetRect({ 0, GuiDefaults::HeaderHeight, GuiDefaults::ScreenWidth, GuiDefaults::BodyHeight - GuiDefaults::HeaderHeight });
    text.SetAlignment(Align_t::Center());
}
