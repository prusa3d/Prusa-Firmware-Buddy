/**
 * @file footer_item_axis.hpp
 * @author Radek Vana
 * @brief axis position related footer items
 * @date 2021-12-02
 */
#pragma once
#include "ifooter_item.hpp"

/**
 * @brief parent for X, Y, Z footer items
 */
class IFooterItemAxis : public AddSuperWindow<FooterIconText_FloatVal> {
protected:
    using buffer_t = std::array<char, 7>;
    static string_view_utf8 static_makeViewIntoBuff(float value, buffer_t &buff);

public:
    IFooterItemAxis(window_t *parent, uint16_t icon_id, view_maker_cb view_maker, reader_cb value_reader);
};

/**
 * @brief current X pos footer item
 */
class FooterItemAxisX : public AddSuperWindow<IFooterItemAxis> {
    static float static_readValue();
    static buffer_t buffer;
    static string_view_utf8 static_makeView(float value) {
        return static_makeViewIntoBuff(value, buffer);
    }

public:
    static string_view_utf8 GetName() { return _("X Axis"); }
    FooterItemAxisX(window_t *parent);
};

/**
 * @brief current Y pos footer item
 */
class FooterItemAxisY : public AddSuperWindow<IFooterItemAxis> {
    static float static_readValue();
    static buffer_t buffer;
    static string_view_utf8 static_makeView(float value) {
        return static_makeViewIntoBuff(value, buffer);
    }

public:
    static string_view_utf8 GetName() { return _("Y Axis"); }
    FooterItemAxisY(window_t *parent);
};

/**
 * @brief current Z pos footer item
 */
class FooterItemAxisZ : public AddSuperWindow<IFooterItemAxis> {
    static float static_readValue();
    static buffer_t buffer;
    static string_view_utf8 static_makeView(float value) {
        return static_makeViewIntoBuff(value, buffer);
    }

public:
    static string_view_utf8 GetName() { return _("Z Axis"); }
    FooterItemAxisZ(window_t *parent);
};

/**
 * @brief current Z pos footer item including MBL
 */
class FooterItemZHeight : public AddSuperWindow<FooterIconText_FloatVal> {
    static string_view_utf8 static_makeView(float value);
    static float static_readValue();

public:
    static string_view_utf8 GetName() { return _("Z Height"); }
    FooterItemZHeight(window_t *parent);
};
