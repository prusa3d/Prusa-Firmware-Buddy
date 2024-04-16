#include <screen_phase_stepping.hpp>

#include "frame_qr_layout.hpp"
#include "i18n.h"
#include "img_resources.hpp"
#include "str_utils.hpp"
#include <array>
#include <guiconfig/wizard_config.hpp>
#include <window_icon.hpp>
#include <window_progress.hpp>
#include <window_qr.hpp>
#include <window_text.hpp>

namespace {
#if PRINTER_IS_PRUSA_XL
constexpr const char *QR_ADDR = "prusa.io/xl-phstep-qr";
constexpr const char *ADDR_IN_TEXT = "prusa.io/xl-phstep";
#else
    #error
#endif
constexpr const char *txt_header { N_("PHASE STEPPING CALIBRATION") };
constexpr const char *txt_learn_more { N_("To learn more about the phase stepping calibration process, read the article:") };
constexpr const char *txt_picking_tool { N_("Picking Tool") };
constexpr const char *txt_calibrating { N_("Running the phase stepping calibration to reduce vibrations. Please wait...") };
constexpr const char *txt_calibrating_x { N_("Calibrating X motor") };
constexpr const char *txt_calibrating_y { N_("Calibrating Y motor") };
constexpr const char *txt_calibration_nok { N_("Calibration of motor %c failed.\nParameter 1: forward %3d%%, backward %3d%%\nParameter 2: forward %3d%%, backward %3d%%") };
constexpr const char *txt_calibration_error { N_("Calibration failed with error.") };

constexpr Rect16 radio_rect = GuiDefaults::GetButtonRect(GuiDefaults::RectScreenBody);
constexpr Rect16 inner_frame_rect = GuiDefaults::RectScreenBody - radio_rect.Height();

namespace frame {

    class CenteredText {
    protected:
        window_text_t text;

    public:
        CenteredText(window_t *parent, string_view_utf8 txt)
            : text(parent, inner_frame_rect, is_multiline::yes, is_closed_on_click_t::no, txt) {
            text.SetAlignment(Align_t::Center());
        }
    };

    class CenteredStaticText : public CenteredText {
    public:
        using CenteredText::CenteredText;
        void update(const fsm::PhaseData &) {}
    };

    class CalibrationNOK : public CenteredText {
    private:
        std::array<char, 150> text_buffer;
        char motor;

    public:
        explicit CalibrationNOK(window_t *parent, char motor)
            : CenteredText { parent, string_view_utf8::MakeNULLSTR() }
            , motor { motor } {
        }

        void update(const fsm::PhaseData &data) {
            std::array<char, 150> fmt;
            _(txt_calibration_nok).copyToRAM(fmt.data(), fmt.size());
            snprintf(text_buffer.data(), text_buffer.size(), fmt.data(), motor, data[0], data[1], data[2], data[3]);
            text.SetText(string_view_utf8::MakeRAM((const uint8_t *)text_buffer.data()));
        }
    };

    class CalibratingMotor {
    private:
        window_text_t text;
        window_text_t title;
        window_numberless_progress_t progress_bar;
        window_text_t phase_x_of_y;
        std::array<char, 10> phase_x_of_y_buffer;

        static constexpr uint16_t padding = 20;

        static constexpr Rect16 text_rect {
            inner_frame_rect.Left() + padding,
            inner_frame_rect.Top() + padding,
            inner_frame_rect.Width() - 2 * padding,
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
            inner_frame_rect.Left() + progress_bar_vertical_margin,
            title_rect.Bottom() + padding,
            inner_frame_rect.Width() - 2 * progress_bar_vertical_margin,
            progress_bar_height,
        };

        static constexpr Rect16 phase_x_of_y_rect {
            text_rect.Left(),
            progress_bar_rect.Bottom() + padding,
            text_rect.Width(),
            height(GuiDefaults::DefaultFont),
        };

