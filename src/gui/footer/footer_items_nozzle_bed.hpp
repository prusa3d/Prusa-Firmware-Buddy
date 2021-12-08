/**
 * @file footer_items_nozzle_bed.hpp
 * @author Radek Vana
 * @brief nozzle and bed heater items for footer
 * @date 2021-04-14
 */

#pragma once
#include "footer_items_heaters.hpp"

class FooterItemNozzle : public AddSuperWindow<FooterItemHeater> {
    static string_view_utf8 static_makeView(int value);
    static int static_readValue();

public:
    static string_view_utf8 GetName() { return _("Nozzle"); }
    FooterItemNozzle(window_t *parent);
};

class FooterItemBed : public AddSuperWindow<FooterItemHeater> {
    static string_view_utf8 static_makeView(int value);
    static int static_readValue();

public:
    static string_view_utf8 GetName() { return _("Bed"); }
    FooterItemBed(window_t *parent);
};
