#include "MItem_MK3.5.hpp"

MI_PINDA::MI_PINDA()
    : WI_SWITCH_0_1_NA_t(get_state(), _(label), nullptr, is_enabled_t::no, is_hidden_t::no) {
}

MI_PINDA::state_t MI_PINDA::get_state() {
    return (buddy::hw::zMin.read() == buddy::hw::Pin::State::low) ? state_t::low : state_t::high;
}

void MI_PINDA::Loop() {
    SetIndex((size_t)get_state());
}
