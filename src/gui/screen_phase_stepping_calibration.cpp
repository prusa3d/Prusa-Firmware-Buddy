#include <screen_phase_stepping_calibration.hpp>

#include "frame_qr_layout.hpp"
#include "i18n.h"
#include "img_resources.hpp"
#include <array>
#include <gui/frame_calibration_common.hpp>
#include <gui/qr.hpp>
#include <guiconfig/wizard_config.hpp>
#include <window_icon.hpp>
#include <window_progress.hpp>
#include <window_text.hpp>
#include <common/utils/string_builder.hpp>
#include <Marlin/src/feature/phase_stepping/calibration.hpp>
#include <bsod/bsod.h>

namespace {

using Buffer = std::array<char, 36>;
// Generate a link into the provided buffer, return the pointer to the buffer (for convenience).
//
// Eg. prusa.io/mk4-phstep-qr
//
// Uses the firmware base, eg. MK4S and MK3.9 are also mk4 in here.
Buffer addr(bool qr) {
    Buffer buffer;
    StringBuilder builder(buffer);
    builder.append_string("prusa.io/");
    debug_assert(PrinterModelInfo::firmware_base().help_url);
    builder.append_string(PrinterModelInfo::firmware_base().help_url);
    builder.append_string("-phstep");
    if (qr) {
        builder.append_string("-qr");
    }
    // Check that we fit within the 36 character buffer.
    debug_assert(!builder.is_problem());
    return buffer;
}

constexpr const char *txt_header { N_("PHASE STEPPING CALIBRATION") };
constexpr const char *txt_learn_more { N_("To learn more about the phase stepping calibration process, read the article:") };
constexpr const char *txt_homing { N_("Homing") };
constexpr const char *txt_calibrating { N_("Running the phase stepping calibration to reduce vibrations. Please wait...") };
constexpr const char *txt_calibrating_x { N_("Calibrating X motor") };
constexpr const char *txt_calibrating_y { N_("Calibrating Y motor") };

namespace frame {

    class CenteredText {
    protected:
        window_text_t text;

    public:
        CenteredText(window_t *parent, const string_view_utf8 &txt)
            : text(parent, ScreenPhaseSteppingCalibration::get_inner_frame_rect(), is_multiline::yes, is_closed_on_click_t::no, txt) {
            text.SetAlignment(Align_t::Center());
        }
    };

    class CenteredStaticText : public CenteredText {
    public:
        using CenteredText::CenteredText;
        void update(const fsm::PhaseData &) {}
    };

    class CalibratingMotor {
    private:
        window_text_t text;
        window_text_t title;
        WindowProgressBar progress_bar;
        window_text_t phase_x_of_y;
        std::array<char, 10> phase_x_of_y_buffer;

        static constexpr uint16_t padding = 20;

        static constexpr Rect16 text_rect {
            ScreenPhaseSteppingCalibration::get_inner_frame_rect().Left() + padding,
            ScreenPhaseSteppingCalibration::get_inner_frame_rect().Top() + padding,
            ScreenPhaseSteppingCalibration::get_inner_frame_rect().Width() - 2 * padding,
            2 * height(GuiDefaults::DefaultFont),
        };

        static constexpr uint16_t additional_title_rect_padding = 30;
        static constexpr Rect16 title_rect {
            text_rect.Left(),
            text_rect.Bottom() + padding + additional_title_rect_padding,
            text_rect.Width(),
            height(GuiDefaults::DefaultFont),
        };

        static constexpr uint16_t progress_bar_height = 4;
        static constexpr uint16_t progress_bar_vertical_margin = 50;
        static constexpr Rect16 progress_bar_rect {
            ScreenPhaseSteppingCalibration::get_inner_frame_rect().Left() + progress_bar_vertical_margin,
            title_rect.Bottom() + padding,
            ScreenPhaseSteppingCalibration::get_inner_frame_rect().Width() - 2 * progress_bar_vertical_margin,
            progress_bar_height,
        };

        static constexpr Rect16 phase_x_of_y_rect {
            text_rect.Left(),
            progress_bar_rect.Bottom() + padding,
            text_rect.Width(),
            height(GuiDefaults::DefaultFont),
        };

    public:
        CalibratingMotor(window_t *parent, const string_view_utf8 &txt)
            : text { parent, text_rect, is_multiline::yes, is_closed_on_click_t::no, _(txt_calibrating) }
            , title { parent, title_rect, is_multiline::no, is_closed_on_click_t::no, txt }
            , progress_bar { parent, progress_bar_rect, COLOR_BRAND, COLOR_DARK_GRAY }
            , phase_x_of_y { parent, phase_x_of_y_rect, is_multiline::no, is_closed_on_click_t::no } {
            title.SetAlignment(Align_t::Center());
            phase_x_of_y.SetAlignment(Align_t::Center());
        }

