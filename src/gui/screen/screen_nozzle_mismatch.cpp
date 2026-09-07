/// @file
#include "screen_nozzle_mismatch.hpp"

#include <fsm/nozzle_mismatch_phases.hpp>
#include <fsm/nozzle_mismatch_mapper.hpp>
#include <gui/standard_frame/frame_prompt.hpp>
#include <gui/standard_frame/frame_qr_prompt.hpp>
#include <gui/standard_frame/frame_wait.hpp>
#include <window_menu_adv.hpp>
#include <window_menu_virtual.hpp>
#include <i_window_menu_item.hpp>
#include <marlin_client.hpp>
#include <tool_index.hpp>
#include <array>

using Phase = PhaseNozzleMismatch;

namespace {

class MI_DOCK : public IWindowMenuItem {
public:
    MI_DOCK(uint8_t dock_idx, Phase phase)
        : dock_idx_(dock_idx)
        , phase_(phase) {
        SetLabel(_(label).formatted(params, dock_idx + 1));
    }

protected:
    void click(IWindowMenu &) override {
        marlin_client::FSM_response_variant(phase_, FSMResponseVariant::make<uint8_t>(dock_idx_));
    }

private:
    static constexpr const char *label = N_("Dock %u");

    StringViewUtf8Parameters<2> params;
    uint8_t dock_idx_;
    Phase phase_;
};

class DockMenu : public WindowMenuVirtual {

public:
    DockMenu(window_frame_t *parent, Rect16 rect, Phase phase)
        : WindowMenuVirtual(parent, rect, CloseScreenReturnBehavior::no)
        , phase_(phase) {
        auto it = PhysicalToolIndex::all().skip_all_disabled();
        if (it.at_end()) {
            // Fallback for the rare case (e.g. after a factory reset) where a
            // tool is in the head but no dock is calibrated yet — at least
            // offer all docks so the user can park it somewhere.
            it = PhysicalToolIndex::all();
        }
        for (; !it.at_end(); ++it) {
            enabled_indices_[count_++] = (*it).to_raw();
        }
        setup_items();
    }

    int item_count() const final {
        return count_;
    }

    void setup_item(ItemVariant &variant, int index) final {
        variant.emplace<MI_DOCK>(enabled_indices_[index], phase_);
    }

private:
    Phase phase_;
    std::array<uint8_t, PhysicalToolIndex::count> enabled_indices_;
    uint8_t count_ = 0;
};

class FrameDockSelection {
public:
    FrameDockSelection(window_frame_t *parent, Phase phase)
        : menu(parent, GuiDefaults::RectScreenNoHeader, phase) {
        parent->CaptureNormalWindow(menu);
    }

    WindowExtendedMenu<DockMenu> menu;
};

class FrameParking : public FrameWait {
public:
    FrameParking(window_frame_t *parent, [[maybe_unused]] Phase phase)
        : FrameWait(parent, N_("Parking to dock...")) {}
};

class FrameHoming : public FrameWait {
public:
    FrameHoming(window_frame_t *parent, [[maybe_unused]] Phase phase)
        : FrameWait(parent, N_("Homing...")) {}
};

} // namespace

using Frames = FrameDefinitionList<ScreenNozzleMismatch::FrameStorage,
    FrameDefinition<Phase::prompt, FramePrompt, nozzle_mismatch_phase_error_code_mapper>,
    FrameDefinition<Phase::dock_selection, FrameDockSelection>,
    FrameDefinition<Phase::parking, FrameParking>,
    FrameDefinition<Phase::dock_not_empty, FrameQRPrompt, nozzle_mismatch_phase_error_code_mapper>,
    FrameDefinition<Phase::tool_lost, FrameQRPrompt, nozzle_mismatch_phase_error_code_mapper>,
    FrameDefinition<Phase::homing, FrameHoming>,
    FrameDefinition<Phase::pickup_failed, FrameQRPrompt, nozzle_mismatch_phase_error_code_mapper>,
    FrameDefinition<Phase::park_failed, FrameQRPrompt, nozzle_mismatch_phase_error_code_mapper>,
    FrameDefinition<Phase::confirm_abort, FramePrompt, nozzle_mismatch_phase_error_code_mapper>>;

ScreenNozzleMismatch::ScreenNozzleMismatch()
    : ScreenFSM(N_("TOOL MISMATCH")) {
    create_frame();
}

ScreenNozzleMismatch::~ScreenNozzleMismatch() {
    destroy_frame();
}

void ScreenNozzleMismatch::create_frame() {
    const auto phase = GetEnumFromPhaseIndex<Phase>(fsm_base_data.GetPhase());
    Frames::create_frame(frame_storage, phase, &inner_frame, phase);
}

void ScreenNozzleMismatch::destroy_frame() {
    const auto phase = GetEnumFromPhaseIndex<Phase>(fsm_base_data.GetPhase());
    Frames::destroy_frame(frame_storage, phase);
}

void ScreenNozzleMismatch::update_frame() {
    const auto phase = GetEnumFromPhaseIndex<Phase>(fsm_base_data.GetPhase());
    Frames::update_frame(frame_storage, phase, fsm_base_data.GetData());
}
