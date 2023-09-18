/**
 * @file MItem_love_board.cpp
 */

#include "MItem_love_board.hpp"
#include "hw_configuration.hpp"
#include "menu_spin_config.hpp"

MI_INFO_SERIAL_NUM_LOVEBOARD::MI_INFO_SERIAL_NUM_LOVEBOARD()
    : WiInfo<28>(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {

    Buff tmp = to_array();
    ChangeInformation(tmp.data());
}

MI_INFO_SERIAL_NUM_LOVEBOARD::Buff MI_INFO_SERIAL_NUM_LOVEBOARD::to_array() {
    Buff tmp = {};
    buddy::hw::Configuration &cnf = buddy::hw::Configuration::Instance();
    memcpy(tmp.data(), cnf.get_love_board().datamatrix, sizeof(LoveBoardEeprom::datamatrix));
    snprintf(tmp.data() + sizeof(LoveBoardEeprom::datamatrix), tmp.size() - sizeof(LoveBoardEeprom::datamatrix), "/%u", cnf.get_love_board().bomID);
    return tmp;
}

MI_INFO_HEATBREAK_TEMP::MI_INFO_HEATBREAK_TEMP()
    : WI_TEMP_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

MI_LOVEBOARD_SINGLE_ERR::MI_LOVEBOARD_SINGLE_ERR()
    : WiSpinInt(buddy::hw::Configuration::Instance().get_loveboard_status().single_read_error_counter, SpinCnf::int_num, _(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {}

MI_LOVEBOARD_REPEATED_ERR::MI_LOVEBOARD_REPEATED_ERR()
    : WiSpinInt(buddy::hw::Configuration::Instance().get_loveboard_status().repeated_read_error_counter, SpinCnf::int_num, _(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {}

MI_LOVEBOARD_CYCLIC_ERR::MI_LOVEBOARD_CYCLIC_ERR()
    : WiSpinInt(buddy::hw::Configuration::Instance().get_loveboard_status().cyclic_read_error_counter, SpinCnf::int_num, _(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {}

MI_LOVEBOARD_RETRIED::MI_LOVEBOARD_RETRIED()
    : WiSpinInt(buddy::hw::Configuration::Instance().get_loveboard_status().retried, SpinCnf::int_num, _(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {}

MI_LOVEBOARD_STATUS::MI_LOVEBOARD_STATUS()
    : WI_INFO_DEV_t(_(label), nullptr) {
    ChangeInformation(buddy::hw::Configuration::Instance().get_loveboard_status().data_valid ? "PASSED" : "FAILED");
}
