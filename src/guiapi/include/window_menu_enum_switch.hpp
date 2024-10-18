#pragma once

#include "WindowMenuSwitch.hpp"

/// List based menu item that allows selecting an item from an enum
/// You gotta:
/// - \p SetIndex in the constructor to the appropriate value
/// - Override \p OnChange to define behavior when the item changes
class WiEnumSwitch : public IWiSwitch {

public:
    using Items = std::span<const char *const>;
    using ItemsEnabled = std::optional<std::span<const bool>>;

public:
    /// \param items Array of names of the individual items (non-translated)
    /// \param translate_items Whether the items should be translated or displayed verbatim
    /// \param items_enabled Optional bool array storing whether individual items are enabled. Disabled items are skipped.
    WiEnumSwitch(const string_view_utf8 &label, const Items &items, bool translate_items, const ItemsEnabled &items_enabled = std::nullopt, const img::Resource *icon = nullptr);

public:
    size_t item_count() const final {
        return items_.size();
    }

    string_view_utf8 current_item_text() const final {
        const char *str = items_[std::min(index, items_.size() - 1)];
        return translate_items_ ? _(str) : string_view_utf8::MakeRAM(str);
    }

protected:
    invalidate_t change(int diff) override;

private:
    Items items_;
    ItemsEnabled items_enabled_;
    bool translate_items_ = false;
};
