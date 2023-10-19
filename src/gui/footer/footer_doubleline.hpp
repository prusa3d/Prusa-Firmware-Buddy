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

    // line0 cannot be changed, it shows temperatures only
    static constexpr FooterLine::IdArray line1_defaults = { { footer::Item::nozzle, footer::Item::bed, footer::Item::none } };

public:
    FooterDoubleLine(window_t *parent);
    template <class... T>
    FooterDoubleLine(window_t *parent, T... args)
        : AddSuperWindow<IFooter>(parent)
        , line_0(this, 0)
        , line_1(this, 1) {
        line_1.Create(line1_defaults);
        line_0.Create({ { args... } }, sizeof...(T)); // footer line takes array of footer::items
    }

    bool SetSlot(size_t slot_id, footer::Item item);

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};
