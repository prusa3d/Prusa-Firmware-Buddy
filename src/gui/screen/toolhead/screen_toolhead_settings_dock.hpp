#pragma once

#include "screen_toolhead_settings_common.hpp"

namespace screen_toolhead_settings {

class MI_DOCK_X : public MI_TOOLHEAD_SPECIFIC_SPIN<MI_DOCK_X> {
public:
    MI_DOCK_X(Toolhead toolhead = default_toolhead);
    static float read_value_impl(ToolheadIndex ix);
    static void store_value_impl(ToolheadIndex ix, float set);
};

class MI_DOCK_Y : public MI_TOOLHEAD_SPECIFIC_SPIN<MI_DOCK_Y> {
public:
    MI_DOCK_Y(Toolhead toolhead = default_toolhead);
    static float read_value_impl(ToolheadIndex ix);
    static void store_value_impl(ToolheadIndex ix, float set);
};

class MI_DOCK_CALIBRATE : public MI_TOOLHEAD_SPECIFIC<MI_DOCK_CALIBRATE, IWindowMenuItem> {
public:
    MI_DOCK_CALIBRATE(Toolhead toolhead = default_toolhead);
    void click(IWindowMenu &);
};

using ScreenToolheadDetailDock_ = ScreenMenu<EFooter::Off,
    MI_RETURN,
    MI_DOCK_X,
    MI_DOCK_Y,
    MI_DOCK_CALIBRATE //
    >;

class ScreenToolheadDetailDock : public ScreenToolheadDetailDock_ {
public:
    ScreenToolheadDetailDock(Toolhead toolhead = default_toolhead);

private:
    const Toolhead toolhead;
};

} // namespace screen_toolhead_settings
