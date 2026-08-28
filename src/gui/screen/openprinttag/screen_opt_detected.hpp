
/// @file
#pragma once

#include "screen_opt_prompt.hpp"

#include <gui/standard_frame/frame_opt_prompt.hpp>
#include <feature/openprinttag/tool_tag.hpp>

namespace buddy::openprinttag {

class ScreenOPTDetected : public ScreenOPTPrompt {

public:
    ScreenOPTDetected(ToolTag tag);
    ~ScreenOPTDetected();

private:
    FrameOPTPrompt frame_;

private:
    const ToolTag tag_;
};

} // namespace buddy::openprinttag
