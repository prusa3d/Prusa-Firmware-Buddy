#pragma once

#include "screen_toolhead_settings_common.hpp"

#include <option/has_adc_side_fsensor.h>

namespace screen_toolhead_settings {

class MI_FS_SPAN : public MI_TOOLHEAD_SPECIFIC_SPIN<MI_FS_SPAN> {
public:
    MI_FS_SPAN(Toolhead toolhead = default_toolhead);
    static float read_value_impl(ToolheadIndex ix);
    static void store_value_impl(ToolheadIndex ix, float set);
};

class MI_FS_REF : public MI_TOOLHEAD_SPECIFIC_SPIN<MI_FS_REF> {
public:
    MI_FS_REF(Toolhead toolhead = default_toolhead);
    static float read_value_impl(ToolheadIndex ix);
    static void store_value_impl(ToolheadIndex ix, float set);
};

#if HAS_ADC_SIDE_FSENSOR()
class MI_SIDE_FS_SPAN : public MI_TOOLHEAD_SPECIFIC_SPIN<MI_SIDE_FS_SPAN> {
public:
    MI_SIDE_FS_SPAN(Toolhead toolhead = default_toolhead);
    static float read_value_impl(ToolheadIndex ix);
    static void store_value_impl(ToolheadIndex ix, float set);
};

class MI_SIDE_FS_REF : public MI_TOOLHEAD_SPECIFIC_SPIN<MI_SIDE_FS_REF> {
public:
    MI_SIDE_FS_REF(Toolhead toolhead = default_toolhead);
    static float read_value_impl(ToolheadIndex ix);
    static void store_value_impl(ToolheadIndex ix, float set);
};
#endif

using ScreenToolheadDetailFS_ = ScreenMenu<EFooter::Off,
    MI_RETURN,
    MI_FS_SPAN,
    MI_FS_REF
#if HAS_ADC_SIDE_FSENSOR()
    ,
    MI_SIDE_FS_SPAN,
    MI_SIDE_FS_REF
#endif
    >;

class ScreenToolheadDetailFS : public ScreenToolheadDetailFS_ {
public:
    ScreenToolheadDetailFS(Toolhead toolhead = default_toolhead);

private:
    const Toolhead toolhead;
};

} // namespace screen_toolhead_settings
