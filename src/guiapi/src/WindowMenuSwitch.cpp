#include "WindowMenuSwitch.hpp"

#include <string_builder.hpp>

MenuItemSwitch::MenuItemSwitch(const string_view_utf8 &label, const std::span<const char *const> &items, size_t initial_index)
    : MenuItemSelectMenu(label)
    , items_(items) //
{
    set_current_item(initial_index);
}

bool MenuItemSwitch::on_item_selected(const OnItemSelectedArgs &args) {
    set_current_item(args.new_index); // OnChange() expects updated MenuItemSelectMenu::current_item_ for correct function
    OnChange(args.old_index);
    return true;
}

string_view_utf8 MenuItemSwitch::build_item_text(int index, [[maybe_unused]] StringViewUtf8ParamBase &params) const {
    const char *str = items_[index];
    if (translate_items_) {
        return _(str);
    } else {
        return string_view_utf8::MakeCPUFLASH(str);
    }
}
