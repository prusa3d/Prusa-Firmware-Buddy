/**
 * @file footer_item_fans.hpp
 * @author Radek Vana
 * @brief axis position related footer items
 * @date 2021-12-02
 */
#pragma once
#include "ifooter_item.hpp"

/**
 * @brief parent for fan footer items
 */
class IFooterItemFan : public AddSuperWindow<FooterIconText_IntVal> {
protected:
    static constexpr int max_rpm = 99999;
    using buffer_t = std::array<char, sizeof("99999rpm")>;
    static string_view_utf8 static_makeViewIntoBuff(int value, buffer_t &buff);

public:
    IFooterItemFan(window_t *parent, uint16_t icon_id, view_maker_cb view_maker, reader_cb value_reader);
};

/**
 * @brief print fan rpm footer item
 */
class FooterItemPrintFan : public AddSuperWindow<IFooterItemFan> {
    static int static_readValue();
    static buffer_t buffer;
    static string_view_utf8 static_makeView(int value) {
        return static_makeViewIntoBuff(value, buffer);
    }

public:
    static string_view_utf8 GetName() { return _("Print Fan"); }
    FooterItemPrintFan(window_t *parent);
};

/**
 * @brief heatbreak fan rpm footer item
 */
class FooterItemHeatBreakFan : public AddSuperWindow<IFooterItemFan> {
    static int static_readValue();
    static buffer_t buffer;
    static string_view_utf8 static_makeView(int value) {
        return static_makeViewIntoBuff(value, buffer);
    }

public:
    static string_view_utf8 GetName() { return _("Hotend Fan"); }
    FooterItemHeatBreakFan(window_t *parent);
};
