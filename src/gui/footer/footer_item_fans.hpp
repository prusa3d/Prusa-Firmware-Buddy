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
    static string_view_utf8 static_makeView(int value);

public:
    IFooterItemFan(window_t *parent, uint16_t icon_id, reader_cb value_reader);
};

/**
 * @brief print fan rpm footer item
 */
class FooterItemPrintFan : public AddSuperWindow<IFooterItemFan> {
    static int static_readValue();

public:
    static string_view_utf8 GetName() { return _("Print fan"); }
    FooterItemPrintFan(window_t *parent);
};

/**
 * @brief heatbreak fan rpm footer item
 */
class FooterItemHeatBreakFan : public AddSuperWindow<IFooterItemFan> {
    static int static_readValue();

public:
    static string_view_utf8 GetName() { return _("Heatbreak fan"); }
    FooterItemHeatBreakFan(window_t *parent);
};
