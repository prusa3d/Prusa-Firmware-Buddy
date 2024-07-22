#include "screen_input_shaper_calibration.hpp"

#include "window_wizard_progress.hpp"
#include <gui/frame_qr_layout.hpp>
#include <img_resources.hpp>
#include <str_utils.hpp>

static ScreenInputShaperCalibration *instance = nullptr;

static const char *text_header = N_("INPUT SHAPER CALIBRATION");

static constexpr auto rect_screen = WizardDefaults::RectSelftestFrame;
static constexpr auto rect_radio = WizardDefaults::RectRadioButton(0);
static constexpr auto rect_frame = Rect16 {
    rect_screen.Left() + WizardDefaults::MarginLeft,
    rect_screen.Top() + WizardDefaults::row_0,
    rect_screen.Width() - (WizardDefaults::MarginLeft + WizardDefaults::MarginRight),
    rect_radio.Top() - rect_screen.Top() - WizardDefaults::row_0
};
static constexpr auto rect_frame_top = Rect16 {
    rect_frame.Left(),
    rect_frame.Top(),
    rect_frame.Width(),
    WizardDefaults::row_h,
};
static constexpr auto rect_frame_bottom = Rect16 {
    rect_frame.Left(),
    rect_frame.Top() + WizardDefaults::progress_row_h + WizardDefaults::row_h,
    rect_frame.Width(),
    WizardDefaults::row_h,
};
static constexpr auto center_frame_bottom = point_i16_t {
    rect_frame_bottom.Left() + rect_frame_bottom.Width() / 2,
    rect_frame_bottom.Top() + rect_frame_bottom.Height() / 2,
};
static constexpr auto progress_top = Rect16::Top_t { 100 };

class FrameInstructions {
private:
    window_text_t text;

protected:
    FrameInstructions(window_t *parent, const string_view_utf8 &txt)
        : text(parent, rect_frame, is_multiline::yes, is_closed_on_click_t::no, txt) {
    }

public:
    void update(fsm::PhaseData) {}
};

class FrameMeasurement {
private:
    window_text_t text_above;
    window_text_t text_below;
    string_view_utf8 text_above_axis;
    window_wizard_progress_t progress;
    std::array<char, sizeof("255 Hz")> text_below_buffer;

protected:
    FrameMeasurement(window_t *parent, const string_view_utf8 &txt)
        : text_above(parent, rect_frame_top, is_multiline::no, is_closed_on_click_t::no, _("Calibrating accelerometer..."))
        , text_below(parent, rect_frame_bottom, is_multiline::no, is_closed_on_click_t::no)
        , text_above_axis(txt)
        , progress(parent, progress_top) {
        text_above.SetAlignment(Align_t::CenterTop());
        text_below.SetAlignment(Align_t::CenterTop());
    }

public:
    void update(fsm::PhaseData data) {
        if (data[3]) {
            progress.SetProgressPercent(data[2] / 2.55);
        } else {
            progress.SetProgressPercent(100 * float(data[2] - data[0]) / (data[1] - data[0]));
            snprintf(text_below_buffer.data(), text_below_buffer.size(), "%3d Hz", data[2]);
            text_above.SetText(text_above_axis);
            text_below.SetText(string_view_utf8::MakeRAM(text_below_buffer.data()));
            text_below.Invalidate();
        }
    }
};

class FrameInfo {
private:
    window_text_t text;
    window_text_t link;
    window_icon_t icon_phone;
    window_qr_t qr;

public:
    explicit FrameInfo(window_t *parent)
        : text(parent, FrameQRLayout::text_rect(), is_multiline::yes, is_closed_on_click_t::no, _(text_info))
        , link(parent, FrameQRLayout::link_rect(), is_multiline::no, is_closed_on_click_t::no)
        , icon_phone(parent, FrameQRLayout::phone_icon_rect(), &img::hand_qr_59x72)
        , qr(parent, FrameQRLayout::qrcode_rect()) {
        text.SetAlignment(Align_t::LeftCenter());
        StringBuilder(link_buffer)
            .append_string("prusa.io/")
            .append_string(PrinterModelInfo::current().help_url)
            .append_string("-iscal");
        link.SetText(_(link_buffer.data()));
        StringBuilder(qr_buffer)
            .append_string("prusa.io/qr-")
            .append_string(PrinterModelInfo::current().help_url)
            .append_string("-iscal");
        qr.SetText(qr_buffer.data());
    }

