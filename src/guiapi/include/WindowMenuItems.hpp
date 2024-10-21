/**
 * @file WindowMenuItems.hpp
 * @author Radek Vana
 * @brief Some commonly used menu items (to be inherited)
 * @date 2020-11-09
 */

#pragma once

#include "i_window_menu_item.hpp"
#include "i_window_menu_item.hpp"
#include "WindowMenuSpin.hpp"
#include "WindowMenuSwitch.hpp"
#include "WindowMenuInfo.hpp"

class WI_ICON_SWITCH_OFF_ON_t : public IWindowMenuItem {

public:
    WI_ICON_SWITCH_OFF_ON_t(bool value, const string_view_utf8 &label, const img::Resource *id_icon = nullptr, is_enabled_t enabled = is_enabled_t::yes, is_hidden_t hidden = is_hidden_t::no);

public:
    void set_value(bool set, bool emit_change);

    inline bool value() const {
        return value_;
    }

    // TODO: Remove this legacy function
    inline size_t GetIndex() const {
        return value_;
    }

    // TODO: Remove this legacy function
    inline void SetIndex(size_t index) {
        set_value(index > 0, false);
    }

protected:
    virtual invalidate_t change(int dif) override;
    virtual void OnChange([[maybe_unused]] size_t old_index) {} // TODO: Remove this ugly legacy parameter

    virtual void click(IWindowMenu &window_menu) override;
    virtual void printExtension(Rect16 extension_rect, Color color_text, Color color_back, ropfn raster_op) const override;

    /// Legacy reference to value_
    const bool &index; // TODO: Remove this legacy variable

private:
    bool value_ = false;
};

class MI_RETURN : public IWindowMenuItem {
public:
    static constexpr const char *label = N_("Return");

    MI_RETURN();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_EXIT : public IWindowMenuItem {
public:
    static constexpr const char *label { N_("Exit") };
    MI_EXIT();

protected:
    void click(IWindowMenu &window_menu) override;
};
