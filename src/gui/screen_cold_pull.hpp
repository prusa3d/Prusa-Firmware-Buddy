#pragma once

#include "screen_fsm.hpp"
#include <footer_line.hpp>
#include "radio_button_fsm.hpp"

class ScreenColdPull final : public ScreenFSM {
private:
    RadioButtonFSM radio;
    FooterLine footer;

public:
    ScreenColdPull();
    ~ScreenColdPull();

    static constexpr Rect16 get_inner_frame_rect() {
        return GuiDefaults::RectScreenBody - GuiDefaults::GetButtonRect(GuiDefaults::RectScreenBody).Height() - static_cast<Rect16::Height_t>(GuiDefaults::FramePadding);
    }

protected:
    PhasesColdPull get_phase() const {
        return GetEnumFromPhaseIndex<PhasesColdPull>(fsm_base_data.GetPhase());
    }

    virtual void create_frame() override;
    virtual void destroy_frame() override;
    virtual void update_frame() override;
};
