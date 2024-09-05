/**
 * @file MItem_love_board.cpp
 */

#include "MItem_love_board.hpp"
#include "hw_configuration.hpp"
#include "WindowMenuSpin.hpp"

MI_INFO_SERIAL_NUM_LOVEBOARD::MI_INFO_SERIAL_NUM_LOVEBOARD()
    : WiInfo<28>(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {

    std::array<char, sizeof(LoveBoardEeprom::datamatrix) + 1 + 3 + 1> tmp;
    buddy::hw::Configuration &cnf = buddy::hw::Configuration::Instance();
    memcpy(tmp.data(), cnf.get_love_board().datamatrix, sizeof(LoveBoardEeprom::datamatrix));
    snprintf(tmp.data() + sizeof(LoveBoardEeprom::datamatrix), tmp.size() - sizeof(LoveBoardEeprom::datamatrix), "/%u", cnf.get_love_board().bomID);
    ChangeInformation(tmp.data());
}

MI_INFO_HEATBREAK_TEMP::MI_INFO_HEATBREAK_TEMP()
    : WI_TEMP_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}
