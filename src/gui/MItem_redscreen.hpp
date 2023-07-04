#pragma once
#include "WindowMenuItems.hpp"

class MI_TRIGGER_REDSCREEN : public WI_LABEL_t {
public:
    MI_TRIGGER_REDSCREEN();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};
