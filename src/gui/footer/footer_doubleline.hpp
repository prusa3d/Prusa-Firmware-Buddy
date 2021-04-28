/**
 * @file footer_doubleline.hpp
 * @author Radek Vana
 * @brief MINI printer footer
 * @date 2021-04-14
 */

#pragma once
#include "window_frame.hpp"
#include "footer_line.hpp"
#include "footer_item_union.hpp"

class FooterDoubleLine : public AddSuperWindow<window_frame_t> {
    FooterLine line_0;
    FooterLine line_1;

    //line0 cannot be changed, it shows temperatures only
    static constexpr FooterLine::IdArray line0_defaults = { { footer::items::ItemNozzle, footer::items::ItemBed, footer::items::count_ } };
    static FooterLine::IdArray line1_init; //adjustable

public:
    FooterDoubleLine(window_t *parent);

    //sets line 1, line 0 is const
    bool SetSlot(size_t slot_id, footer::items item);
    static bool SetSlotInit(size_t slot_id, footer::items item);
    static footer::items GetSlotInit(size_t slot_id);

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};
