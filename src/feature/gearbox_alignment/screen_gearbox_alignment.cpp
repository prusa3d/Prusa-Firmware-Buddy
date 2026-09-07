/// @file
#include "screen_gearbox_alignment.hpp"

#include <fsm/gearbox_alignment_phases.hpp>
#include <i18n.h>
#include <img_resources.hpp>
#include <guiconfig/wizard_config.hpp>
#include <gui/standard_frame/frame_calibration_text.hpp>
#include <gui/standard_frame/frame_calibration_text_with_image.hpp>
#include <gui/standard_frame/frame_prompt.hpp>

static ScreenGearboxAlignment *instance = nullptr;

static const char *text_header = N_("GEARBOX ALIGNMENT");
static constexpr size_t content_top_y = WizardDefaults::row_1 + WizardDefaults::progress_row_h;

class FrameIntro final : public FramePrompt {
public:
    FrameIntro(window_frame_t *parent)
        : FramePrompt {
            parent,
            PhaseGearboxAlignment::intro,
            _("Gearbox alignment"),
            _("The gearbox alignment is only necessary for user-assembled or serviced gearboxes. In all other cases, you can skip this step."),
        } {
    }

#if HAS_TOOLCHANGER()
    void update(const fsm::PhaseData &raw) {
        const auto data = fsm::deserialize_data<FSMGearboxAlignmentData>(raw);
        title.SetText(_("Tool %i gearbox alignment").formatted(title_params_, PhysicalToolIndex::from_raw(data.physical_tool_index).display_index()));
    }

private:
    StringViewUtf8Parameters<4> title_params_;
#endif
};

class FrameFilamentLoadedAskUnload final : public FrameCalibrationText {
public:
    FrameFilamentLoadedAskUnload(window_frame_t *parent)
        : FrameCalibrationText {
            parent,
            PhaseGearboxAlignment::filament_loaded_ask_unload,
            _("We need to start without the filament in the extruder. Please unload it."),
            content_top_y,
        } {}
};

class FrameFilamentUnknownAskUnload final : public FrameCalibrationText {
public:
    FrameFilamentUnknownAskUnload(window_frame_t *parent)
        : FrameCalibrationText {
            parent,
            PhaseGearboxAlignment::filament_unknown_ask_unload,
            _("Before you proceed, make sure filament is unloaded from the Nextruder."),
            content_top_y,
        } {}
};

class FrameLoosenScrews final : public FrameCalibrationTextWithImage {
public:
    FrameLoosenScrews(window_frame_t *parent)
        : FrameCalibrationTextWithImage {
            parent,
            PhaseGearboxAlignment::loosen_screws,
            _("Rotate each screw counter-clockwise by 1.5 turns. The screw heads should be flush with the cover. Unlock and open the idler."),
            content_top_y,
            &img::transmission_loose_187x175,
            187,
        } {}
};

class FrameAlignment final : public FrameCalibrationTextWithImage {
public:
    FrameAlignment(window_frame_t *parent)
        : FrameCalibrationTextWithImage {
            parent,
            PhaseGearboxAlignment::alignment,
            _("Gearbox alignment in progress, please wait (approx. 20 seconds)"),
            content_top_y,
            &img::transmission_gears_187x175,
            187,
        } {}
};

class FrameTightenScrews final : public FrameCalibrationTextWithImage {
public:
    FrameTightenScrews(window_frame_t *parent)
        : FrameCalibrationTextWithImage {
            parent,
            PhaseGearboxAlignment::tighten_screws,
            _("Tighten the M3 screws firmly in the correct order, they should be slightly below the surface. Do not over-tighten."),
            content_top_y,
            &img::transmission_tight_187x175,
            187,
        } {}
};

class FrameDone final : public FrameCalibrationTextWithImage {
public:
    FrameDone(window_frame_t *parent)
        : FrameCalibrationTextWithImage {
            parent,
            PhaseGearboxAlignment::done,
            _("Close the idler door and secure it with the swivel. The calibration is done!"),
            content_top_y,
            &img::transmission_close_187x175,
            187,
        } {}
};

using Frames = FrameDefinitionList<ScreenGearboxAlignment::FrameStorage,
    FrameDefinition<PhaseGearboxAlignment::intro, FrameIntro>,
    FrameDefinition<PhaseGearboxAlignment::filament_loaded_ask_unload, FrameFilamentLoadedAskUnload>,
    FrameDefinition<PhaseGearboxAlignment::filament_unknown_ask_unload, FrameFilamentUnknownAskUnload>,
    FrameDefinition<PhaseGearboxAlignment::loosen_screws, FrameLoosenScrews>,
    FrameDefinition<PhaseGearboxAlignment::alignment, FrameAlignment>,
    FrameDefinition<PhaseGearboxAlignment::tighten_screws, FrameTightenScrews>,
    FrameDefinition<PhaseGearboxAlignment::done, FrameDone>>;

static PhaseGearboxAlignment get_phase(const fsm::BaseData &fsm_base_data) {
    return GetEnumFromPhaseIndex<PhaseGearboxAlignment>(fsm_base_data.GetPhase());
}

ScreenGearboxAlignment::ScreenGearboxAlignment()
    : ScreenFSM { text_header, rect_screen } {

    create_frame();
    instance = this;
}

ScreenGearboxAlignment::~ScreenGearboxAlignment() {
    instance = nullptr;
    destroy_frame();
}

ScreenGearboxAlignment *ScreenGearboxAlignment::GetInstance() {
    return instance;
}

void ScreenGearboxAlignment::create_frame() {
    Frames::create_frame(frame_storage, get_phase(fsm_base_data), &inner_frame);
}

void ScreenGearboxAlignment::destroy_frame() {
    Frames::destroy_frame(frame_storage, get_phase(fsm_base_data));
}

void ScreenGearboxAlignment::update_frame() {
    Frames::update_frame(frame_storage, get_phase(fsm_base_data), fsm_base_data.GetData());
}
