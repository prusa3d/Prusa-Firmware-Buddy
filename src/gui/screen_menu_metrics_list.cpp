#include "screen_menu_metrics_list.hpp"

#include <sound.hpp>
#include <metric_handlers.h>
#include <ScreenHandler.hpp>
#include <str_utils.hpp>

using namespace screen_menu_metrics_list;

MI_METRIC::MI_METRIC(metric_t *metric)
    : WI_ICON_SWITCH_OFF_ON_t(metric->enabled, {})
    , metric_(metric) //
{
    StringBuilder sb(label_);
    sb.append_printf("%3i %s", metric - metric_get_iterator_begin() + 1, metric->name);
    SetLabel(string_view_utf8::MakeRAM(label_.data()));
}

void MI_METRIC::OnChange(size_t) {
    metric_->enabled = index;
}

WindowMenuMetricsList::WindowMenuMetricsList(window_t *parent, Rect16 rect)
    : WindowMenuVirtual(parent, rect, CloseScreenReturnBehavior::yes) {
    setup_items();
}

int WindowMenuMetricsList::item_count() const {
    // +1 for the MI_RETURN
    return metric_get_iterator_end() - metric_get_iterator_begin() + 1;
}

void WindowMenuMetricsList::setup_item(ItemVariant &variant, int index) {
    if (index == 0) {
        variant.emplace<MI_RETURN>();

    } else {
        // -1 because of the MI_RETURN
        variant.emplace<MI_METRIC>(metric_get_iterator_begin() + index - 1);
    }
}

ScreenMenuMetricsList::ScreenMenuMetricsList()
    : ScreenMenuBase(nullptr, _("METRICS LIST"), EFooter::Off) {
}
