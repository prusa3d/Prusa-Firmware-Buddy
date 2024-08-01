#include "auto_layout.hpp"

void layout_vertical_stack(const Rect16 &rect, const std::initializer_list<window_t *> &windows, const std::initializer_list<StackLayoutItem> &items) {
    assert(windows.size() == items.size());

    int minimum_size = 0;
    float remaining_stretch_ratio_sum = 0;

    // First pass - collect information
    for (const auto &item : items) {
        minimum_size += item.margin_top;

        std::visit([&]<typename T>(T val) {
            if constexpr (std::is_same_v<T, StackLayoutItem::Stretch>) {
                remaining_stretch_ratio_sum += val.ratio;
            } else {
                minimum_size += val;
            }
        },
            item.height);

        minimum_size += item.margin_bottom;
    }

    assert(rect.Height() >= minimum_size);
    int remaining_stretch_space = rect.Height() - minimum_size;

    // Second pass - lay out
    int y = rect.Top();
    auto window_it = windows.begin();
    for (const auto &item : items) {
        y += item.margin_top;

        const int item_height = std::visit([&]<typename T>(T val) -> int {
            if constexpr (std::is_same_v<T, StackLayoutItem::Stretch>) {
                const int result = remaining_stretch_space * val.ratio / remaining_stretch_ratio_sum;
                remaining_stretch_ratio_sum -= val.ratio;
                remaining_stretch_space -= result;
                return result;

            } else {
                return val;
            }
        },
            item.height);

        if (auto window = *window_it) {
            const int available_width = rect.Width() - item.margin_side * 2;
            const int item_width = std::visit([&]<typename T>(T val) -> int {
                if constexpr (std::is_same_v<T, StackLayoutItem::Stretch>) {
                    return available_width;
                } else {
                    return val;
                }
            },
                item.width);

            const int distance_from_rect_side = item.margin_side + (available_width - item_width) / 2;
            window->SetRect(Rect16::fromLTRB(rect.Left() + distance_from_rect_side, y, rect.Right() - distance_from_rect_side, y + item_height));
        }

        y += item_height + item.margin_bottom;
        window_it++;
    }
}
