/**
 * @file footer_line.cpp
 * @author Radek Vana
 * @date 2021-04-14
 */

#include "footer_line.hpp"
#include "footer_positioning.hpp"
#include "ScreenHandler.hpp"
#include "footer_eeprom.hpp"

FooterLine::FooterLine(window_t *parent, size_t line_no)
    : AddSuperWindow<window_frame_t>(parent, footer::LineRect(line_no), positioning::relative) {
    item_ids.fill(footer::items::count_);
}

void FooterLine::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    switch (event) {
    case GUI_event_t::CHILD_CHANGED: //event from child
        positionWindows();
        break;
    case GUI_event_t::REINIT_FOOTER:
        // count means all items changed, could be caused by change of centering option
        if (footer::DecodeItemFromEvent(param) == footer::items::count_) {
            positionWindows();
        }
        break;
    default:
        break;
    }

    SuperWindowEvent(sender, event, param);
}

// does not call destructor, just rewrites
bool FooterLine::Create(footer::items item_id, size_t index) {
    if (index >= max_items)
        return false;
    if (item_ids[index] == item_id)
        return false;

    // erase old item
    Erase(index);

    // create new item
    switch (item_id) {
    case footer::items::ItemNozzle:
        new (&items[index]) FooterItemNozzle(this);
        break;
    case footer::items::ItemBed:
        new (&items[index]) FooterItemBed(this);
        break;
    case footer::items::ItemFilament:
        new (&items[index]) FooterItemFilament(this);
        break;
    case footer::items::ItemFSensor:
        new (&items[index]) FooterItemFSensor(this);
        break;
    case footer::items::ItemSpeed:
        new (&items[index]) FooterItemSpeed(this);
        break;
    case footer::items::ItemAxisX:
        new (&items[index]) FooterItemAxisX(this);
        break;
    case footer::items::ItemAxisY:
        new (&items[index]) FooterItemAxisY(this);
        break;
    case footer::items::ItemAxisZ:
        new (&items[index]) FooterItemAxisZ(this);
        break;
    case footer::items::ItemZHeight:
        new (&items[index]) FooterItemZHeight(this);
        break;
    case footer::items::ItemPrintFan:
        new (&items[index]) FooterItemPrintFan(this);
        break;
    case footer::items::ItemHeatbreakFan:
        new (&items[index]) FooterItemHeatBreakFan(this);
        break;
#if defined(FOOTER_HAS_LIVE_Z)
    case footer::items::ItemLiveZ:
        new (&items[index]) FooterItemLiveZ(this);
        break;
#endif
#if defined(FOOTER_HAS_SHEETS)
    case footer::items::ItemSheets:
        new (&items[index]) FooterItemSheets(this);
        break;
#endif
    case footer::items::count_:
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
        //erase all
        for (index = 0; index < max_items; ++index) {
            unregister(index);
        }
    } else {
        //erase just one
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
    std::array<Rect16, array_sz> temp_rects;           //can have 2 extra empty rectangles
    std::array<Rect16::Width_t, array_sz> temp_widths; //can have 2 extra 0 valuses
    size_t count_with_borders = 0;

    if (center) {
        //add zero widths on sides
        temp_widths = addBorderZeroWidths(widths, count);
        count_with_borders = count + 2;
    } else {
        std::copy(widths.begin(), widths.begin() + count, temp_widths.begin());
        count_with_borders = count;
    }
    size_t count_after_split = calculateItemRects(temp_rects.data(), temp_widths.data(), count_with_borders);

    if (count_with_borders != count_after_split)
        return false; //did not fit

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
    return item_ids[index] != footer::items::count_;
}

window_t *FooterLine::SlotAccess(size_t index) const {
    if (index >= item_ids.size())
        return nullptr;
    if (!slotUsed(index))
        return nullptr;
    return (window_t *)(&(items[index]));
}

footer::items FooterLine::SlotUsedBy(size_t index) {
    return item_ids[index];
}

void FooterLine::unregister(size_t index) {
    window_t *pWin = SlotAccess(index);
    if (pWin) {
        unregisterAnySubWin(*pWin, first_normal, last_normal);
        item_ids[index] = footer::items::count_;
    }
}

void FooterLine::SetCenterN(size_t n_and_fewer) {
    if (footer::eeprom::SetCenterNAndFewer(n_and_fewer) == changed_t::yes) {
        Screens::Access()->ScreenEvent(nullptr, GUI_event_t::REINIT_FOOTER, footer::EncodeItemForEvent(footer::items::count_));
    }
}

size_t FooterLine::GetCenterN() {
    return footer::eeprom::LoadItemDrawCnf().centerNAndFewer;
}
