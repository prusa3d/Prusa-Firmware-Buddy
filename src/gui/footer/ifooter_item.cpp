/**
 * @file ifooter_item.cpp
 * @author Radek Vana
 * @date 2021-03-30
 */

#include "ifooter_item.hpp"
#include "cmath_ext.h"
#include "display_helper.h"

IFooterItem::IFooterItem(window_t *parent, Rect16::W_t width)
    : AddSuperWindow<window_frame_t>(parent, Rect16(0, 0 /* item_top*/, width, item_h))
    , update_period(500)
    , last_updated(gui::GetTick()) {
}

IFooterItem::TickResult IFooterItem::tick() {
    changed_t changed = updateValue();

    if (changed == changed_t::no) {
        return TickResult::unchanged;
    }

    resized_t resized = updateState();

    return resized == resized_t::no ? TickResult::changed : TickResult::changed_and_resized;
}

void IFooterItem::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    switch (event) {
    case GUI_event_t::LOOP: {
        uint16_t now = gui::GetTick(); // must be uint16_t - to match other time variables
        if (uint16_t(now - last_updated) >= update_period) {
            last_updated = now;
            TickResult res = tick();
            if (res == TickResult::changed_and_resized) {
                if (GetParent()) {
                    GetParent()->WindowEvent(this, GUI_event_t::CHILD_CHANGED, nullptr);
                }
            }
        }
    } break;
    case GUI_event_t::REINIT_FOOTER:
        // print format could change - updateState will recreate stringview
        // do not update value (it is not its time)
        // just update state and notify parent if size changed
        if (updateState() == resized_t::yes) {
            if (GetParent()) {
                GetParent()->WindowEvent(this, GUI_event_t::CHILD_CHANGED, nullptr);
            }
        }
        break;
    default:
        break;
    }

    SuperWindowEvent(sender, event, param);
}

Rect16::Width_t IFooterItem::TextWidth(string_view_utf8 text) {
    uint16_t strlen_text = 0;
    const size_ui16_t txt_size = font_meas_text(GuiDefaults::FooterFont, &text, &strlen_text);
    return txt_size.w;
}

IFooterIconText::IFooterIconText(window_t *parent, const img::Resource *icon, Rect16::W_t width)
    : AddSuperWindow<IFooterItem>(parent, width)
    , icon(this, icon)
    , text(this, Rect16::Left_t(icon ? icon->w + GuiDefaults::FooterIconTextSpace : 0)) {
}

Rect16::Width_t IFooterIconText::MeasureTextWidth(string_view_utf8 text) {
    uint16_t strlen_text = 0;
    const size_ui16_t txt_size = font_meas_text(GuiDefaults::FooterFont, &text, &strlen_text);
    return txt_size.w;
}

FooterIconText_IntVal::FooterIconText_IntVal(window_t *parent, const img::Resource *icon,
    view_maker_cb view_maker, reader_cb value_reader)
    : AddSuperWindow<IFooterIconText>(parent, icon, GetTotalWidth(icon ? icon->w : 0, view_maker(value_reader())))
    , makeView(view_maker)
    , readCurrentValue(value_reader)
    , value(value_reader()) {
    text.SetText(makeView(value));
}

Rect16::Width_t FooterIconText_IntVal::GetTotalWidth(Rect16::Width_t icon_w, string_view_utf8 view) {
    return MeasureTextWidth(view) + Rect16::Width_t(icon_w ? icon_w + GuiDefaults::FooterIconTextSpace : 0);
}

changed_t FooterIconText_IntVal::updateValue() {
    changed_t ret = changed_t::no;
    if (value != readCurrentValue()) {
        value = readCurrentValue();
        ret = changed_t::yes;
    }

    return ret;
}

resized_t FooterIconText_IntVal::updateState() {
    string_view_utf8 current_view = makeView(value);
    text.SetText(current_view);
    text.Invalidate(); // text could change, without changing pointer to buffer, need manual invalidation
                       // value changed so text is ivalid, this will not cause unnecessary redraw

    Rect16::Width_t current_width = GetTotalWidth(icon.Width(), current_view);
    if (current_width != Width()) {
        Resize(current_width);
        text.Resize(MeasureTextWidth(current_view));
        return resized_t::yes;
    }
    return resized_t::no;
}
FooterIconText_FloatVal::FooterIconText_FloatVal(window_t *parent, const img::Resource *icon,
    view_maker_cb view_maker, reader_cb value_reader)
    : AddSuperWindow<IFooterIconText>(parent, icon, GetTotalWidth(icon ? icon->w : 0, view_maker(value_reader())))
    , makeView(view_maker)
    , readCurrentValue(value_reader)
    , value(value_reader()) {
    text.SetText(makeView(value));
}

Rect16::Width_t FooterIconText_FloatVal::GetTotalWidth(Rect16::Width_t icon_w, string_view_utf8 view) {
    return MeasureTextWidth(view) + Rect16::Width_t(icon_w ? icon_w + GuiDefaults::FooterIconTextSpace : 0);
}

changed_t FooterIconText_FloatVal::updateValue() {
    changed_t ret = changed_t::no;
    if (value != readCurrentValue()) {
        value = readCurrentValue();
        ret = changed_t::yes;
    }

    return ret;
}

resized_t FooterIconText_FloatVal::updateState() {
    string_view_utf8 current_view = makeView(value);
    text.SetText(current_view);
    text.Invalidate(); // text could change, without changing pointer to buffer, need manual invalidation
    // value changed so text is ivalid, this will not cause unnecessary redraw

    Rect16::Width_t current_width = GetTotalWidth(icon.Width(), current_view);
    if (current_width != Width()) {
        Resize(current_width);
        text.Resize(MeasureTextWidth(current_view));
        return resized_t::yes;
    }
    return resized_t::no;
}
