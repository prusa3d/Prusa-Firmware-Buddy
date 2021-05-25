/**
 * @file footer_doubleline.cpp
 * @author Radek Vana
 * @date 2021-04-14
 */

#include "footer_doubleline.hpp"
#include "ScreenHandler.hpp"
#include "footer_eeprom.hpp"

FooterDoubleLine::FooterDoubleLine(window_t *parent)
    : AddSuperWindow<IFooter>(parent)
    , line_0(this, 0)
    , line_1(this, 1) {
    line_0.Create(line0_defaults);
    line_1.Create(footer::eeprom::Load());
}

bool FooterDoubleLine::SetSlot(size_t slot_id, footer::items item) {
    return IFooter::SetSlot(line_1, slot_id, item);
}

void FooterDoubleLine::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::REINIT_FOOTER) {
        //!!!cannot use linked lists inside footer lines!!!, because they would change when Create method is called
        //if needed "footer::DecodeItemFromEvent" is meant do be used in combination with "FooterLine::SlotUsedBy" and "FooterLine::SlotAccess" instead
        line_1.Create(footer::eeprom::Load()); // create will not do anything if wanted item type already exist in given slot
    }

    SuperWindowEvent(sender, event, param);
}
