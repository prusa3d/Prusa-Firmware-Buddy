#pragma once

#include <screen_menu.hpp>
#include <window_menu_virtual.hpp>
#include <WindowMenuItems.hpp>
#include <metric.h>

namespace screen_menu_metrics_list {

class MI_METRIC final : public WI_ICON_SWITCH_OFF_ON_t {

public:
    MI_METRIC(metric_t *metric);

protected:
    void OnChange(size_t) final;

private:
    std::array<char, 32> label_;
    metric_t *const metric_;
};

class WindowMenuMetricsList final : public WindowMenuVirtual<MI_RETURN, MI_METRIC> {

public:
    WindowMenuMetricsList(window_t *parent, Rect16 rect);

public:
    int item_count() const final;

protected:
    void setup_item(ItemVariant &variant, int index) final;
};

class ScreenMenuMetricsList : public ScreenMenuBase<WindowMenuMetricsList> {
public:
    ScreenMenuMetricsList();
};

} // namespace screen_menu_metrics_list

using ScreenMenuMetricsList = screen_menu_metrics_list::ScreenMenuMetricsList;