    void update(fsm::PhaseData) {}

private:
    static constexpr const char *text_info = N_(
        "Attention: Ensure the accelerometer is properly connected. Follow the step-by-step guide in the link below:");
    std::array<char, 32> link_buffer;
    std::array<char, 32> qr_buffer;
};

class FrameParking final {
private:
    window_text_t text;
    window_icon_hourglass_t spinner;

public:
    explicit FrameParking(window_t *parent)
        : text(parent, rect_frame_top, is_multiline::yes, is_closed_on_click_t::no, _(text_parking))
        , spinner(parent, center_frame_bottom) {
        spinner.SetRect(spinner.GetRect() - Rect16::Left_t(spinner.GetRect().Width() / 2));
        text.SetAlignment(Align_t::Center());
    }

    void update(fsm::PhaseData) {}

    static constexpr const char *text_parking = N_("Parking");
};

class FrameConnectToBoard final : public FrameInstructions {
public:
    explicit FrameConnectToBoard(window_t *parent)
        : FrameInstructions(parent, _(text_connect_to_board)) {}

    static constexpr const char *text_connect_to_board = N_(
        "Accelerometer is not responding. "
        "Turn off the printer and make sure the accelerometer cable is connected to the main board. "
        "You can also abort the input shaper calibration and continue using the printer with default settings.");
};

class FrameWaitForExtruderTemperature final {
    window_text_t text_above;
    WindowBlinkingText text_below;
    std::array<char, sizeof("NNN°C")> text_below_buffer;

public:
    explicit FrameWaitForExtruderTemperature(window_t *parent)
        : text_above { parent, rect_frame_top, is_multiline::no, is_closed_on_click_t::no, _(text_wait_for_extruder_temperature) }
        , text_below { parent, rect_frame_bottom } {
        text_above.SetAlignment(Align_t::CenterTop());
        text_below.SetAlignment(Align_t::CenterTop());
        text_below.SetBlinkColor(COLOR_AZURE);
    }

    static constexpr const char *text_wait_for_extruder_temperature = N_(
        "Waiting for nozzle to cool down");

    void update(fsm::PhaseData data) {
        const auto current_temperature = ((data[0] << 8) | data[1]) % 1000;

        snprintf(text_below_buffer.data(), text_below_buffer.size(), "%3d°C", current_temperature);
        text_below.SetText(string_view_utf8::MakeRAM(text_below_buffer.data()));
        text_below.Invalidate();
    }
};

class FrameAttachToExtruder final : public FrameInstructions {
public:
    explicit FrameAttachToExtruder(window_t *parent)
        : FrameInstructions(parent, _(text_attach_to_extruder)) {}

    static constexpr const char *text_attach_to_extruder = N_(
        "Firmly attach the accelerometer to the extruder "
        "(remove silicone sock if necessary). "
        "In the next step, extruder will start vibrating and resonance will be measured.");
};

class FrameAttachToBed final : public FrameInstructions {
public:
    explicit FrameAttachToBed(window_t *parent)
        : FrameInstructions(parent, _(text_attach_to_bed)) {}

    static constexpr const char *text_attach_to_bed = N_(
        "Firmly attach the accelerometer to the heatbed. "
        "In the next step, heatbed will start vibrating and resonance will be measured.");
};

class FrameMeasuringExtruder final : public FrameMeasurement {
public:
    explicit FrameMeasuringExtruder(window_t *parent)
        : FrameMeasurement(parent, _(text_measuring_x_axis)) {}

    static constexpr const char *text_measuring_x_axis = N_("Measuring X resonance...");
};

class FrameMeasuringBed final : public FrameMeasurement {
public:
    explicit FrameMeasuringBed(window_t *parent)
        : FrameMeasurement(parent, _(text_measuring_y_axis)) {}

