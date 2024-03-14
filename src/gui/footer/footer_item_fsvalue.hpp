#pragma once
#include "ifooter_item.hpp"
#include "i18n.h"
#include "sensor_data_buffer.hpp"

class FooterItemFSValue : public AddSuperWindow<FooterIconText_IntVal> {
    static string_view_utf8 static_makeView(int value);
    static int static_readValue();

public:
    FooterItemFSValue(window_t *parent);
};
