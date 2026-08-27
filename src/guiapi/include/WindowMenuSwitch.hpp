#pragma once

#include <gui/menu_item/menu_item_select_menu.hpp>

class MenuItemSwitch : public MenuItemSelectMenu {

public:
    MenuItemSwitch(const string_view_utf8 &label, const std::span<const char *const> &items, size_t initial_index = 0);

    inline void set_translate_items(bool set) {
        translate_items_ = set;
    }

    inline int item_count() const final {
        return items_.size();
    }

protected:
    [[deprecated("Use on_item_selected")]]
    virtual void OnChange([[maybe_unused]] size_t old_index) {};

    bool on_item_selected(const OnItemSelectedArgs &args) override;

    string_view_utf8 build_item_text(int index, StringViewUtf8ParamBase &params) const final;

private:
    std::span<const char *const> items_;
    bool translate_items_ = true;
};
