/**
 * @file footer_line.cpp
 * @author Radek Vana
 * @date 2021-04-14
 */

#include "footer_line.hpp"
#include "footer_positioning.hpp"
#include "ScreenHandler.hpp"
#include "footer_eeprom.hpp"
#include <option/has_side_fsensor.h>
#include <option/has_mmu2.h>

FooterLine::FooterLine(window_t *parent, size_t line_no)
    : AddSuperWindow<window_frame_t>(parent, footer::line_rect(line_no), positioning::relative) {
    item_ids.fill(footer::Item::none);
}

void FooterLine::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    switch (event) {
    case GUI_event_t::CHILD_CHANGED: // event from child
        positionWindows();
        break;
    case GUI_event_t::REINIT_FOOTER:
        // none means all items changed, could be caused by change of centering option
        if (footer::decode_item_from_event(param) == footer::Item::none) {
            positionWindows();
        }
        break;
    default:
        break;
    }

    SuperWindowEvent(sender, event, param);
}

// does not call destructor, just rewrites
bool FooterLine::Create(footer::Item item_id, size_t index) {
    if (index >= max_items) {
        return false;
    }

    if (item_id >= footer::Item::_count) {
        item_id = footer::Item::none;
    }
    if (item_ids[index] == item_id) {
        return false;
    }

    // erase old item
    Erase(index);

    // create new item
    switch (item_id) {
    case footer::Item::nozzle:
        new (&items[index]) FooterItemNozzle(this);
        break;
    case footer::Item::bed:
        new (&items[index]) FooterItemBed(this);
        break;
    case footer::Item::filament:
        new (&items[index]) FooterItemFilament(this);
        break;
    case footer::Item::f_sensor:
        new (&items[index]) FooterItemFSensor(this);
        break;
    case footer::Item::f_s_value:
        new (&items[index]) FooterItemFSValue(this);
        break;
    case footer::Item::speed:
        new (&items[index]) FooterItemSpeed(this);
        break;
    case footer::Item::axis_x:
        new (&items[index]) FooterItemAxisX(this);
        break;
    case footer::Item::axis_y:
        new (&items[index]) FooterItemAxisY(this);
        break;
    case footer::Item::axis_z:
        new (&items[index]) FooterItemAxisZ(this);
        break;
    case footer::Item::z_height:
        new (&items[index]) FooterItemZHeight(this);
        break;
    case footer::Item::print_fan:
        new (&items[index]) FooterItemPrintFan(this);
        break;
    case footer::Item::heatbreak_fan:
        new (&items[index]) FooterItemHeatBreakFan(this);
        break;
    case footer::Item::input_shaper_x:
        new (&items[index]) FooterItemInputShaperX(this);
        break;
    case footer::Item::input_shaper_y:
        new (&items[index]) FooterItemInputShaperY(this);
        break;
    case footer::Item::live_z:
#if defined(FOOTER_HAS_LIVE_Z)
        new (&items[index]) FooterItemLiveZ(this);
#endif
        break;
    case footer::Item::sheets:
#if defined(FOOTER_HAS_SHEETS)
        new (&items[index]) FooterItemSheets(this);
#endif
        break;
    case footer::Item::heatbreak_temp:
        new (&items[index]) FooterItemHeatBreak(this);
        break;
    case footer::Item::finda:
#if HAS_MMU2()
        new (&items[index]) FooterItemFinda(this);
#endif
        break;
    case footer::Item::current_tool:
#if defined(FOOTER_HAS_TOOL_NR)
        new (&items[index]) FooterItemCurrentTool(this);
#endif
        break;
    case footer::Item::all_nozzles:
#if defined(FOOTER_HAS_TOOL_NR)
        new (&items[index]) FooterItemAllNozzles(this);
#endif
        break;
    case footer::Item::f_sensor_side:
#if HAS_SIDE_FSENSOR()
        new (&items[index]) FooterItemFSensorSide(this);
#endif
        break;
    case footer::Item::nozzle_diameter:
        new (&items[index]) FooterItemNozzleDiameter(this);
        break;
    case footer::Item::nozzle_pwm:
        new (&items[index]) FooterItemNozzlePWM(this);
        break;
    case footer::Item::enclosure_temp:
#if XL_ENCLOSURE_SUPPORT()
        new (&items[index]) FooterItemEnclosure(this);
#endif
        break;
    case footer::Item::none:
    case footer::Item::_count:
        break;
    }
    item_ids[index] = item_id;

    positionWindows();
    return true;
}

void FooterLine::Create(const IdArray &ids, size_t count) {
    count = std::min(count, ids.size());
    for (size_t i = 0; i < count; ++i) {
        Create(ids[i], i);
    }
}

// does not call destructor, just rewrites
void FooterLine::Erase(size_t index) {
    if (index >= max_items) {
        // erase all
        for (index = 0; index < max_items; ++index) {
            unregister(index);
        }
    } else {
        // erase just one
        unregister(index);
    }
    positionWindows();
}