        void update(const fsm::PhaseData &data) {
            const uint8_t current_calibration_phase = data[0];
            const uint8_t calibration_phases_count = data[1];
            if (calibration_phases_count) {
                const uint8_t progress = 100 * (2 * current_calibration_phase + 1) / (2 * calibration_phases_count);
                snprintf(phase_x_of_y_buffer.data(), phase_x_of_y_buffer.size(), "%d / %d", current_calibration_phase + 1, calibration_phases_count);
                phase_x_of_y.SetText(string_view_utf8::MakeRAM(phase_x_of_y_buffer.data()));
                phase_x_of_y.Invalidate();
                progress_bar.set_progress_percent(progress);
            } else {
                phase_x_of_y.SetText(string_view_utf8::MakeNULLSTR());
                phase_x_of_y.Invalidate();
                progress_bar.set_progress_percent(0);
            }
        }
    };

    class Introduction final {
        Buffer text_link;
        Buffer qr_link;
        window_text_t text;
        window_text_t link;
        window_icon_t icon_phone;
        QRStaticStringWindow qr;

    public:
        explicit Introduction(window_t *parent)
            : text_link { addr(false) }
            , qr_link { addr(true) }
            , text { parent, FrameQRLayout::text_rect(), is_multiline::yes, is_closed_on_click_t::no, _(txt_learn_more) }
            , link { parent, FrameQRLayout::link_rect(), is_multiline::no, is_closed_on_click_t::no, string_view_utf8::MakeRAM(text_link.data()) }
            , icon_phone { parent, FrameQRLayout::phone_icon_rect(), &img::hand_qr_59x72 }
            , qr { parent, FrameQRLayout::qrcode_rect(), Align_t::Center(), qr_link.data() } {
        }
        void update(const fsm::PhaseData &) {}
    };

    class Homing final : public CenteredStaticText {
    public:
        explicit Homing(window_t *parent)
            : CenteredStaticText { parent, _(txt_homing) } {
        }
    };

    class CalibratingX final : public CalibratingMotor {
    public:
        explicit CalibratingX(window_t *parent)
            : CalibratingMotor { parent, _(txt_calibrating_x) } {
        }
    };

    class CalibratingY final : public CalibratingMotor {
    public:
        explicit CalibratingY(window_t *parent)
            : CalibratingMotor { parent, _(txt_calibrating_y) } {
        }
    };

    class CalibrationOK final {
        window_text_t title;
        window_text_t motor_x;
        window_text_t motor_y;
        StringViewUtf8Parameters<10> motor_x_params;
        StringViewUtf8Parameters<10> motor_y_params;

        static constexpr uint16_t padding = 20;

        static constexpr Rect16 title_rect {
            ScreenPhaseSteppingCalibration::get_inner_frame_rect().Left() + padding,
            ScreenPhaseSteppingCalibration::get_inner_frame_rect().Top() + padding,
            ScreenPhaseSteppingCalibration::get_inner_frame_rect().Width() - 2 * padding,
            height(GuiDefaults::DefaultFont),
        };

        static constexpr Rect16 motor_x_rect {
            title_rect.Left(),
            title_rect.Bottom() + padding + 50,
            title_rect.Width(),
            height(Font::small),
        };

        static constexpr Rect16 motor_y_rect {
            motor_x_rect.Left(),
            motor_x_rect.Bottom() + padding,
            motor_x_rect.Width(),
            height(Font::small),
        };

    public:
        explicit CalibrationOK(window_t *parent)
            : title { parent, title_rect, is_multiline::no, is_closed_on_click_t::no }
            , motor_x { parent, motor_x_rect, is_multiline::no, is_closed_on_click_t::no }
            , motor_y { parent, motor_y_rect, is_multiline::no, is_closed_on_click_t::no } {
            title.SetAlignment(Align_t::Center());
            title.SetText(_("Phase stepping calibration completed"));
            motor_x.SetAlignment(Align_t::Center());
            motor_x.set_font(Font::small);
            motor_y.SetAlignment(Align_t::Center());
            motor_y.set_font(Font::small);
        }