    static constexpr const char *text_measuring_y_axis = N_("Measuring Y resonance...");
};

class FrameComputing final {
private:
    window_text_t text_above;
    window_text_t text_below;
    window_wizard_progress_t progress;
    std::array<char, sizeof("Axis X shaper XXX")> text_below_buffer;

public:
    FrameComputing(window_t *parent)
        : text_above(parent, rect_frame_top, is_multiline::no, is_closed_on_click_t::no, _(text_computing))
        , text_below(parent, rect_frame_bottom, is_multiline::no, is_closed_on_click_t::no)
        , progress(parent, 100 /*TODO*/) {
        text_above.SetAlignment(Align_t::CenterTop());
        text_below.SetAlignment(Align_t::CenterTop());
    }

    void update(fsm::PhaseData data) {
        progress.SetProgressPercent(data[0] / 2.55);
        snprintf(text_below_buffer.data(), text_below_buffer.size(), "Axis %c shaper %3s",
            data[2] == 0 ? 'X' : (data[2] == 1 ? 'Y' : '?'), input_shaper::to_short_string(static_cast<input_shaper::Type>(data[1])));
        text_below.SetText(string_view_utf8::MakeRAM(text_below_buffer.data()));
        text_below.Invalidate();
    }

    static constexpr const char *text_computing = N_("Computing best shaper...");
};

class FrameBadResults {
private:
    window_text_t text_above;
    window_text_t text_below;

    ArrayStringBuilder<150> str_build_x_axis;
    ArrayStringBuilder<150> str_build_y_axis;

    static constexpr const char *text_freq_low = N_("axis frequency is too low.\nPlease tighten the belt.");
    static constexpr const char *text_freq_high = N_("axis frequency is too high.\nPlease check your HW setup.\nIf the problem prevails, contact the customer support.");
    static constexpr const char *text_shaper_x = N_("Recommended shaper frequency for X axis: ");
    static constexpr const char *text_shaper_y = N_("Recommended shaper frequency for Y axis: ");
    static constexpr const char *type_freq_format = "%3s %3dHz";
    static constexpr const char *text_x_axis = "X ";
    static constexpr const char *text_y_axis = "Y ";

public:
    FrameBadResults(window_t *parent)
        : text_above(parent, Rect16(rect_frame.Left(), rect_frame.Top(), rect_frame.Width(), WizardDefaults::row_h * 4), is_multiline::yes, is_closed_on_click_t::no)
        , text_below(parent, Rect16(rect_frame.Left(), rect_frame.Top() + WizardDefaults::row_h * 4, rect_frame.Width(), WizardDefaults::row_h * 4), is_multiline::yes, is_closed_on_click_t::no) {
    }

    void update(fsm::PhaseData data) {
        const auto x_type = static_cast<input_shaper::Type>(data[0]);
        const auto x_freq = data[1];
        const auto y_type = static_cast<input_shaper::Type>(data[2]);
        const auto y_freq = data[3];

        if (x_freq < input_shaper::low_freq_limit_hz) {
            str_build_x_axis.append_string(text_x_axis);
            str_build_x_axis.append_string_view(_(text_freq_low));
        } else if (x_freq > input_shaper::high_freq_limit_hz) {
            str_build_x_axis.append_string(text_x_axis);
            str_build_x_axis.append_string_view(_(text_freq_high));
        } else {
            str_build_x_axis.append_string_view(_(text_shaper_x));
            str_build_x_axis.append_printf(type_freq_format, to_short_string(x_type), x_freq);
        }

        if (y_freq < input_shaper::low_freq_limit_hz) {
            str_build_y_axis.append_string(text_y_axis);
            str_build_y_axis.append_string_view(_(text_freq_low));
        } else if (y_freq > input_shaper::high_freq_limit_hz) {
            str_build_y_axis.append_string(text_y_axis);
            str_build_y_axis.append_string_view(_(text_freq_high));
        } else {
            str_build_y_axis.append_string_view(_(text_shaper_y));
            str_build_y_axis.append_printf(type_freq_format, to_short_string(y_type), y_freq);
        }

        text_above.SetText(string_view_utf8::MakeRAM(str_build_x_axis.str()));
        text_below.SetText(string_view_utf8::MakeRAM(str_build_y_axis.str()));
    }
};

