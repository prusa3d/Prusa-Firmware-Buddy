#include "screen_print_preview.hpp"
#include <standard_frame/frame_wait.hpp>
#include <standard_frame/frame_prompt.hpp>
#include <guiconfig/guiconfig.h>
#include <gcode_description.hpp>
#include <window_thumbnail.hpp>
#include <window_text.hpp>
#include <window_roll_text.hpp>
#include <img_resources.hpp>
#include <gcode/gcode_info.hpp>
#include <fsm/print_preview_mapper.hpp>
#include <gui/screen/print/frame_compat_checks.hpp>
#include <sound.hpp>
#include <meta_utils.hpp>

#if HAS_LARGE_DISPLAY()
    #include <dialogs/resolution_480x320/radio_button_preview.hpp>
#elif HAS_MINI_DISPLAY()
    #include <dialogs/resolution_240x320/radio_button_preview.hpp>
#else
    #error
#endif

#include <option/has_mmu2.h>
#include <option/has_tool_mapping.h>
#include <option/has_e2ee_support.h>
#include <option/has_wastebin_fill_tracking.h>
#if HAS_WASTEBIN_FILL_TRACKING()
    #include <feature/wastebin_watcher/wastebin_watcher.hpp>
#endif

#include <static_alocation_ptr.hpp>
#include <bsod/bsod.h>
#if HAS_TOOL_MAPPING()
    #include <gui/screen/print/frame_tool_mapping.hpp>
#endif

#if HAS_MMU2()
    #include <feature/prusa/MMU2/mmu2_mk4.h>
#endif

using Phase = PhasesPrintPreview;

namespace {
const auto text_loading = N_("Loading...");
const auto text_downloading = N_("Downloading...");
const auto text_header_print = N_("PRINT");

#if HAS_LARGE_DISPLAY()
static constexpr Rect16 title_rect {
    GuiDefaults::PreviewThumbnailRect.Left(),
    GuiDefaults::HeaderHeight + 8,
    GuiDefaults::ScreenWidth - 2 * GuiDefaults::PreviewThumbnailRect.Left(),
    TITLE_HEIGHT
};

static constexpr Rect16 buttons_rect {
    GuiDefaults::PreviewThumbnailRect.Right() + (GuiDefaults::ScreenWidth - GuiDefaults::PreviewThumbnailRect.Right() - RadioButtonPreview::vertical_buttons_width) / 2,
    GuiDefaults::PreviewThumbnailRect.Top(),
    RadioButtonPreview::vertical_buttons_width,
    GuiDefaults::ScreenHeight - GuiDefaults::PreviewThumbnailRect.Top()
};

#else
static constexpr Rect16 title_rect { PADDING, PADDING, GuiDefaults::ScreenWidth - 2 * PADDING, TITLE_HEIGHT };
static constexpr Rect16 buttons_rect { GuiDefaults::GetButtonRect(GuiDefaults::RectScreen) };
#endif

} // namespace

namespace frames {

class FrameThumbnailPreview {
public:
    FrameThumbnailPreview(window_frame_t *parent)
        : title(parent, title_rect)
        , line(parent, Rect16(title_rect.Left(), title_rect.Bottom(), title_rect.Width(), 2))
        , thumbnail(parent, GuiDefaults::PreviewThumbnailRect)
        , radio(parent, buttons_rect)
        , gcode_description(parent) {

        debug_assert(GCodeInfo::getInstance().is_loaded() && "GCodeInfo must be initialized before ScreenPrintPreview is created");
        radio.SetBtnCount(2);
        line.SetBackColor(COLOR_DARK_GRAY);
        parent->CaptureNormalWindow(radio);
        //  this MakeRAM is safe - gcode_file_name is set to vars->media_LFN, which is statically allocated in RAM
        title.SetText(string_view_utf8::MakeRAM(GCodeInfo::getInstance().GetGcodeFilename()));
        gcode_description.update(GCodeInfo::getInstance());
    }

private:
    window_roll_text_t title;
    BasicWindow line;
    WindowPreviewThumbnail thumbnail; // draws preview image
    RadioButtonPreview radio;
    GCodeInfoWithDescription gcode_description;
};

#if HAS_E2EE_SUPPORT()
class FrameUntrustedIdentity : FramePrompt {
public:
    FrameUntrustedIdentity(window_frame_t *parent)
        : FramePrompt(parent, Phase::untrusted_identity, map_print_preview_phase_to_error_code) {
        // TODO: The hash does not fit all 64 chars!! Do we need that big of a hash, wouldn't SHA128 be enough?
        // Also the message and options should be different, if the identity is just one time
        info.SetText(info.GetText().formatted(params, GCodeInfo::getInstance().get_identity_info().identity_name.data(), GCodeInfo::getInstance().get_identity_info().key_hash_str.data()));
    }

private:
    StringViewUtf8Parameters<e2ee::IDENTITY_NAME_LEN + e2ee::KEY_HASH_STR_BUFFER_LEN> params;
};
#endif

#if HAS_WASTEBIN_FILL_TRACKING()
class FrameWastebinOverfill : FramePrompt {
public:
    FrameWastebinOverfill(window_frame_t *parent)
        : FramePrompt(parent, Phase::wastebin_overfill_warning,
            _("Nozzle cleaner may overflow"),
            _("This print may overfill the nozzle cleaner.\nNozzle cleaner: %u / %u pellets.\nThis print: %u tool changes.\n\nPrint: start the print anyway.\nIgnore: print without nozzle cleaner checks this print.\nEmpty: empty the nozzle cleaner now.")) {
        info.SetText(info.GetText().formatted(params_,
            WastebinWatcher::instance().fill_level(),
            WastebinWatcher::instance().capacity(),
            static_cast<unsigned>(GCodeInfo::getInstance().get_total_toolchanges().value_or(0))));
    }

private:
    StringViewUtf8Parameters<20> params_;
};
#endif

} // namespace frames