void FooterLine::positionWindows() {
    Rectangles item_rects;
    std::array<Rect16::Width_t, max_items> widths;
    size_t count = storeWidths(widths);

    count = split(item_rects, widths, count);

    setItemRectangles(item_rects.begin(), item_rects.begin() + count);

    Invalidate();
}

void FooterLine::setItemRectangles(Rectangles::iterator rectangles_begin, Rectangles::iterator rectangles_end) {
    // index == index of item (nth item)
    for (size_t index = 0; index < max_items; ++index) {
        window_t *pWin = SlotAccess(index);
        if (pWin) {
            if (rectangles_begin != rectangles_end) {
                pWin->SetRectWithoutTransformation(*rectangles_begin);
                ++rectangles_begin;
                pWin->Show();
            } else {
                pWin->Hide();
            }
        }
    }
}

size_t FooterLine::split(Rectangles &returned_rects, const std::array<Rect16::Width_t, max_items> &widths, size_t count) const {
    bool split_result = false;
    // this loop will usually be run only once, max twice, when items does not fit in the window
    while ((!split_result) && count) {

        split_result = try_split(returned_rects, widths, count);

        if (!split_result) {
            --count; // 1 fewer item for next attempt
        }
    }

    return count;
}

bool FooterLine::try_split(Rectangles &returned_rects, const std::array<Rect16::Width_t, max_items> &widths, size_t count) const {
    bool center = GetCenterN() >= count;
    std::array<Rect16, array_sz> temp_rects; // can have 2 extra empty rectangles
    std::array<Rect16::Width_t, array_sz> temp_widths; // can have 2 extra 0 valuses
    size_t count_with_borders = 0;

    if (center) {
        // add zero widths on sides
        temp_widths = addBorderZeroWidths(widths, count);
        count_with_borders = count + 2;
    } else {
        std::copy(widths.begin(), widths.begin() + count, temp_widths.begin());
        count_with_borders = count;
    }
    size_t count_after_split = calculateItemRects(temp_rects.data(), temp_widths.data(), count_with_borders);

    if (count_with_borders != count_after_split) {
        return false; // did not fit
    }

    auto src_begin = center ? temp_rects.begin() + 1 : temp_rects.begin();
    auto src_end = src_begin + count;
    std::copy(src_begin, src_end, returned_rects.begin());

    return true;
}

size_t FooterLine::calculateItemRects(Rect16 *item_rects, Rect16::Width_t *widths, size_t count) const {
    Rect16 ths_rc = GetRectWithoutTransformation();
    ths_rc = point_i16_t({ 0, 0 }); // pos 0:0, because of relative coords
    return ths_rc.HorizontalSplit(item_rects, widths, count);
}

size_t FooterLine::storeWidths(std::array<Rect16::Width_t, max_items> &widths) const {
    size_t count = 0;
    for (size_t index = 0; index < max_items; ++index) {
        window_t *pWin = SlotAccess(index);
        if (pWin) {
            widths[count] = pWin->GetRectWithoutTransformation().Width(); // without this complicated call last rect could be cut - when resized from small to big
            ++count;
        }
    }
    return count;
}

std::array<Rect16::Width_t, FooterLine::array_sz> FooterLine::addBorderZeroWidths(const std::array<Rect16::Width_t, FooterLine::max_items> &source, size_t count) {
    count = std::min(count, FooterLine::max_items);
    std::array<Rect16::Width_t, array_sz> ret;
    ret.fill(0);
    std::copy(source.begin(), source.begin() + count, ret.begin() + 1);
    return ret;
}

bool FooterLine::slotUsed(size_t index) const {
    return item_ids[index] != footer::Item::none;
}

window_t *FooterLine::SlotAccess(size_t index) const {
    if (index >= item_ids.size()) {
        return nullptr;
    }
    if (!slotUsed(index)) {
        return nullptr;
    }
    return (window_t *)(&(items[index]));
}

footer::Item FooterLine::SlotUsedBy(size_t index) {
    return item_ids[index];
}

void FooterLine::unregister(size_t index) {
    window_t *pWin = SlotAccess(index);
    if (pWin) {
        unregisterAnySubWin(*pWin, first_normal, last_normal);
        item_ids[index] = footer::Item::none;
    }
}

void FooterLine::SetCenterN(size_t n_and_fewer) {
    if (footer::eeprom::set_center_n_and_fewer(n_and_fewer) == changed_t::yes) {
        Screens::Access()->ScreenEvent(nullptr, GUI_event_t::REINIT_FOOTER, footer::encode_item_for_event(footer::Item::none));
    }
}

size_t FooterLine::GetCenterN() {
    return footer::eeprom::load_item_draw_cnf().center_n_and_fewer;
}
