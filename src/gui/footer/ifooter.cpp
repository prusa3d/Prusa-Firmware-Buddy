/**
 * @file footer_doubleline.cpp
 * @author Radek Vana
 * @date 2021-04-14
 */

#include "ifooter.hpp"
#include "ScreenHandler.hpp"
#include "footer_eeprom.hpp"

IFooter::IFooter(window_t *parent)
    : AddSuperWindow<window_frame_t>(parent, GuiDefaults::RectFooter, positioning::absolute) {
    Disable();
}

bool IFooter::SetSlot(FooterLine &line, size_t slot_id, footer::Item item) {
    return SetSlotInit(slot_id, item) && line.Create(item, slot_id);
}

bool IFooter::SetSlotInit(size_t slot_id, footer::Item item) {
    if (slot_id >= footer::eeprom::load().size())
        return false;
    if (footer::eeprom::load()[slot_id] != item) {
        footer::eeprom::set(item, slot_id);
        // send event to all windows - there can be multiple footers, ScreenEvent is the best way
        Screens::Access()->ScreenEvent(nullptr, GUI_event_t::REINIT_FOOTER, footer::encode_item_for_event(item));
    }
    return true;
}

footer::Item IFooter::GetSlotInit(size_t slot_id) {
    if (slot_id >= footer::eeprom::load().size())
        return footer::Item::none;
    return footer::eeprom::load()[slot_id];
}
