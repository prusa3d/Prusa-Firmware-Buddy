#include "frame_qr_layout.hpp"
#include "img_resources.hpp"
#include "screen_cold_pull.hpp"
#include "window_progress.hpp"
#include "fonts.hpp"
#include <find_error.hpp>
#include "utility_extensions.hpp"

#include <guiconfig/wizard_config.hpp>
#include <common/cold_pull.hpp>
#include <common/sound.hpp>
#include <common/str_utils.hpp>
#include <lang/i18n.h>

namespace {

// Show message when this time is left to wait for
constexpr const unsigned TAKING_TOO_LONG_TIMEOUT_SEC { 5 * 60 };

ScreenColdPull *instance = nullptr;

const char *text_header = N_("COLD PULL");

constexpr Rect16 get_radio_frame() {
    return GuiDefaults::GetButtonRect(GuiDefaults::RectScreenBody);
}

constexpr Rect16 get_inner_frame() {
    return GuiDefaults::RectScreenBody - get_radio_frame().Height() - static_cast<Rect16::Height_t>(GuiDefaults::FramePadding);
}

constexpr const unsigned TITLE_TOP { 70 };
constexpr const unsigned LABEL_TOP { 180 };
constexpr const unsigned LABEL_PADDING { 12 };
constexpr const unsigned PROGRESS_TOP { 105 };
constexpr const unsigned PROGRESS_BAR_H { 16 };
constexpr const unsigned PROGRESS_NUM_Y_OFFSET { 10 };
constexpr const unsigned PROGRESS_H { 60 };
constexpr const unsigned PROGRESS_BAR_X_PAD { 24 };
constexpr const unsigned PROGRESS_BAR_CORNER_RADIUS { 4 };

constexpr Rect16 get_title_rect() {
    return {
        get_inner_frame().Left(),
        TITLE_TOP,
        get_inner_frame().Width(),
        GuiDefaults::ButtonHeight
    };
}

constexpr Rect16 get_info_rect() {
    return {
        get_inner_frame().Left() + LABEL_PADDING,
        LABEL_TOP,
        get_inner_frame().Width() - 2 * LABEL_PADDING,
        2 * GuiDefaults::ButtonHeight
    };
}

constexpr Rect16 get_progress_rect(Rect16 rect) {
    return Rect16(rect.Left() + PROGRESS_BAR_X_PAD, PROGRESS_TOP, rect.Width() - 2 * PROGRESS_BAR_X_PAD, PROGRESS_H);
}

constexpr Rect16 get_progress_bar_rect(const Rect16 parent_rect) {
    const Rect16 rect = get_progress_rect(parent_rect);
    return {
        rect.Left(),
        rect.Top(),
        rect.Width(),
        PROGRESS_BAR_H
    };
}

constexpr Rect16 get_progress_number_rect(const Rect16 parent_rect) {
    const Rect16 rect = get_progress_rect(parent_rect);
    return {
        rect.Left(),
        int16_t(rect.Top() + PROGRESS_BAR_H + PROGRESS_NUM_Y_OFFSET),
        rect.Width(),
        uint16_t(rect.Height() - PROGRESS_BAR_H - PROGRESS_NUM_Y_OFFSET)
    };
}

namespace Frame {

    /** common base class for two text frame */
    class TextFrame {
    protected:
        window_text_t title;
        window_text_t info;

    public:
        TextFrame(window_t *parent, const string_view_utf8 &txt_title, const string_view_utf8 &txt_info)
            : title(parent, get_title_rect(), is_multiline::no, is_closed_on_click_t::no, txt_title)
            , info(parent, get_info_rect(), is_multiline::yes, is_closed_on_click_t::no, txt_info) {
            title.SetAlignment(Align_t::Center());
            title.set_font(GuiDefaults::FontBig);
            info.SetAlignment(Align_t::Center());
            info.set_font(Font::small);
        }

        TextFrame(window_t *parent, const string_view_utf8 &txt_title)
            : TextFrame(parent, txt_title, {}) {}

        virtual ~TextFrame() = default;
    };

    /** common base class for two text frame with progress bar */
    class ProgressFrame : public TextFrame {
    protected:
        window_numberless_progress_t progress_bar;
        window_numb_t progress_number;

    public:
        ProgressFrame(window_t *parent, const string_view_utf8 &txt_title, const string_view_utf8 &txt_info)
            : TextFrame(parent, txt_title, txt_info)
            , progress_bar(parent, get_progress_bar_rect(parent->GetRect()), COLOR_ORANGE, COLOR_DARK_GRAY, PROGRESS_BAR_CORNER_RADIUS)
            , progress_number(parent, get_progress_number_rect(parent->GetRect()), 0, "%.0f%%", Font::big) {
            progress_number.SetAlignment(Align_t::Center());
        }

        virtual ~ProgressFrame() = default;
    };

