#pragma once
#include <i_window_menu_item.hpp>

class MI_M600 : public IWindowMenuItem {
    inline static constexpr const char *const label = N_("Filament Change");

public:
    MI_M600();

protected:
    void click(IWindowMenu &window_menu) override;
    void Loop() override;
};
