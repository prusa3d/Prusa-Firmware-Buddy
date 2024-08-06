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

/// Subclass of \p WiEnumSwitch that is directly linked to an enum store item
/// \param item_ptr_ Member pointer to the store item, so \p &ConfigStore::member
template <const auto item_ptr_>
class WiStoreEnumSwitch : public WiEnumSwitch {

public:
    using Value = std::remove_cvref_t<decltype(config_store().*item_ptr_)>::value_type;

public:
    WiStoreEnumSwitch(const string_view_utf8 &label, const Items &items, bool translate_items, const ItemsEnabled &items_enabled = std::nullopt, const img::Resource *icon = nullptr)
        : WiEnumSwitch(label, items, translate_items, items_enabled, icon) {
        if constexpr (std::is_enum_v<Value>) {
            this->SetIndex(ftrstd::to_underlying((config_store().*item_ptr_).get()));
        } else {
            this->SetIndex((config_store().*item_ptr_).get());
        }
    }

protected:
    void OnChange(size_t) override {
        (config_store().*item_ptr_).set(static_cast<Value>(index));
    }
};