    // For future use with XL/MMU; now here just for translations
    [[maybe_unused]] static constexpr const char *TODOtext3 = N_("Before you continue, unload the filament. Then press down the blue part on the fitting and pull the PTFE tube from the tool head.");
    [[maybe_unused]] static constexpr const char *TODOtext4 = N_("Before you continue, make sure PLA filament is loaded directly into the extruder.");

    /** individual frames */
    class Introduction final {
        window_text_t text;
        window_text_t link;
        window_icon_t icon_phone;
        window_qr_t qr;

        char error_code_str[10 + 5 + 1]; // static text before error code has 32 chars
    public:
        explicit Introduction(window_t *parent)
            : text(parent, FrameQRLayout::text_rect(), is_multiline::yes, is_closed_on_click_t::no)
            , link(parent, FrameQRLayout::link_rect(), is_multiline::no, is_closed_on_click_t::no)
            , icon_phone(parent, FrameQRLayout::phone_icon_rect(), &img::hand_qr_59x72)
            , qr(parent, FrameQRLayout::qrcode_rect()) {

            text.SetAlignment(Align_t::LeftCenter());

            const auto err_desc = find_error(ErrCode::WARNING_COLD_PULL_INTRO);
            text.SetText(_(err_desc.err_text));

            const auto err_code_num = ftrstd::to_underlying(ErrCode::WARNING_COLD_PULL_INTRO);
            qr.SetQRHeader(err_code_num);

            snprintf(error_code_str, sizeof(error_code_str), text_link, err_code_num);
            link.SetText(string_view_utf8::MakeRAM((const uint8_t *)error_code_str));
        }

        void update(fsm::PhaseData) {}

        static constexpr const char *text_link = "prusa.io/%05u";
    };

    class PrepareFilament final : public TextFrame {
    public:
        explicit PrepareFilament(window_t *parent)
            : TextFrame(parent, _(text_title), _(text_info)) {}

        void update(fsm::PhaseData) {}

        static constexpr const char *text_title = N_("Filament check");
        static constexpr const char *text_info = N_("Before you continue,\nmake sure that PLA filament is loaded.");
    };

    // Blank screen is needed to avoid short flicker of the lower screen when switching from Load filament dialog
    // to FramePreheat. There is short but noticable period where the underlaying screen is visible before
    // switch do next happens. If it's black it looks nice.
    // It goes from FrameLoadUnload -> FrameBlank -> Load-dialog -> FrameBlank -> FramePreheat.
    class Blank final {
    public:
        explicit Blank([[maybe_unused]] window_t *parent) {}

        void update(fsm::PhaseData) {}
    };

    class CoolDown final : public ProgressFrame {
        bool has_text3 { false };

    public:
        explicit CoolDown(window_t *parent)
            : ProgressFrame(parent, _(text1), _(text2)) {
        }

        void update(fsm::PhaseData fsm_data) {
            const cold_pull::TemperatureProgressData data { fsm_data };
            progress_bar.SetProgressPercent(data.percent);
            if (data.time_sec > TAKING_TOO_LONG_TIMEOUT_SEC) {
                progress_number.SetValue(data.percent);
            } else {
                if (!has_text3) {
                    // one-time change
                    has_text3 = true;
                    info.SetText(_(text3));
                    progress_number.PrintAsTime();
                }
                progress_number.SetValue(data.time_sec);
            }
        }

        static constexpr const char *text1 = N_("Cooling the nozzle");
        static constexpr const char *text2 = N_("Don't touch the extruder.");
        static constexpr const char *text3 = N_("Takes too long, will skip soon.");
    };

    class HeatUp final : public ProgressFrame {
    public:
        explicit HeatUp(window_t *parent)
            : ProgressFrame(parent, _(text1), _(text2)) {
            Sound_Play(eSOUND_TYPE::SingleBeep);
            Sound_Play(eSOUND_TYPE::SingleBeep);
        }

        void update(fsm::PhaseData fsm_data) {
            const cold_pull::TemperatureProgressData data { fsm_data };
            progress_bar.SetProgressPercent(data.percent);
            progress_number.SetValue(data.percent);
        }

        static constexpr const char *text1 = N_("Heating up the nozzle");
        static constexpr const char *text2 = N_("The filament will be unloaded automatically.");
    };

    class AutomaticPull final : public TextFrame {
    public:
        explicit AutomaticPull(window_t *parent)
            : TextFrame(parent, _(text1)) {
            Sound_Play(eSOUND_TYPE::SingleBeep);
        }

        void update(fsm::PhaseData) {}

        static constexpr const char *text1 = N_("Unloading");
    };

    class ManualPull final : public TextFrame {
    public:
        explicit ManualPull(window_t *parent)
            : TextFrame(parent, _(text1), _(text2)) {
        }

        void update(fsm::PhaseData) {}

        static constexpr const char *text1 = N_("Remove the filament manually");
        static constexpr const char *text2 = N_("There might be a slight resistance.\nIf the filament is stuck, open the idler lever.");
    };

    class PullDone final : public TextFrame {
    public:
        explicit PullDone(window_t *parent)
            : TextFrame(parent, _(text1), _(text2)) {
        }

