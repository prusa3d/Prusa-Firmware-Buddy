#include "screen_manual_belt_tuning.hpp"
#include <feature/manual_belt_tuning/manual_belt_tuning_config.hpp>

#include <marlin_server_types/fsm/manual_belt_tuning_phases.hpp>
#include <img_resources.hpp>
#include <guiconfig/wizard_config.hpp>
#include <window_numb.hpp>
#include <client_response.hpp>
#include <qr.hpp>
#include <meta_utils.hpp>
#include <standard_frame/frame_text_prompt.hpp>
#include <standard_frame/frame_wait.hpp>

using Phase = PhaseManualBeltTuning;

namespace {

// Introduction to belt tuning calibration, QR, prerequisites PHASE: intro
constexpr const char link_begin_calib[] = "prusa.io/core-belt-calibration";
constexpr const char *txt_title_begin = N_("Let's calibrate your belt tension");
constexpr const char *txt_desc_begin = N_("Before we begin, scan the QR code for the guide. You'll also need an Allen key.");
// Checking if X-axis gantry is correctly lined up PHASE: check_x_gantry
constexpr const char link_belt_calib_gantry[] = "prusa.io/core-belt-calibration";
constexpr const char *txt_title_gantry = N_("Check the gantry squareness");
#if HAS_INDX()
constexpr const char *txt_desc_gantry = N_("Move printhead to the back. Check there's no gap between gantry and tensioner on both ends. If there is, follow the guide.");
#else
constexpr const char *txt_desc_gantry = N_("Move printhead to the front. Check there's no gap between gantry and tensioner on both ends. If there is, follow the guide.");
#endif
// Waiting for homing and moving to wizard position
constexpr auto txt_wait = N_("Printer is homing, please wait.");
// Prepare for measuring actual frequencies, QR, prerequisites PHASE: intro_measure
constexpr const char *txt_title_measure = N_("Measuring actual belt frequency");
constexpr const char *txt_desc_measure = N_("The upper belt will be measured first, then the lower. Use the knob to adjust resonance. Correct frequency appears as slow belt motion with sharp peaks.");
// Measuring actual upper belt frequency, Knob PHASE: measure_upper_belt
constexpr const char *txt_title_up_belt_freq = N_("Upper belt actual frequency");
constexpr const char *txt_desc_up_belt_freq = N_("Turn the knob to adjust frequency. Look for slow belt movement with sharp, regular peaks, then click the knob to proceed.");
// Measuring actual lower belt frequency, Knob PHASE: measure_lower_belt
constexpr const char *txt_title_lo_belt_freq = N_("Lower belt actual frequency");
constexpr const char *txt_desc_lo_belt_freq = N_("Turn the knob to adjust frequency. Look for slow belt movement with sharp, regular peaks, then click the knob to proceed.");
// Show results of both belt measurements in Newtons PHASE: show_tension
constexpr const char *txt_title_freq_report = N_("Actual belts frequencies");
constexpr const char *txt_desc_freq_report = N_("Upper belt: %.1f Hz\nLower belt: %.1f Hz\n\nOptimal tension is between %u and %u Hz\n\nReady to calculate screw adjustment.\nPress Continue to proceed.");
// Warning about x-gantry alignment issue PHASE: alignment_issue
constexpr const char *txt_title_alignment_issue = N_("Alignment issue detected");
constexpr const char *txt_desc_alignment_issue = N_("Measured belt frequencies are out of the expected range.\nPlease check that the gantry is aligned properly and repeat the calibration.");
// User adjusts the tensioners with calculated allen key turns PHASE: adjust_tensioners
constexpr const char link_tensioning[] = "prusa.io/core-belt-calibration-tensioning";
constexpr const char *txt_tighten = N_("tighten");
constexpr const char *txt_loosen = N_("loosen");
constexpr const char *txt_title_turn_screw = N_("Adjust belt tensioners");
constexpr const char *txt_desc_turn_screw = N_("Turn both screws:\n%s %s turns\n\nPress Continue when done.");
constexpr const char *txt_desc_no_turn = N_("No need to turn the tensioner screws.\n\nPress Continue to finish calibration or Adjust to repeat measuring.");
// Finish screen
constexpr auto txt_desc_finished = N_("Belt tension has been successfully calibrated.");

constexpr uint8_t qr_size = 100;

constexpr Rect16 rect_title = Rect16(WizardDefaults::MarginLeft, WizardDefaults::row_0, GuiDefaults::ScreenWidth - WizardDefaults::MarginLeft - WizardDefaults::MarginRight, WizardDefaults::txt_h);
constexpr Rect16 rect_line = Rect16(WizardDefaults::MarginLeft, WizardDefaults::row_1, GuiDefaults::ScreenWidth - WizardDefaults::MarginLeft - WizardDefaults::MarginRight, 1);

constexpr Rect16 rect_qr = Rect16(GuiDefaults::ScreenWidth - WizardDefaults::MarginRight - qr_size, WizardDefaults::row_1 + 10 /*=visual delimeter*/, qr_size, qr_size);
constexpr Rect16 rect_desc = Rect16(WizardDefaults::MarginLeft, WizardDefaults::row_1 + 10 /*=visual delimeter*/, GuiDefaults::ScreenWidth - WizardDefaults::MarginLeft - WizardDefaults::MarginRight, WizardDefaults::Y_space - WizardDefaults::RectRadioButton(0).Height() - WizardDefaults::row_h - 30 /*=visual space*/);
constexpr Rect16 rect_desc_qr = Rect16(WizardDefaults::MarginLeft, WizardDefaults::row_1 + 10 /*=visual delimeter*/, GuiDefaults::ScreenWidth - WizardDefaults::MarginLeft - WizardDefaults::MarginRight - qr_size - 5 /*=padding*/, WizardDefaults::Y_space - WizardDefaults::RectRadioButton(0).Height() - WizardDefaults::row_h - 70 /*=space for link*/);

constexpr Rect16 rect_scan_me = Rect16(rect_qr.Left(), rect_qr.Bottom(), qr_size, WizardDefaults::txt_h);
constexpr Rect16 rect_details = Rect16(WizardDefaults::MarginLeft, rect_desc_qr.Bottom(), GuiDefaults::ScreenWidth, WizardDefaults::txt_h);
constexpr Rect16 rect_link = Rect16(WizardDefaults::MarginLeft, rect_details.Bottom(), GuiDefaults::ScreenWidth, WizardDefaults::txt_h);
constexpr Rect16 rect_desc_knob = Rect16(WizardDefaults::MarginLeft, WizardDefaults::row_1 + 10, GuiDefaults::ScreenWidth - WizardDefaults::MarginLeft - WizardDefaults::MarginRight, WizardDefaults::Y_space - WizardDefaults::RectRadioButton(0).Height() - WizardDefaults::row_h - 80 /*=visual space*/);

constexpr Rect16 rect_numb = Rect16(GuiDefaults::ScreenWidth / 2 - 50, WizardDefaults::RectRadioButton(0).Top() - 100, 100, 22);
constexpr Rect16 rect_knob = Rect16(GuiDefaults::ScreenWidth / 2 - 41, WizardDefaults::RectRadioButton(0).Top() - 70, 81, 55);
constexpr Rect16 rect_minus = Rect16(GuiDefaults::ScreenWidth / 2 - 61, WizardDefaults::RectRadioButton(0).Top() - 70, 20, 55);
constexpr Rect16 rect_plus = Rect16(GuiDefaults::ScreenWidth / 2 + 40, WizardDefaults::RectRadioButton(0).Top() - 70, 20, 55);
} // namespace

