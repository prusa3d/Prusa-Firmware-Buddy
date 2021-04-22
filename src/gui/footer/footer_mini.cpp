/**
 * @file footer_mini.cpp
 * @author Radek Vana
 * @date 2021-04-14
 */

#include "footer_mini.hpp"
FooterLine::IdArray FooterMini::line1_init = { { footer::items::ItemSpeed, footer::items::ItemLiveZ, footer::items::ItemFilament } };

FooterMini::FooterMini(window_t *parent)
    : AddSuperWindow<window_frame_t>(parent, GuiDefaults::RectFooter, positioning::absolute)
    , line_0(this, 0)
    , line_1(this, 1) {
    Disable();
    line_0.Create(line0_defaults);
    line_1.Create(line1_init);
}

bool FooterMini::SetSlot(size_t slot_id, footer::items item) {
    return SetSlotInit(slot_id, item) && line_1.Create(item, slot_id);
}

bool FooterMini::SetSlotInit(size_t slot_id, footer::items item) {
    if (slot_id >= line1_init.size())
        return false;
    line1_init[slot_id] = item;
    return true;
}

footer::items FooterMini::GetSlotInit(size_t slot_id) {
    if (slot_id >= line1_init.size())
        return footer::items::count_;
    return line1_init[slot_id];
}