    public:
        CalibratingMotor(window_t *parent, string_view_utf8 txt)
            : text { parent, text_rect, is_multiline::yes, is_closed_on_click_t::no, _(txt_calibrating) }
            , title { parent, title_rect, is_multiline::no, is_closed_on_click_t::no, txt }
            , progress_bar { parent, progress_bar_rect, COLOR_ORANGE, COLOR_DARK_GRAY }
            , phase_x_of_y { parent, phase_x_of_y_rect, is_multiline::no, is_closed_on_click_t::no } {
            title.SetAlignment(Align_t::Center());
            phase_x_of_y.SetAlignment(Align_t::Center());
        }

        void update(const fsm::PhaseData &data) {
            const uint8_t current_calibration_phase = data[0];
            const uint8_t calibration_phases_count = data[1];
            const uint8_t progress = data[2];

            snprintf(phase_x_of_y_buffer.data(), phase_x_of_y_buffer.size(), "%d / %d", current_calibration_phase + 1, calibration_phases_count);
            phase_x_of_y.SetText(string_view_utf8::MakeRAM(phase_x_of_y_buffer.data()));
            phase_x_of_y.Invalidate();
            progress_bar.SetProgressPercent(progress);
        }
    };

    class Introduction final {
        window_text_t text;
        window_text_t link;
        window_icon_t icon_phone;
        window_qr_t qr;

    public:
        explicit Introduction(window_t *parent)
            : text { parent, FrameQRLayout::text_rect(), is_multiline::yes, is_closed_on_click_t::no, _(txt_learn_more) }
            , link { parent, FrameQRLayout::link_rect(), is_multiline::no, is_closed_on_click_t::no, string_view_utf8::MakeCPUFLASH(reinterpret_cast<const uint8_t *>(ADDR_IN_TEXT)) }
            , icon_phone { parent, FrameQRLayout::phone_icon_rect(), &img::hand_qr_59x72 }
            , qr { parent, FrameQRLayout::qrcode_rect(), QR_ADDR } {
        }
        void update(const fsm::PhaseData &) {}
    };

    class PickingTool final : public CenteredStaticText {
    public:
        explicit PickingTool(window_t *parent)
            : CenteredStaticText { parent, _(txt_picking_tool) } {
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
        using TextBuffer = std::array<char, 50>;
        TextBuffer motor_x_buffer;
        TextBuffer motor_y_buffer;

        static constexpr uint16_t padding = 20;

        static constexpr Rect16 title_rect {
            inner_frame_rect.Left() + padding,
            inner_frame_rect.Top() + padding,
            inner_frame_rect.Width() - 2 * padding,
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

        static void update_helper(TextBuffer &text_buffer, window_text_t &text, const char motor, const uint8_t reduction) {
            TextBuffer fmt;
            _("Motor %c vibration reduced by %2d%%").copyToRAM(fmt.data(), fmt.size());
            snprintf(text_buffer.data(), text_buffer.size(), fmt.data(), motor, reduction);
            text.SetText(string_view_utf8::MakeRAM(text_buffer.data()));
        }

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
            update_helper(motor_x_buffer, motor_x, 'X', (data[0] + data[1]) / 2);
            update_helper(motor_y_buffer, motor_y, 'Y', (data[2] + data[3]) / 2);
        }
    };

    class CalibrationError final : public CenteredStaticText {
    public:
        explicit CalibrationError(window_t *parent)
            : CenteredStaticText { parent, _(txt_calibration_error) } {
        }
    };

    class CalibrationXNOK final : public CalibrationNOK {
    public:
        explicit CalibrationXNOK(window_t *parent)
            : CalibrationNOK { parent, 'X' } {}
    };

    class CalibrationYNOK final : public CalibrationNOK {
    public:
        explicit CalibrationYNOK(window_t *parent)
            : CalibrationNOK { parent, 'Y' } {}
    };

} // namespace frame

ScreenPhaseStepping *instance = nullptr;

PhasesPhaseStepping get_phase(const fsm::BaseData &fsm_base_data) {
    return GetEnumFromPhaseIndex<PhasesPhaseStepping>(fsm_base_data.GetPhase());
}

template <PhasesPhaseStepping Phase, class Frame>
struct FrameDefinition {
    using FrameType = Frame;
    static constexpr PhasesPhaseStepping phase = Phase;
};

template <class Storage, class... T>
struct FrameDefinitionList {
    template <class F>
    using FrameType = typename F::FrameType;

