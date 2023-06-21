/**
 * @file screen_menu_no_tools.cpp
 */

#include "screen_menu_no_tools.hpp"

MI_INFO_NOZZLE_TEMP::MI_INFO_NOZZLE_TEMP()
    : WI_TEMP_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}