namespace frames {

class FrameTitle {
public:
    FrameTitle(window_frame_t *parent, const char *txt_title)
        : line(parent, rect_line)
        , title(parent, rect_title, is_multiline::no, is_closed_on_click_t::no, _(txt_title)) {
        line.SetBackColor(COLOR_DARK_GRAY);
        title.set_font(Font::big);
        title.SetTextColor(COLOR_BRAND);
    }

protected:
    BasicWindow line;
    window_text_t title;
};

class FrameTitleRadio : public FrameTitle {

public:
    FrameTitleRadio(window_frame_t *parent, Phase phase, const char *txt_title)
        : FrameTitle(parent, txt_title)
        , radio(parent, WizardDefaults::RectRadioButton(0), phase) {
        parent->CaptureNormalWindow(radio);
    }

protected:
    RadioButtonFSM radio;
};

class FrameTitleDescRadio : public FrameTitleRadio {

public:
    FrameTitleDescRadio(window_frame_t *parent, Phase phase, const char *title, const char *desc)
        : FrameTitleRadio(parent, phase, title)
        , phase(phase)
        , desc_ptr(desc)
        , desc(parent, rect_desc, is_multiline::yes) {}

    void update(fsm::PhaseData data) {
        if (phase == PhaseManualBeltTuning::show_tension) {
            const auto tensions = fsm::deserialize_data<belt_tensions>(data);
            desc.SetText(_(desc_ptr).formatted(params, tensions.get_upper(), tensions.get_lower(), manual_belt_tuning::freq_result_min, manual_belt_tuning::freq_result_max));
        } else {
            desc.SetText(_(desc_ptr));
        }
    }

protected:
    Phase phase;
    const char *desc_ptr;
    window_text_t desc;
    StringViewUtf8Parameters<30> params;
};

class FrameTitleDescRadioQR : public FrameTitleRadio {

public:
    FrameTitleDescRadioQR(window_frame_t *parent, Phase phase, const char *title, const char *desc, const char *qr_link)
        : FrameTitleRadio(parent, phase, title)
        , phase(phase)
        , desc_ptr(desc)
        , qr(parent, rect_qr, Align_t::Center(), qr_link)
        , scan_me(parent, rect_scan_me, is_multiline::no, is_closed_on_click_t::no, _("Scan me"))
        , details(parent, rect_details, is_multiline::no, is_closed_on_click_t::no, _("More details at"))
        , link(parent, rect_link, is_multiline::no, is_closed_on_click_t::no, string_view_utf8::MakeRAM(qr_link))
        , desc(parent, rect_desc_qr, is_multiline::yes) {
        qr.SetAlignment(Align_t::RightTop());
        details.set_font(Font::small);
        link.set_font(Font::small);
        scan_me.set_font(Font::small);
        scan_me.SetAlignment(Align_t::Center());
    }

