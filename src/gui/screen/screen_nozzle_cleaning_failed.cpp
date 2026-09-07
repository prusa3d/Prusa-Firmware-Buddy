#include "screen_nozzle_cleaning_failed.hpp"
#include <standard_frame/frame_extensions/with_footer.hpp>
#include <footer_line.hpp>
#include <standard_frame/frame_progress_prompt.hpp>
#include <find_error.hpp>
#include <standard_frame/frame_qr_prompt.hpp>
#include <fsm/nozzle_cleaning_failed_phases.hpp>
#include <standard_frame/frame_prompt.hpp>
#include <standard_frame/frame_prompt.hpp>
#include <img_resources.hpp>
#include <fsm/nozzle_cleaning_failed_mapper.hpp>

using Phase = PhaseNozzleCleaningFailed;

namespace {
constexpr auto txt_desc_cleaning_failed = N_("\nNozzle cleaning failed.");
constexpr auto warning_suffix = "nozzle-cleaning-failed"_tstr;

class FrameNozzleCleaningProgress : public FrameProgressPrompt {
public:
    FrameNozzleCleaningProgress(window_frame_t *parent, Phase fsm_phase)
        : FrameProgressPrompt(parent, fsm_phase, nozzle_cleaning_failed_phase_error_code_mapper) {
    }

    void update(const fsm::PhaseData &data_) {
        const auto data = fsm::deserialize_data<NozzleCleaningFailedProgressData>(data_);
        progress_bar.set_progress_percent(static_cast<float>(data.progress_0_255) / 255.0f * 100.0f);
    }
};

constexpr FooterLine::IdArray nozzle_cleaning_footer_items = { footer::Item::nozzle, footer::Item::bed };

using Frames = FrameDefinitionList<ScreenNozzleCleaningFailed::FrameStorage,
    FrameDefinition<Phase::cleaning_failed, FrameQRPrompt, Phase::cleaning_failed, txt_desc_cleaning_failed, warning_suffix>,
#if HAS_NOZZLE_CLEANING_FAILED_PURGING()
    FrameDefinition<Phase::recommend_purge, WithFooter<FramePrompt, nozzle_cleaning_footer_items>, Phase::recommend_purge, nozzle_cleaning_failed_phase_error_code_mapper>,
    FrameDefinition<Phase::wait_temp, WithFooter<FrameNozzleCleaningProgress, nozzle_cleaning_footer_items>, Phase::wait_temp>,
    FrameDefinition<Phase::purge, WithFooter<FrameNozzleCleaningProgress, nozzle_cleaning_footer_items>, Phase::purge>,
    FrameDefinition<Phase::autoretract, WithFooter<FrameNozzleCleaningProgress, nozzle_cleaning_footer_items>, Phase::autoretract>,
    FrameDefinition<Phase::remove_filament, WithFooter<FramePrompt, nozzle_cleaning_footer_items>, Phase::remove_filament, nozzle_cleaning_failed_phase_error_code_mapper>,
#endif
#if HAS_SWITCHABLE_AUTO_RETRACT()
    FrameDefinition<Phase::offer_auto_retract_enable, FramePrompt, Phase::offer_auto_retract_enable, nozzle_cleaning_failed_phase_error_code_mapper>,
#endif
    FrameDefinition<Phase::warn_abort, WithFooter<FramePrompt, nozzle_cleaning_footer_items>, Phase::warn_abort, nozzle_cleaning_failed_phase_error_code_mapper>>;

} // namespace

ScreenNozzleCleaningFailed::ScreenNozzleCleaningFailed()
    : ScreenFSM(N_("WARNING"), GuiDefaults::RectScreenNoHeader) {
    header.SetIcon(&img::warning_16x16);
    create_frame();
}

ScreenNozzleCleaningFailed::~ScreenNozzleCleaningFailed() {
    destroy_frame();
}

void ScreenNozzleCleaningFailed::create_frame() {
    Frames::create_frame(frame_storage, get_phase(), &inner_frame);
}

void ScreenNozzleCleaningFailed::destroy_frame() {
    Frames::destroy_frame(frame_storage, get_phase());
}

void ScreenNozzleCleaningFailed::update_frame() {
    Frames::update_frame(frame_storage, get_phase(), fsm_base_data.GetData());
}
