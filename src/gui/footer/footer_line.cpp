/**
 * @file footer_line.cpp
 * @author Radek Vana
 * @date 2021-04-14
 */

#include "footer_line.hpp"
#include "footer_positioning.hpp"

FooterLine::FooterLine(window_t *parent, size_t line_no)
    : AddSuperWindow<window_frame_t>(parent, footer::LineRect(line_no), positioning::relative) {
    item_ids.fill(footer::items::count_);
}

void FooterLine::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::CHILD_CHANGED) {
        positionWindows();
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
    case footer::items::ItemSpeed:
        new (&items[index]) FooterItemSpeed(this);
        break;
    case footer::items::ItemLiveZ:
        new (&items[index]) FooterItemLiveZ(this);
        break;
    case footer::items::ItemSheets:
        new (&items[index]) FooterItemSheets(this);
        break;
    case footer::items::count_:
        break;
    }
    item_ids[index] = item_id;

    positionWindows();
    return true;
}

void FooterLine::Create(const IdArray &ids) {
    for (size_t i = 0; i < ids.size(); ++i) {
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
    Rect16 splits[max_items];
    Rect16::Width_t widths[max_items];
    size_t count = 0;

    //store widths
    for (size_t index = 0; index < max_items; ++index) {
        window_t *pWin = slotAccess(index);
        if (pWin) {
            widths[count] = pWin->GetRectWithoutTransformation().Width(); // without this complicated call last rect could be cut - when resized from small to big
            ++count;
        }
    }

    if (!count)
        return; // no item

    //calculate rects
    Rect16 ths_rc = GetRectWithoutTransformation();
    ths_rc = point_i16_t({ 0, 0 }); // pos 0:0, because of relative coords
    count = ths_rc.HorizontalSplit(splits, widths, count);

    // change rects
    size_t index = 0;      // index of item (nth item)
    size_t used_index = 0; // index of valid item (nth item)
    for (; index < max_items; ++index) {
        window_t *pWin = slotAccess(index);
        if (pWin) {
            pWin->SetRectWithoutTransformation(splits[used_index]);
            ++used_index;
        }
    }
    Invalidate();
}

bool FooterLine::slotUsed(size_t index) {
    return item_ids[index] != footer::items::count_;
}

window_t *FooterLine::slotAccess(size_t index) {
    if (!slotUsed(index))
        return nullptr;
    return (window_t *)(&(items[index]));
}

void FooterLine::unregister(size_t index) {
    window_t *pWin = slotAccess(index);
    if (pWin) {
        unregisterAnySubWin(*pWin, first_normal, last_normal);
        item_ids[index] = footer::items::count_;
    }
}