    void update(fsm::PhaseData data) {
        if (phase == PhaseManualBeltTuning::adjust_tensioners) {

            auto revs = fsm::deserialize_data<screw_revs>(data);

            if (revs.turn_eights == 0) {
                desc.SetText(_(txt_desc_no_turn));
                return;
            }

            StringBuilder sb1(buffer_tighten_loosen);
            if (revs.turn_eights > 0) {
                sb1.append_string_view(_(txt_tighten));
            } else {
                sb1.append_string_view(_(txt_loosen));
            }

            StringBuilder sb2(buffer_eights);
            auto revs_abs = abs(revs.turn_eights);
            if (revs_abs >= 8) {
                const auto turns = revs_abs / 8;
                revs_abs %= 8;
                sb2.append_printf("%d & ", turns);
            }
            sb2.append_printf("%d/8", revs_abs);

            desc.SetText(_(desc_ptr).formatted(params, buffer_tighten_loosen.data(), buffer_eights.data()));

        } else {
            desc.SetText(_(desc_ptr));
        }
    }

protected:
    Phase phase;
    const char *desc_ptr;
    QRStaticStringWindow qr;
    window_text_t scan_me;
    window_text_t details;
    window_text_t link;
    window_text_t desc;
    std::array<char, 40> buffer_tighten_loosen = {};
    std::array<char, 40> buffer_eights = {};
    StringViewUtf8Parameters<120> params;
};

class FrameAdjustKnob : public FrameTitle {

    class DoneResponder : public window_t {

    public:
        DoneResponder(window_t *parent, Phase phase)
            : window_t(parent, Rect16 {})
            , phase_(phase) {
        }

    protected:
        const Phase phase_;