        void update(fsm::PhaseData) {}

        static constexpr const char *text1 = N_("Cold Pull successfully completed");
        static constexpr const char *text2 = N_("You can continue printing. If the issue persists,\nrepeat this procedure again.");

        // for XL only, enabled now because of translations
        [[maybe_unused]] static constexpr const char *TODOtext10 = N_("Cold Pull successfully completed. Insert PTFE tube back in the fitting. You can continue printing. If the issue persists, repeat this procedure again.");
    };

} // namespace Frame

PhasesColdPull get_cold_pull_phase(fsm::BaseData fsm_base_data) {
    return GetEnumFromPhaseIndex<PhasesColdPull>(fsm_base_data.GetPhase());
}

template <PhasesColdPull Phase, class Frame>
struct FrameDefinition {
    using FrameType = Frame;
    static constexpr PhasesColdPull phase = Phase;
};

template <class Storage, class... T>
struct FrameDefinitionList {
    template <class F>
    using FrameType = typename F::FrameType;

    static_assert(Storage::template has_ideal_size_for<FrameType<T>...>());

    static void create_frame(Storage &storage, PhasesColdPull phase, window_t *parent) {
        auto f = [&]<typename FD> {
            if (phase == FD::phase) {
                storage.template create<typename FD::FrameType>(parent);
            }
        };
        (f.template operator()<T>(), ...);
    }

    static void destroy_frame(Storage &storage, PhasesColdPull phase) {
        auto f = [&]<typename FD> {
            if (phase == FD::phase) {
                storage.template destroy<typename FD::FrameType>();
            }
        };
        (f.template operator()<T>(), ...);
    }

    static void update_frame(Storage &storage, PhasesColdPull phase, fsm::PhaseData data) {
        auto f = [&]<typename FD> {
            if (phase == FD::phase) {
                storage.template as<typename FD::FrameType>()->update(data);
            }
        };
        (f.template operator()<T>(), ...);
    }
};

using Frames = FrameDefinitionList<ScreenColdPull::FrameStorage,
    FrameDefinition<PhasesColdPull::introduction, Frame::Introduction>,
    FrameDefinition<PhasesColdPull::prepare_filament, Frame::PrepareFilament>,
    FrameDefinition<PhasesColdPull::blank_load, Frame::Blank>,
    FrameDefinition<PhasesColdPull::blank_unload, Frame::Blank>,
    FrameDefinition<PhasesColdPull::cool_down, Frame::CoolDown>,
    FrameDefinition<PhasesColdPull::heat_up, Frame::HeatUp>,
    FrameDefinition<PhasesColdPull::automatic_pull, Frame::AutomaticPull>,
    FrameDefinition<PhasesColdPull::manual_pull, Frame::ManualPull>,
    FrameDefinition<PhasesColdPull::pull_done, Frame::PullDone>>;

} // namespace

ScreenColdPull::ScreenColdPull()
    : AddSuperWindow<screen_t> {}
    , header { this, _(text_header) }
    , footer { this, 0, footer::Item::nozzle, footer::Item::bed, footer::Item::heatbreak_temp }
    , radio { this, get_radio_frame(), PhasesColdPull::introduction }
    , inner_frame { this, get_inner_frame() } {
    ClrMenuTimeoutClose();
    CaptureNormalWindow(radio);
    create_frame();
    instance = this;
}

ScreenColdPull::~ScreenColdPull() {
    instance = nullptr;
    destroy_frame();
    ReleaseCaptureOfNormalWindow();
}

ScreenColdPull *ScreenColdPull::GetInstance() { return instance; }

void ScreenColdPull::Change(fsm::BaseData data) { do_change(data); }

void ScreenColdPull::InitState(screen_init_variant var) {
    if (auto fsm_base_data = var.GetFsmBaseData()) {
        do_change(*fsm_base_data);
    }
}

screen_init_variant ScreenColdPull::GetCurrentState() const {
    screen_init_variant var;
    var.SetFsmBaseData(fsm_base_data);
    return var;
}

void ScreenColdPull::do_change(fsm::BaseData new_fsm_base_data) {
    if (new_fsm_base_data.GetPhase() != fsm_base_data.GetPhase()) {
        destroy_frame();
        fsm_base_data = new_fsm_base_data;
        create_frame();
        radio.Change(get_cold_pull_phase(fsm_base_data));
    } else {
        fsm_base_data = new_fsm_base_data;
    }
    update_frame();
}

void ScreenColdPull::create_frame() {
    Frames::create_frame(frame_storage, get_cold_pull_phase(fsm_base_data), &inner_frame);
}

void ScreenColdPull::destroy_frame() {
    Frames::destroy_frame(frame_storage, get_cold_pull_phase(fsm_base_data));
}

void ScreenColdPull::update_frame() {
    Frames::update_frame(frame_storage, get_cold_pull_phase(fsm_base_data), fsm_base_data.GetData());
}
