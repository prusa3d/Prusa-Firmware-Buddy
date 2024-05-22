/**
 * @file footer_singleline.hpp
 * @author Radek Vana
 * @brief single line footer
 * @date 2021-04-28
 */

#pragma once
#include "ifooter.hpp"

class FooterSingleline : public AddSuperWindow<IFooter> {
    FooterLine line_0;

public:
    FooterSingleline(window_t *parent);
    template <class... T>
    FooterSingleline(window_t *parent, T... args)
        : AddSuperWindow<IFooter>(parent)
        , line_0(this, 0) {
        line_0.Create({ { args... } }, sizeof...(T)); // footer line takes array of footer::items
    }

    bool SetSlot(size_t slot_id, footer::Item item);

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};