class FrameResults {
private:
    window_text_t text;
    StringViewUtf8Parameters<20> params;

public:
    FrameResults(window_t *parent)
        : text(parent, rect_frame, is_multiline::yes, is_closed_on_click_t::no) {
    }

    void update(fsm::PhaseData data) {
        const auto x_type = static_cast<input_shaper::Type>(data[0]);
        const auto x_freq = data[1];
        const auto y_type = static_cast<input_shaper::Type>(data[2]);
        const auto y_freq = data[3];

        static const char *some_EN_text = N_("Computed shapers:\n  X axis %3s %3dHz\n  Y axis %3s %3dHz\nStore and use computed values?");
        const string_view_utf8 str = _(some_EN_text).formatted(params, to_short_string(x_type), x_freq, to_short_string(y_type), y_freq);
        text.SetText(str);
        text.Invalidate();
    }
};

class FrameMeasurementFailed final : public FrameInstructions {
public:
    explicit FrameMeasurementFailed(window_t *parent)
        : FrameInstructions(parent, _(text_measurement_failed)) {}

    static constexpr const char *text_measurement_failed = N_("Measurement failed.");
};

using Frames = FrameDefinitionList<ScreenInputShaperCalibration::FrameStorage,
    FrameDefinition<PhasesInputShaperCalibration::info, FrameInfo>,
    FrameDefinition<PhasesInputShaperCalibration::parking, FrameParking>,
    FrameDefinition<PhasesInputShaperCalibration::connect_to_board, FrameConnectToBoard>,
    FrameDefinition<PhasesInputShaperCalibration::wait_for_extruder_temperature, FrameWaitForExtruderTemperature>,
    FrameDefinition<PhasesInputShaperCalibration::attach_to_extruder, FrameAttachToExtruder>,
    FrameDefinition<PhasesInputShaperCalibration::measuring_x_axis, FrameMeasuringExtruder>,
    FrameDefinition<PhasesInputShaperCalibration::attach_to_bed, FrameAttachToBed>,
    FrameDefinition<PhasesInputShaperCalibration::measuring_y_axis, FrameMeasuringBed>,
    FrameDefinition<PhasesInputShaperCalibration::measurement_failed, FrameMeasurementFailed>,
    FrameDefinition<PhasesInputShaperCalibration::computing, FrameComputing>,
    FrameDefinition<PhasesInputShaperCalibration::bad_results, FrameBadResults>,
    FrameDefinition<PhasesInputShaperCalibration::results, FrameResults>>;

static PhasesInputShaperCalibration get_phase(const fsm::BaseData &fsm_base_data) {
    return GetEnumFromPhaseIndex<PhasesInputShaperCalibration>(fsm_base_data.GetPhase());
}

ScreenInputShaperCalibration::ScreenInputShaperCalibration()
    : ScreenFSM { text_header, rect_screen }
    , radio(this, rect_radio, PhasesInputShaperCalibration::finish) {
    CaptureNormalWindow(radio);
    create_frame();
    instance = this;
}

ScreenInputShaperCalibration::~ScreenInputShaperCalibration() {
    instance = nullptr;
    destroy_frame();
}

ScreenInputShaperCalibration *ScreenInputShaperCalibration::GetInstance() {
    return instance;
}

void ScreenInputShaperCalibration::create_frame() {
    Frames::create_frame(frame_storage, get_phase(fsm_base_data), this);
    radio.Change(get_phase(fsm_base_data));
}

void ScreenInputShaperCalibration::destroy_frame() {
    Frames::destroy_frame(frame_storage, get_phase(fsm_base_data));
}

void ScreenInputShaperCalibration::update_frame() {
    Frames::update_frame(frame_storage, get_phase(fsm_base_data), fsm_base_data.GetData());
}
