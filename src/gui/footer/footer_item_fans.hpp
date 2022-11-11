/**
 * @file footer_item_fans.hpp
 * @brief axis position related footer items
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
    IFooterItemFan(window_t *parent, const png::Resource *icon, view_maker_cb view_maker, reader_cb value_reader);
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
    static string_view_utf8 GetName();
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
    static string_view_utf8 GetName();
    FooterItemHeatBreakFan(window_t *parent);
};
