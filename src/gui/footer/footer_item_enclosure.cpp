/**
 * @file footer_item_enclosure.cpp
 */

#include "footer_item_enclosure.hpp"
#include "img_resources.hpp"
#include "xl_enclosure.hpp"

FooterItemEnclosure::FooterItemEnclosure(window_t *parent)
    : AddSuperWindow<FooterIconText_IntVal>(parent, &img::enclosure_16x16, static_makeView, static_readValue) {
}

/**
 * @brief Enclosure temperature (°C)
 * @return negative int MIN if value is invalid
 */
int FooterItemEnclosure::static_readValue() {
    return int(xl_enclosure.getEnclosureTemperature());
}

/**
 * @brief Create stringview from int value
 *
 * @param value in °C, or negative min if invalid
 * @return string_view_utf8
 */
string_view_utf8 FooterItemEnclosure::static_makeView(int value) {
    static constexpr const char *str = "%u\xC2\xB0\x43";
    static char buff[7]; // max "999°C", minimum 1°C - zero and negative degrees are
    if (value == std::numeric_limits<int>().min()) {
        strlcpy(buff, "--", sizeof(buff));
    } else {
        uint value_to_print = std::clamp(value, 0, 999);
        snprintf(buff, sizeof(buff), str, value_to_print);
    }

    return string_view_utf8::MakeRAM((const uint8_t *)buff);
}