    static_assert(Storage::template has_ideal_size_for<FrameType<T>...>());

    static void create_frame(Storage &storage, PhasesPhaseStepping phase, window_t *parent) {
        auto f = [&]<typename FD> {
            if (phase == FD::phase) {
                storage.template create<typename FD::FrameType>(parent);
            }
        };
        (f.template operator()<T>(), ...);
    }

    static void destroy_frame(Storage &storage, PhasesPhaseStepping phase) {
        auto f = [&]<typename FD> {
            if (phase == FD::phase) {
                storage.template destroy<typename FD::FrameType>();
            }
        };
        (f.template operator()<T>(), ...);
    }

    static void update_frame(Storage &storage, PhasesPhaseStepping phase, const fsm::PhaseData &data) {
        auto f = [&]<typename FD> {
            if (phase == FD::phase) {
                storage.template as<typename FD::FrameType>()->update(data);
            }
        };
        (f.template operator()<T>(), ...);
    }
};

using Frames = FrameDefinitionList<ScreenPhaseStepping::FrameStorage,
    FrameDefinition<PhasesPhaseStepping::intro, frame::Introduction>,
    FrameDefinition<PhasesPhaseStepping::pick_tool, frame::PickingTool>,
    FrameDefinition<PhasesPhaseStepping::calib_x, frame::CalibratingX>,
    FrameDefinition<PhasesPhaseStepping::calib_y, frame::CalibratingY>,
    FrameDefinition<PhasesPhaseStepping::calib_x_nok, frame::CalibrationXNOK>,
    FrameDefinition<PhasesPhaseStepping::calib_y_nok, frame::CalibrationYNOK>,
    FrameDefinition<PhasesPhaseStepping::calib_error, frame::CalibrationError>,
    FrameDefinition<PhasesPhaseStepping::calib_ok, frame::CalibrationOK>>;

} // namespace

ScreenPhaseStepping::ScreenPhaseStepping()
    : AddSuperWindow<screen_t> {}
    , header { this, _(txt_header) }
    , inner_frame { this, inner_frame_rect }
    , radio(this, radio_rect, PhasesPhaseStepping::intro) {
    ClrMenuTimeoutClose();
    CaptureNormalWindow(radio);
    create_frame();
    instance = this;
}

ScreenPhaseStepping::~ScreenPhaseStepping() {
    instance = nullptr;
    ReleaseCaptureOfNormalWindow();
}

ScreenPhaseStepping *ScreenPhaseStepping::GetInstance() {
    return instance;
}

void ScreenPhaseStepping::Change(fsm::BaseData data) {
    return do_change(data);
}

void ScreenPhaseStepping::InitState(screen_init_variant var) {
    if (auto fsm_base_data = var.GetFsmBaseData()) {
        do_change(*fsm_base_data);
    }
}

screen_init_variant ScreenPhaseStepping::GetCurrentState() const {
    screen_init_variant var;
    var.SetFsmBaseData(fsm_base_data);
    return var;
}

void ScreenPhaseStepping::do_change(fsm::BaseData new_fsm_base_data) {
    if (new_fsm_base_data.GetPhase() != fsm_base_data.GetPhase()) {
        destroy_frame();
        fsm_base_data = new_fsm_base_data;
        create_frame();
        radio.Change(get_phase(fsm_base_data));
    } else {
        fsm_base_data = new_fsm_base_data;
    }
    update_frame();
}

void ScreenPhaseStepping::create_frame() {
    Frames::create_frame(frame_storage, get_phase(fsm_base_data), &inner_frame);
}

void ScreenPhaseStepping::destroy_frame() {
    Frames::destroy_frame(frame_storage, get_phase(fsm_base_data));
}

void ScreenPhaseStepping::update_frame() {
    Frames::update_frame(frame_storage, get_phase(fsm_base_data), fsm_base_data.GetData());
}
