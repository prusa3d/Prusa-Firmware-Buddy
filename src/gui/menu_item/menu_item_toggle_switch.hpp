#pragma once

#include <i_window_menu_item.hpp>
#include <img_resources.hpp>
#include <common/utils/tristate.hpp>

/// Toggle switch with icon
/// This is similar as WI_ICON_SWITCH_OFF_ON_t, just with cleaner API and tri-state support
class MenuItemToggleSwitch : public IWindowMenuItem {

public:
    MenuItemToggleSwitch(Tristate value, const string_view_utf8 &label);

public:
    inline Tristate value() const {
        return value_;
    }

    void set_value(Tristate set, bool emit_toggled);

protected:
    /// Callback called when the value is changed by the user
    virtual void toggled([[maybe_unused]] Tristate previous_value) = 0;

protected:
    void click(IWindowMenu &) override;
    void printExtension(Rect16 extension_rect, Color color_text, Color color_back, ropfn raster_op) const override;

private:
    Tristate value_ = false;
};