namespace {
using Frames = FrameDefinitionList<ScreenPrintPreview::FrameStorage,
    FrameDefinition<Phase::loading, FrameWait, text_loading>,
    FrameDefinition<Phase::download_wait, FrameWait, text_downloading>,
    FrameDefinition<Phase::main_dialog, frames::FrameThumbnailPreview>,
    FrameDefinition<Phase::unfinished_selftest, FramePrompt, Phase::unfinished_selftest, map_print_preview_phase_to_error_code>,
    FrameDefinition<Phase::new_firmware_available, FramePrompt, Phase::new_firmware_available, map_print_preview_phase_to_error_code>,
    FrameDefinition<Phase::gcode_incompatible_warning, screen_print_preview::FrameCompatibilityChecks, Phase::gcode_incompatible_warning>,
    FrameDefinition<Phase::gcode_incompatible_fatal, screen_print_preview::FrameCompatibilityChecks, Phase::gcode_incompatible_fatal>,
    FrameDefinition<Phase::filament_incompatible_warning, screen_print_preview::FrameCompatibilityChecks, Phase::filament_incompatible_warning>,
    FrameDefinition<Phase::filament_incompatible_fatal, screen_print_preview::FrameCompatibilityChecks, Phase::filament_incompatible_fatal>,
    FrameDefinition<Phase::filament_not_inserted, FramePrompt, Phase::filament_not_inserted, map_print_preview_phase_to_error_code>,
#if HAS_MMU2()
    FrameDefinition<Phase::mmu_filament_inserted, FramePrompt, Phase::mmu_filament_inserted, map_print_preview_phase_to_error_code>,
#endif
#if HAS_TOOL_MAPPING()
    FrameDefinition<Phase::tools_mapping, FrameToolMapping>,
#endif
    FrameDefinition<Phase::wrong_filament, FramePrompt, Phase::wrong_filament, map_print_preview_phase_to_error_code>,
#if HAS_E2EE_SUPPORT()
    FrameDefinition<Phase::untrusted_identity, frames::FrameUntrustedIdentity>,
#endif
#if HAS_WASTEBIN_FILL_TRACKING()
    FrameDefinition<Phase::wastebin_overfill_warning, frames::FrameWastebinOverfill>,
#endif
    FrameDefinition<Phase::file_error, FramePrompt, Phase::file_error, map_print_preview_phase_to_error_code>>;

} // namespace

ScreenPrintPreview::ScreenPrintPreview()
    : ScreenFSM(text_header_print, GuiDefaults::RectScreenNoHeader) {
    header.SetIcon(&img::print_16x16);
    create_frame();
}

ScreenPrintPreview::~ScreenPrintPreview() {
    destroy_frame();
}

void ScreenPrintPreview::create_frame() {
    Frames::create_frame(frame_storage, get_phase(), &inner_frame);
#if HAS_TOOL_MAPPING()
    if (get_phase() == Phase::tools_mapping) {
        header.SetText(_("FILAMENT MAPPING"));
        header.set_show_bed_info(true);
    } else {
        header.SetText(_(text_header_print));
        header.set_show_bed_info(false);
    }
#endif
}

void ScreenPrintPreview::destroy_frame() {
    Frames::destroy_frame(frame_storage, get_phase());
}

void ScreenPrintPreview::update_frame() {
    Frames::update_frame(frame_storage, get_phase(), fsm_base_data.GetData());
}

void ScreenPrintPreview::windowEvent([[maybe_unused]] window_t *sender, GUI_event_t event, void *param) {
    switch (event) {

    // Catch event when USB is removed
    case GUI_event_t::MEDIA: {
        const MediaState_t media_state = MediaState_t(reinterpret_cast<int>(param));
        if (media_state == MediaState_t::removed || media_state == MediaState_t::error) {
            marlin_client::print_abort(); // Abort print from marlin_server and close printing screens
        }
        break;
    }

    // Swipe left/right during preview phase -> go back
    case GUI_event_t::TOUCH_SWIPE_LEFT:
    case GUI_event_t::TOUCH_SWIPE_RIGHT: {
        if (get_phase() == PhasesPrintPreview::main_dialog) {
            sound::play(SoundType::button_echo);
            marlin_client::FSM_response(PhasesPrintPreview::main_dialog, Response::Back);
        }
    }

    default:
        break;
    }
}
