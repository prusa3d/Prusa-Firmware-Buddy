#include <window_menu_enum_switch.hpp>

WiEnumSwitch::WiEnumSwitch(const string_view_utf8 &label, const Items &items, bool translate_items, const ItemsEnabled &items_enabled, const img::Resource *icon)
    : IWiSwitch(label, icon)
    , items_(items)
    , items_enabled_(items_enabled)
    , translate_items_(translate_items) {
    assert(!items_enabled || items_enabled->size() == items.size());

    // Update extension width
    changeExtentionWidth();
}

invalidate_t WiEnumSwitch::change(int diff) {
    if (items_enabled_) {
        // Skip disabled items
        auto new_index = index;
        const auto item_count = this->item_count();

        while (true) {
            new_index = (new_index + 1) % item_count;

            // If we're where we started, stop to prevent infinite loop
            if (new_index == index) {
                return invalidate_t::no;
            }

            if ((*items_enabled_)[new_index]) {
                SetIndex(new_index);
                return invalidate_t::yes;
            }
        }
    } else {
        return IWiSwitch::change(diff);
    }
}
