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

bool IFooter::SetSlot(FooterLine &line, size_t slot_id, footer::items item) {
    return SetSlotInit(slot_id, item) && line.Create(item, slot_id);
}

bool IFooter::SetSlotInit(size_t slot_id, footer::items item) {
    if (slot_id >= footer::eeprom::Load().size())
        return false;
    if (footer::eeprom::Load()[slot_id] != item) {
        footer::eeprom::Set(item, slot_id);
        //send event to all windows - there can be multiple footers, ScreenEvent is the best way
        Screens::Access()->ScreenEvent(nullptr, GUI_event_t::REINIT_FOOTER, footer::EncodeItemForEvent(item));
    }
    return true;
}

footer::items IFooter::GetSlotInit(size_t slot_id) {
    if (slot_id >= footer::eeprom::Load().size())
        return footer::items::count_;
    return footer::eeprom::Load()[slot_id];
}
