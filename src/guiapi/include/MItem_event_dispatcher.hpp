/**
 * @file MItem_event_dispatcher.hpp
 * @brief Abstract event dispatcher menu item
 */

#pragma once
#include "WindowMenuLabel.hpp"
#include "ScreenHandler.hpp"

class MI_event_dispatcher : public WI_LABEL_t {
protected:
    virtual void click(IWindowMenu & /*window_menu*/) override {
        //no way to change header at this level, have to dispatch event
        Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CLICK, (void *)this); //WI_LABEL is not a window, cannot set sender param
    }

public:
    explicit MI_event_dispatcher(string_view_utf8 label)
        : WI_LABEL_t(label, 0, is_enabled_t::yes, is_hidden_t::no) {}
    virtual void Do() = 0;
};
