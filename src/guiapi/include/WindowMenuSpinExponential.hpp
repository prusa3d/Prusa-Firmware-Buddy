/**
 * @file WindowMenuSpinExponential.hpp
 * @author Radek Vana
 * @brief Exponential version of Spinner menu item
 * @date 2021-08-01
 */

#pragma once

#include "WindowMenuSpin.hpp"

/**
 * @brief Exponential switch
 * step is not added but multiplies current value
 * with step == 2 we have values 1,2,4,8 ...
 */
class WiSpinExp : public WiSpinInt {
public:
    WiSpinExp(int val, const Config &cnf, string_view_utf8 label, const img::Resource *id_icon = nullptr, is_enabled_t enabled = is_enabled_t::yes, is_hidden_t hidden = is_hidden_t::no);
    virtual invalidate_t change(int dif) override;

    static int exponential_change(int dif, int val, const Config &cnf);
};

/**
 * @brief Exponential switch with support of 0
 * not inherited to support config.Min() <= 0 check in WiSpinExp
 */
class WiSpinExpWith0 : public WiSpinInt {
public:
    WiSpinExpWith0(int val, const Config &cnf, string_view_utf8 label, const img::Resource *id_icon = nullptr, is_enabled_t enabled = is_enabled_t::yes, is_hidden_t hidden = is_hidden_t::no);
    virtual invalidate_t change(int dif) override;
};
