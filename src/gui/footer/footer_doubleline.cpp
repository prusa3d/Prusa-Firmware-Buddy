/**
 * @file footer_doubleline.cpp
 * @author Radek Vana
 * @date 2021-04-14
 */

#include "footer_doubleline.hpp"
#include "ScreenHandler.hpp"

FooterLine::IdArray FooterDoubleLine::line1_init = { { footer::items::ItemSpeed, footer::items::ItemLiveZ, footer::items::ItemFilament } };

FooterDoubleLine::FooterDoubleLine(window_t *parent)
    : AddSuperWindow<window_frame_t>(parent, GuiDefaults::RectFooter, positioning::absolute)
    , line_0(this, 0)
    , line_1(this, 1) {
    Disable();
    line_0.Create(line0_defaults);
    line_1.Create(line1_init);
}

bool FooterDoubleLine::SetSlot(size_t slot_id, footer::items item) {
    return SetSlotInit(slot_id, item) && line_1.Create(item, slot_id);
}

bool FooterDoubleLine::SetSlotInit(size_t slot_id, footer::items item) {
    if (slot_id >= line1_init.size())
        return false;
    if (line1_init[slot_id] != item) {
        line1_init[slot_id] = item;
        //send event to all windows - there can be multiple footers, ScreenEvent is the best way
        Screens::Access()->ScreenEvent(nullptr, GUI_event_t::REINIT_FOOTER, footer::EncodeItemForEvent(item));
    }
    return true;
}

footer::items FooterDoubleLine::GetSlotInit(size_t slot_id) {
    if (slot_id >= line1_init.size())
        return footer::items::count_;
    return line1_init[slot_id];
}

void FooterDoubleLine::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::REINIT_FOOTER) {
        //!!!cannot use linked lists inside footer lines!!!, because they would change when Create method is called
        //if needed "footer::DecodeItemFromEvent" is meant do be used in combination with "FooterLine::SlotUsedBy" and "FooterLine::SlotAccess" instead
        for (size_t index = 0; index < FooterLine::Size(); ++index) {
            //no need to recreate for line 0 - it is const
            //2nd line is not const, can create different item
            line_1.Create(line1_init[index], index); // create will not do anything if wanted item type already exist in given slot
        }
    }

    SuperWindowEvent(sender, event, param);
}
