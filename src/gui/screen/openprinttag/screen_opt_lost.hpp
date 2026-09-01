
/// @file
#pragma once

#include "screen_opt_prompt.hpp"

#include <gui/standard_frame/frame_prompt.hpp>
#include <feature/openprinttag/tool_tag.hpp>

namespace buddy::openprinttag {

class ScreenOPTLost : public ScreenOPTPrompt {

public:
    ScreenOPTLost(VirtualToolIndex tool);

protected:
    void screenEvent(window_t *sender, GUI_event_t event, void *param) override;

private:
    window_frame_t frame_;
    FramePrompt prompt_;

private:
    const VirtualToolIndex tool_;
    StringViewUtf8Parameters<16> text_params_;
};

} // namespace buddy::openprinttag
