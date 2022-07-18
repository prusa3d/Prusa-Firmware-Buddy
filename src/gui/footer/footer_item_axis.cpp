/**
* @file footer_item_axis.cpp
* @author Radek Vana
* @date 2021-12-02
*/
#include "footer_item_axis.hpp"
#include "resource.h"
#include "menu_vars.h"

//static variables

IFooterItemAxis::buffer_t IFooterItemAxis::buff;

string_view_utf8 IFooterItemAxis::static_makeViewIntoBuff(float value) {
    int printed_chars = snprintf(buff.data(), buff.size(), "%.2f", (double)value);

    if (printed_chars < 1) {
        buff[0] = '\0';
    } else {
        while (((--printed_chars) > 2) && (buff[printed_chars] == '0') && (buff[printed_chars - 1] != '.')) {
            buff[printed_chars] = '\0';
        }
    }
    return string_view_utf8::MakeRAM((const uint8_t *)buff.data());
}

IFooterItemAxis::IFooterItemAxis(window_t *parent, ResourceId icon_id, reader_cb value_reader)
    : AddSuperWindow<FooterIconText_FloatVal>(parent, icon_id, static_makeViewIntoBuff, value_reader) {
}
