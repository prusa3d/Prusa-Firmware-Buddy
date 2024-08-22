#include "frame_qr_layout.hpp"
#include "img_resources.hpp"
#include "screen_cold_pull.hpp"
#include "window_progress.hpp"
#include "fonts.hpp"
#include "utility_extensions.hpp"

#if HAS_TOOLCHANGER()
    #include <window_tool_action_box.hpp>
#endif

#if HAS_MMU2()
    #include <feature/prusa/MMU2/mmu2_mk4.h>
#endif

#include <find_error.hpp>
#include <gui/text_error_url.hpp>
#include <gui/qr.hpp>
#include <guiconfig/wizard_config.hpp>
#include <common/cold_pull.hpp>
#include <common/sound.hpp>
#include <common/str_utils.hpp>
#include <lang/i18n.h>

namespace {

// Show message when this time is left to wait for
constexpr const unsigned TAKING_TOO_LONG_TIMEOUT_SEC { 5 * 60 };

const char *text_header = N_("COLD PULL");

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
        ScreenColdPull::get_inner_frame_rect().Left(),
        TITLE_TOP,
        ScreenColdPull::get_inner_frame_rect().Width(),
        GuiDefaults::ButtonHeight
    };
}

constexpr Rect16 get_info_rect() {
    return {
        ScreenColdPull::get_inner_frame_rect().Left() + LABEL_PADDING,
        LABEL_TOP,
        ScreenColdPull::get_inner_frame_rect().Width() - 2 * LABEL_PADDING,
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

namespace frame {

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
    };

    /** individual frames */
    class Introduction final {
        window_text_t text;
        TextErrorUrlWindow link;
        window_icon_t icon_phone;
        QRErrorUrlWindow qr;

    public:
        explicit Introduction(window_t *parent)
            : text(parent, FrameQRLayout::text_rect(), is_multiline::yes, is_closed_on_click_t::no)
            , link(parent, FrameQRLayout::link_rect(), ErrCode::WARNING_COLD_PULL_INTRO)
            , icon_phone(parent, FrameQRLayout::phone_icon_rect(), &img::hand_qr_59x72)
            , qr(parent, FrameQRLayout::qrcode_rect(), ErrCode::WARNING_COLD_PULL_INTRO) {

            text.SetAlignment(Align_t::LeftCenter());

            const auto err_desc = find_error(ErrCode::WARNING_COLD_PULL_INTRO);
            text.SetText(_(err_desc.err_text));
        }
    };

#if HAS_TOOLCHANGER()
    class SelectTool final : public ToolBox::DialogToolActionBox<ToolBox::MenuSelect> {
    public:
        SelectTool(window_t *) {
            Screens::Access()->gui_loop_until_dialog_closed();
            Response r;
            switch (get_result()) {
            case ToolBox::DialogResult::Tool1:
                r = Response::Tool1;
                break;
            case ToolBox::DialogResult::Tool2:
                r = Response::Tool2;
                break;
            case ToolBox::DialogResult::Tool3:
                r = Response::Tool3;
                break;
            case ToolBox::DialogResult::Tool4:
                r = Response::Tool4;
                break;
            case ToolBox::DialogResult::Tool5:
                r = Response::Tool5;
                break;
            case ToolBox::DialogResult::Unknown:
            case ToolBox::DialogResult::Park:
            case ToolBox::DialogResult::Return:
                [[fallthrough]];
            default:
                r = Response::Continue;
                break;
            }
            marlin_client::FSM_response(PhasesColdPull::select_tool, r);
        }
    };

    class PickTool final : public TextFrame {
    public:
        PickTool(window_t *parent)
            : TextFrame(parent, _(text_title)) {
        }

        static constexpr const char *text_title = N_("Please wait");
    };
#endif

#if HAS_MMU2()
    class StopMMU final : public TextFrame {
    public:
        explicit StopMMU(window_t *parent)
            : TextFrame(parent, _(text_title), _(text_info)) {}

        static constexpr const char *text_title = N_("Stopping MMU");
        static constexpr const char *text_info = "";
    };

    class Cleanup final : public TextFrame {
    public:
        explicit Cleanup(window_t *parent)
            : TextFrame(parent, _(text_title), _(text_info)) {}

        static constexpr const char *text_title = N_("Restarting MMU");
        static constexpr const char *text_info = "";
    };

#else
    using Cleanup = common_frames::Blank;
#endif

#if HAS_TOOLCHANGER() || HAS_MMU2()
    class UnloadFilamentPtfe final : public TextFrame {
    public:
        explicit UnloadFilamentPtfe(window_t *parent)
            : TextFrame(parent, _(text_title), _(text_info)) {}

        static constexpr const char *text_title = N_("Unload filament");
        static constexpr const char *text_info = N_("Before you continue, unload the filament. Then press down the blue part on the fitting and pull the PTFE tube from the tool head.");
    };

    class LoadFilamentPtfe final : public TextFrame {
    public:
        explicit LoadFilamentPtfe(window_t *parent)
            : TextFrame(parent, _(text_title), _(text_info)) {}

        static constexpr const char *text_title = N_("Load filament");
        static constexpr const char *text_info = N_("Before you continue, make sure PLA filament is loaded directly into the extruder.");
    };
#endif

    class PrepareFilament final : public TextFrame {
    public:
        explicit PrepareFilament(window_t *parent)
            : TextFrame(parent, _(text_title), _(text_info)) {}

        static constexpr const char *text_title = N_("Filament check");
        static constexpr const char *text_info = N_("Before you continue,\nmake sure that PLA filament is loaded.");
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
    static_assert(common_frames::is_update_callable<CoolDown>);

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
    static_assert(common_frames::is_update_callable<HeatUp>);

    class AutomaticPull final : public TextFrame {
    public:
        explicit AutomaticPull(window_t *parent)
            : TextFrame(parent, _(text1)) {
            Sound_Play(eSOUND_TYPE::SingleBeep);
        }

        static constexpr const char *text1 = N_("Unloading");
    };

    class ManualPull final : public TextFrame {
    public:
        explicit ManualPull(window_t *parent)
            : TextFrame(parent, _(text1), _(text2)) {
        }

        static constexpr const char *text1 = N_("Remove the filament manually");
        static constexpr const char *text2 = N_("There might be a slight resistance.\nIf the filament is stuck, open the idler lever.");
    };

    class PullDone final : public TextFrame {
    public:
        explicit PullDone(window_t *parent)
            : TextFrame(parent, _(text1), _(text2)) {
        }

        static constexpr const char *text1 = N_("Cold Pull successfully completed");
        static constexpr const char *text2 = N_("You can continue printing. If the issue persists,\nrepeat this procedure again.");

        // for XL only, enabled now because of translations
        [[maybe_unused]] static constexpr const char *TODOtext10 = N_("Cold Pull successfully completed. Insert PTFE tube back in the fitting. You can continue printing. If the issue persists, repeat this procedure again.");
    };

} // namespace frame

// Blank screen is needed to avoid short flicker of the lower screen when switching from Load filament dialog
// to FramePreheat. There is short but noticable period where the underlaying screen is visible before
// switch do next happens. If it's black it looks nice.
// It goes from FrameLoadUnload -> FrameBlank -> Load-dialog -> FrameBlank -> FramePreheat.
using Frames = FrameDefinitionList<ScreenColdPull::FrameStorage,
    FrameDefinition<PhasesColdPull::introduction, frame::Introduction>,
#if HAS_TOOLCHANGER()
    FrameDefinition<PhasesColdPull::select_tool, frame::SelectTool>,
    FrameDefinition<PhasesColdPull::pick_tool, frame::PickTool>,
#endif
#if HAS_MMU2()
    FrameDefinition<PhasesColdPull::stop_mmu, frame::StopMMU>,
#endif
#if HAS_TOOLCHANGER() || HAS_MMU2()
    FrameDefinition<PhasesColdPull::unload_ptfe, frame::UnloadFilamentPtfe>,
    FrameDefinition<PhasesColdPull::load_ptfe, frame::LoadFilamentPtfe>,
#endif
    FrameDefinition<PhasesColdPull::prepare_filament, frame::PrepareFilament>,
    FrameDefinition<PhasesColdPull::blank_load, common_frames::Blank>,
    FrameDefinition<PhasesColdPull::blank_unload, common_frames::Blank>,
    FrameDefinition<PhasesColdPull::cool_down, frame::CoolDown>,
    FrameDefinition<PhasesColdPull::heat_up, frame::HeatUp>,
    FrameDefinition<PhasesColdPull::automatic_pull, frame::AutomaticPull>,
    FrameDefinition<PhasesColdPull::manual_pull, frame::ManualPull>,
    FrameDefinition<PhasesColdPull::cleanup, frame::Cleanup>,
    FrameDefinition<PhasesColdPull::pull_done, frame::PullDone>>;

} // namespace

ScreenColdPull::ScreenColdPull()
    : ScreenFSM(text_header, ScreenColdPull::get_inner_frame_rect())
    , radio { this, GuiDefaults::GetButtonRect(GuiDefaults::RectScreenBody), PhasesColdPull::introduction }
    , footer { this, 0, footer::Item::nozzle, footer::Item::bed, footer::Item::heatbreak_temp } {
    CaptureNormalWindow(radio);
    create_frame();
}

ScreenColdPull::~ScreenColdPull() {
    destroy_frame();
}

void ScreenColdPull::create_frame() {
    Frames::create_frame(frame_storage, get_phase(), &inner_frame);
    radio.Change(get_phase());
}

void ScreenColdPull::destroy_frame() {
    Frames::destroy_frame(frame_storage, get_phase());
}

void ScreenColdPull::update_frame() {
    Frames::update_frame(frame_storage, get_phase(), fsm_base_data.GetData());
}
