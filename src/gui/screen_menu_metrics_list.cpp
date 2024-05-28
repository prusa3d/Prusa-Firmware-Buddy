#include "screen_menu_metrics_list.hpp"

#include <sound.hpp>
#include <metric_handlers.h>
#include <ScreenHandler.hpp>
#include <str_utils.hpp>

using namespace screen_menu_metrics_list;

MI_METRIC::MI_METRIC(metric_t *metric)
    : WI_ICON_SWITCH_OFF_ON_t(is_metric_enabled_for_handler(metric, &metric_handler_syslog), {})
    , metric_(metric) //
{
    StringBuilder sb(label_);
    sb.append_printf("%3i %s", metric - metric_get_iterator_begin() + 1, metric->name);
    SetLabel(string_view_utf8::MakeRAM(label_.data()));
}

void MI_METRIC::OnChange(size_t) {
    (index ? metric_enable_for_handler : metric_disable_for_handler)(metric_, &metric_handler_syslog);
}

WindowMenuMetricsList::WindowMenuMetricsList(window_t *parent, Rect16 rect)
    : WindowMenuVirtual(parent, rect) {
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

void screen_menu_metrics_list::ScreenMenuMetricsList::screenEvent(window_t *sender, GUI_event_t event, void *param) {
    switch (event) {

    case GUI_event_t::TOUCH_SWIPE_LEFT:
    case GUI_event_t::TOUCH_SWIPE_RIGHT:
        Sound_Play(eSOUND_TYPE::ButtonEcho);
        Screens::Access()->Close();
        return;

    default:
        break;
    }

    ScreenMenuBase::screenEvent(sender, event, param);
}
