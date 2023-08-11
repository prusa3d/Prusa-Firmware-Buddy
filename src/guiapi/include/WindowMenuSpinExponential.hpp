/**
 * @file WindowMenuSpinExponential.hpp
 * @author Radek Vana
 * @brief Exponential version of Spinner menu item
 * @date 2021-08-01
 */

#pragma once

#include "WindowMenuSpin.hpp"

class WiSpinExp : public AddSuper<WiSpinInt> {
public:
    WiSpinExp(int val, const Config &cnf, string_view_utf8 label, const img::Resource *id_icon = nullptr, is_enabled_t enabled = is_enabled_t::yes, is_hidden_t hidden = is_hidden_t::no);
    virtual invalidate_t change(int dif) override;
};