        virtual void windowEvent([[maybe_unused]] window_t *sender, GUI_event_t event, [[maybe_unused]] void *param) override {
            switch (event) {
            case GUI_event_t::CLICK:
                marlin_client::FSM_response(phase_, Response::Done);
                break;
            default:
                break;
            }
        }
    };

public:
    FrameAdjustKnob(window_frame_t *parent, Phase phase, const char *title, const char *desc)
        : FrameTitle(parent, title)
        , phase(phase)
        , desc(parent, rect_desc_knob, is_multiline::yes, is_closed_on_click_t::no, _(desc))
        , numb(parent, rect_numb, 0, "%0.1f Hz")
        , knob(parent, rect_knob, &img::turn_knob_81x55)
        , plus(parent, rect_plus, is_multiline::no, is_closed_on_click_t::no, string_view_utf8::MakeRAM("+"))
        , minus(parent, rect_minus, is_multiline::no, is_closed_on_click_t::no, string_view_utf8::MakeRAM("-"))
        , done(parent, phase) {
        plus.SetAlignment(Align_t::Center());
        plus.set_font(Font::big);
        plus.SetTextColor(COLOR_BRAND);
        minus.SetAlignment(Align_t::Center());
        minus.set_font(Font::big);
        minus.SetTextColor(COLOR_BRAND);
        numb.SetAlignment(Align_t::Center());

        static_cast<window_frame_t *>(parent)->CaptureNormalWindow(done);
    }

    void update(fsm::PhaseData data) {
        const auto tensions = fsm::deserialize_data<belt_tensions>(data);
        if (phase == PhaseManualBeltTuning::measure_upper_belt) {
            numb.SetValue(tensions.get_upper());
        } else if (phase == PhaseManualBeltTuning::measure_lower_belt) {
            numb.SetValue(tensions.get_lower());
        }
    }

private:
    Phase phase;
    window_text_t desc;
    window_numb_t numb;
    window_icon_t knob;
    window_text_t plus;
    window_text_t minus;
    DoneResponder done;
};

} // namespace frames

namespace {

using Frames = FrameDefinitionList<ScreenManualBeltTuning::FrameStorage,
    FrameDefinition<Phase::intro, frames::FrameTitleDescRadioQR, Phase::intro, txt_title_begin, txt_desc_begin, link_begin_calib>,
    FrameDefinition<Phase::check_x_gantry, frames::FrameTitleDescRadioQR, Phase::check_x_gantry, txt_title_gantry, txt_desc_gantry, link_belt_calib_gantry>,
    FrameDefinition<Phase::homing_wait, FrameWait, txt_wait>,
    FrameDefinition<Phase::intro_measure, frames::FrameTitleDescRadioQR, Phase::intro_measure, txt_title_measure, txt_desc_measure, link_begin_calib>,
    FrameDefinition<Phase::measure_upper_belt, frames::FrameAdjustKnob, Phase::measure_upper_belt, txt_title_up_belt_freq, txt_desc_up_belt_freq>,
    FrameDefinition<Phase::measure_lower_belt, frames::FrameAdjustKnob, Phase::measure_lower_belt, txt_title_lo_belt_freq, txt_desc_lo_belt_freq>,
    FrameDefinition<Phase::show_tension, frames::FrameTitleDescRadio, Phase::show_tension, txt_title_freq_report, txt_desc_freq_report>,
    FrameDefinition<Phase::alignment_issue, frames::FrameTitleDescRadioQR, Phase::alignment_issue, txt_title_alignment_issue, txt_desc_alignment_issue, link_belt_calib_gantry>,
    FrameDefinition<Phase::adjust_tensioners, frames::FrameTitleDescRadioQR, Phase::adjust_tensioners, txt_title_turn_screw, txt_desc_turn_screw, link_tensioning>,
    FrameDefinition<Phase::finished, FrameTextPrompt, Phase::finished, txt_desc_finished>>;

} // namespace

ScreenManualBeltTuning::ScreenManualBeltTuning()
    : ScreenFSM("BELT TUNING", GuiDefaults::RectScreenNoHeader) {
    header.SetIcon(&img::selftest_16x16);
    create_frame();
}

ScreenManualBeltTuning::~ScreenManualBeltTuning() {
    destroy_frame();
}

void ScreenManualBeltTuning::create_frame() {
    Frames::create_frame(frame_storage, get_phase(), &inner_frame);
}

void ScreenManualBeltTuning::destroy_frame() {
    Frames::destroy_frame(frame_storage, get_phase());
}

void ScreenManualBeltTuning::update_frame() {
    Frames::update_frame(frame_storage, get_phase(), fsm_base_data.GetData());
}