        void update(const fsm::PhaseData &data) {
            const uint8_t reduction_x = data[0];
            const uint8_t reduction_y = data[1];

            static constexpr const char *motor_vibration_txt = N_("Motor %c vibration reduced by %2d%%");
            motor_x.SetText(_(motor_vibration_txt).formatted(motor_x_params, 'X', reduction_x));
            motor_y.SetText(_(motor_vibration_txt).formatted(motor_y_params, 'Y', reduction_y));
        }
    };

    class CalibrationError final {
    private:
        window_text_t text;
        window_text_t detail;

        static constexpr uint16_t padding = 20;

        static constexpr Rect16 text_rect {
            ScreenPhaseSteppingCalibration::get_inner_frame_rect().Left() + padding,
            ScreenPhaseSteppingCalibration::get_inner_frame_rect().Top() + padding,
            ScreenPhaseSteppingCalibration::get_inner_frame_rect().Width() - 2 * padding,
            height(GuiDefaults::DefaultFont),
        };

        static constexpr Rect16 detail_rect {
            text_rect.Left(),
            text_rect.Bottom() + padding,
            text_rect.Width(),
            2 * height(GuiDefaults::DefaultFont),
        };

    public:
        explicit CalibrationError(window_t *parent)
            : text {
                parent,
                text_rect,
                is_multiline::yes,
                is_closed_on_click_t::no,
                _("Calibration failed with error."),
            }
            , detail {
                parent,
                detail_rect,
                is_multiline::yes,
                is_closed_on_click_t::no,
            } {
        }

        void update(const fsm::PhaseData &data) {
            const auto error = static_cast<phase_stepping::CalibrateAxisError>(data[0]);
            detail.SetText(_(to_string(error)));
        }
    };

    class CalibrationNOK final : public CenteredStaticText {
    public:
        explicit CalibrationNOK(window_t *parent)
            : CenteredStaticText {
                parent,
                _("Calibration failed to find corrections to motor current."),
            } {
        }
    };

    class RestoreDefaults final : public CenteredStaticText {
    public:
        explicit RestoreDefaults(window_t *parent)
            : CenteredStaticText {
                parent,
                _("Phase stepping defaults have been successfully restored."),
            } {
        }
    };

} // namespace frame

using Frames = FrameDefinitionList<ScreenPhaseSteppingCalibration::FrameStorage,
    FrameDefinition<PhasesPhaseStepping::restore_defaults, frame::RestoreDefaults>,
    FrameDefinition<PhasesPhaseStepping::intro, frame::Introduction>,
    FrameDefinition<PhasesPhaseStepping::home, frame::Homing>,
#if HAS_ATTACHABLE_ACCELEROMETER()
    FrameDefinition<PhasesPhaseStepping::connect_to_board, standard_frame_without_radio::FrameConnectToBoard>,
    FrameDefinition<PhasesPhaseStepping::wait_for_extruder_temperature, standard_frame_without_radio::FrameWaitForExtruderTemperature>,
    FrameDefinition<PhasesPhaseStepping::attach_to_extruder, standard_frame_without_radio::FrameAttachToExtruder>,
    FrameDefinition<PhasesPhaseStepping::attach_to_bed, standard_frame_without_radio::FrameAttachToBed>,
#endif
    FrameDefinition<PhasesPhaseStepping::calib_x, frame::CalibratingX>,
    FrameDefinition<PhasesPhaseStepping::calib_y, frame::CalibratingY>,
    FrameDefinition<PhasesPhaseStepping::calib_nok, frame::CalibrationNOK>,
    FrameDefinition<PhasesPhaseStepping::calib_error, frame::CalibrationError>,
    FrameDefinition<PhasesPhaseStepping::calib_ok, frame::CalibrationOK>>;

} // namespace

ScreenPhaseSteppingCalibration::ScreenPhaseSteppingCalibration()
    : ScreenFSM(txt_header, ScreenPhaseSteppingCalibration::get_inner_frame_rect())
    , radio { this, rect_radio, PhasesPhaseStepping::intro } {
    header.SetIcon(&img::selftest_16x16);
    CaptureNormalWindow(radio);
    create_frame();
}

ScreenPhaseSteppingCalibration::~ScreenPhaseSteppingCalibration() {
    destroy_frame();
}

void ScreenPhaseSteppingCalibration::create_frame() {
    Frames::create_frame(frame_storage, get_phase(), &inner_frame);
    radio.Change(get_phase());
}

void ScreenPhaseSteppingCalibration::destroy_frame() {
    Frames::destroy_frame(frame_storage, get_phase());
}

void ScreenPhaseSteppingCalibration::update_frame() {
    Frames::update_frame(frame_storage, get_phase(), fsm_base_data.GetData());
}
