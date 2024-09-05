/**
 * @file menu_item_xlcd.cpp
 */

#include "menu_item_xlcd.hpp"
#include "hw_configuration.hpp"
#include "WindowMenuSpin.hpp"

MI_INFO_SERIAL_NUM_XLCD::MI_INFO_SERIAL_NUM_XLCD()
    : WiInfo<28>(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {

    std::array<char, sizeof(XlcdEeprom::datamatrix) + 1 + 3 + 1> tmp;
    buddy::hw::Configuration &cnf = buddy::hw::Configuration::Instance();
    memcpy(tmp.data(), cnf.get_xlcd().datamatrix, sizeof(XlcdEeprom::datamatrix));
    snprintf(tmp.data() + sizeof(XlcdEeprom::datamatrix), tmp.size() - sizeof(XlcdEeprom::datamatrix), "/%u", cnf.get_xlcd().bomID);
    ChangeInformation(tmp.data());
}
