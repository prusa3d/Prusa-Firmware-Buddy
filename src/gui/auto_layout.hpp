#pragma once

#include <variant>

#include <window.hpp>
#include <guiconfig/GuiDefaults.hpp>

struct StackLayoutItem {

public:
    struct Stretch {
        /// Size ratio of the stretching items (only applies to height)
        float ratio = 1;
    };
    static constexpr Stretch stretch { 1 };

public:
    std::variant<int, Stretch> height;

    /// If set, the width will be fixed to the given size and the item will be horizontally centered
    std::variant<int, Stretch> width = stretch;

    int16_t margin_side = 0;

    int16_t margin_top = 0;
    int16_t margin_bottom = 0;
};

/// Automatically lays out the windows in a vertical stack, based on the provided configuration
void layout_vertical_stack(const Rect16 &rect, const std::initializer_list<window_t *> &windows, const std::initializer_list<StackLayoutItem> &items);

namespace standard_stack_layout {

static constexpr StackLayoutItem for_radio {
    .height = GuiDefaults::ButtonHeight,
    .margin_side = GuiDefaults::ButtonSpacing,
    .margin_bottom = GuiDefaults::FramePadding,
};

static constexpr StackLayoutItem for_progress_bar {
    .height = 8,
    .margin_side = 32,
    .margin_top = 12,
    .margin_bottom = 8,
};

} // namespace standard_stack_layout
