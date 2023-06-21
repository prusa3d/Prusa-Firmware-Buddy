/**
 * @file MItem_love_board.cpp
 */

#include "MItem_love_board.hpp"
#include "hw_configuration.hpp"

MI_INFO_SERIAL_NUM_LOVEBOARD::MI_INFO_SERIAL_NUM_LOVEBOARD()
    : WiInfo<28>(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {

    char tmp[sizeof(LoveBoardEeprom::datamatrix_id) + 1 + 3 + 1] = ""; // datamatrix + '/' + "123" + '\n'
    buddy::hw::Configuration &cnf = buddy::hw::Configuration::Instance();
    memcpy(tmp, cnf.get_love_board().datamatrix_id, sizeof(LoveBoardEeprom::datamatrix_id));
    snprintf(tmp + sizeof(LoveBoardEeprom::datamatrix_id), sizeof(tmp) - sizeof(LoveBoardEeprom::datamatrix_id), "/%u", cnf.get_love_board().bom_id);
    ChangeInformation(tmp);
}

MI_INFO_HEATBREAK_TEMP::MI_INFO_HEATBREAK_TEMP()
    : WI_TEMP_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}
