/**
 * @file footer_doubleline.hpp
 * @author Radek Vana
 * @brief MINI printer footer
 * @date 2021-04-14
 */

#pragma once
#include "ifooter.hpp"

class FooterDoubleLine : public AddSuperWindow<IFooter> {
    FooterLine line_0;
    FooterLine line_1;

    //line0 cannot be changed, it shows temperatures only
    static constexpr FooterLine::IdArray line0_defaults = { { footer::items::ItemNozzle, footer::items::ItemBed, footer::items::count_ } };

public:
    FooterDoubleLine(window_t *parent);

    bool SetSlot(size_t slot_id, footer::items item);

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};
