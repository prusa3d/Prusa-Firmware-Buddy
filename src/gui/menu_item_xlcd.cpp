/**
 * @file menu_item_xlcd.cpp
 */

#include "menu_item_xlcd.hpp"
#include "hw_configuration.hpp"
#include "menu_spin_config.hpp"

MI_INFO_SERIAL_NUM_XLCD::MI_INFO_SERIAL_NUM_XLCD()
    : WiInfo<28>(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {

    Buff tmp = to_array();
    ChangeInformation(tmp.data());
}

MI_INFO_SERIAL_NUM_XLCD::Buff MI_INFO_SERIAL_NUM_XLCD::to_array() {
    Buff tmp = {};
    buddy::hw::Configuration &cnf = buddy::hw::Configuration::Instance();
    memcpy(tmp.data(), cnf.get_xlcd().datamatrix, sizeof(XlcdEeprom::datamatrix));
    snprintf(tmp.data() + sizeof(XlcdEeprom::datamatrix), tmp.size() - sizeof(XlcdEeprom::datamatrix), "/%u", cnf.get_xlcd().bomID);
    return tmp;
}

MI_XLCD_SINGLE_ERR::MI_XLCD_SINGLE_ERR()
    : WiSpinInt(buddy::hw::Configuration::Instance().get_xlcd_status().single_read_error_counter, SpinCnf::int_num, _(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {}

MI_XLCD_REPEATED_ERR::MI_XLCD_REPEATED_ERR()
    : WiSpinInt(buddy::hw::Configuration::Instance().get_xlcd_status().repeated_read_error_counter, SpinCnf::int_num, _(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {}

MI_XLCD_CYCLIC_ERR::MI_XLCD_CYCLIC_ERR()
    : WiSpinInt(buddy::hw::Configuration::Instance().get_xlcd_status().cyclic_read_error_counter, SpinCnf::int_num, _(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {}

MI_XLCD_RETRIED::MI_XLCD_RETRIED()
    : WiSpinInt(buddy::hw::Configuration::Instance().get_xlcd_status().retried, SpinCnf::int_num, _(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {}

MI_XLCD_STATUS::MI_XLCD_STATUS()
    : WI_INFO_DEV_t(_(label), nullptr) {
    ChangeInformation(buddy::hw::Configuration::Instance().get_xlcd_status().data_valid ? "DETECTED" : "NOT DETECTED");
}
