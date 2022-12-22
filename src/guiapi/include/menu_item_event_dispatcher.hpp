/**
 * @file menu_item_event_dispatcher.hpp
 * @brief Abstract event dispatcher menu item
 */

#pragma once
#include "WindowMenuLabel.hpp"

class MI_event_dispatcher : public WI_LABEL_t {
protected:
    virtual void click(IWindowMenu & /*window_menu*/) override;

public:
    explicit MI_event_dispatcher(string_view_utf8 label)
        : WI_LABEL_t(label, nullptr, is_enabled_t::yes, is_hidden_t::no) {}
    virtual void Do() = 0;
};
