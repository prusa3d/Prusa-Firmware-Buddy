#pragma once

#include "IDialogMarlin.hpp"
#include <warning_type.hpp>
#include <gui/standard_frame/frame_icon_qr_prompt.hpp>
#include <optional>

static_assert(sizeof(fsm::PhaseData) == sizeof(WarningType), "If this does not hold, we need to revise how we send the type through teh fsm machinery.");
class DialogWarning : public IDialogMarlin {
    std::optional<FrameIconQRPrompt> frame_;

public:
    DialogWarning(fsm::BaseData);

    void Change(fsm::BaseData data) override;
};
