/// @file
#pragma once

#include <i_window_menu_item.hpp>
#include <utils/compact_optional.hpp>
#include <uuid/uuid.hpp>

class MenuItemUUID : public IWindowMenuItem {

public:
    using Value = CompactOptional<UUID, UUID::nil()>;

    MenuItemUUID(const string_view_utf8 &label, const Value &value = std::nullopt);

    const Value &value() const {
        return value_;
    }
    void set_value(const Value &set);

protected:
    void printExtension(Rect16 extension_rect, Color color_text, Color color_back, ropfn raster_op) const override;
    void click(IWindowMenu &) override;

private:
    Value value_;
};
