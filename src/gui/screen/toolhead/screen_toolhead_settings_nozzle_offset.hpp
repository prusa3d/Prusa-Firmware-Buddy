#pragma once

#include "screen_toolhead_settings_common.hpp"

namespace screen_toolhead_settings {

class MI_NOZZLE_OFFSET_COMPONENT : public MI_TOOLHEAD_SPECIFIC_SPIN<MI_NOZZLE_OFFSET_COMPONENT> {
public:
    MI_NOZZLE_OFFSET_COMPONENT(uint8_t component, Toolhead toolhead = default_toolhead);
    float read_value_impl(ToolheadIndex ix);
    void store_value_impl(ToolheadIndex ix, float set);

private:
    const uint8_t component_;
    StringViewUtf8Parameters<2> label_params_;
};

using ScreenToolheadDetailNozzleOffset_ = ScreenMenu<EFooter::Off,
    MI_RETURN,
    WithConstructorArgs<MI_NOZZLE_OFFSET_COMPONENT, 0>,
    WithConstructorArgs<MI_NOZZLE_OFFSET_COMPONENT, 1>,
    WithConstructorArgs<MI_NOZZLE_OFFSET_COMPONENT, 2> //
    >;

class ScreenToolheadDetailNozzleOffset : public ScreenToolheadDetailNozzleOffset_ {
public:
    ScreenToolheadDetailNozzleOffset(Toolhead toolhead = default_toolhead);

private:
    const Toolhead toolhead;
};

} // namespace screen_toolhead_settings
