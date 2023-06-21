/**
 * @file selftest_sub_state.hpp
 * @author Radek Vana
 * @date 2021-03-03
 */

#pragma once

enum class SelftestSubtestState_t : uint8_t { // it is passed as uint8_t between threads
    undef,
    ok,
    not_good,
    running
};
