/**
 * @file WindowMenuSpinExponential.hpp
 * @author Radek Vana
 * @date 2021-08-01
 */

#include "WindowMenuSpinExponential.hpp"

WiSpinExp::WiSpinExp(int val, const Config &cnf, string_view_utf8 label, uint16_t id_icon, is_enabled_t enabled, is_hidden_t hidden)
    : AddSuper<WiSpinInt>(val, cnf, label, id_icon, enabled, hidden) {}

invalidate_t WiSpinExp::change(int dif) {
    if (dif != 0) {
        int val = GetVal();

        //I don't want to use pow(), because it works with floats
        //step will be in 99% 0, 1 or -1 .. so it is fine

        if (dif > 0) {
            while ((--dif) >= 0) {
                val *= config.Step();
            }
        } else {
            while ((++dif) <= 0) {
                val /= config.Step();
            }
        }

        value = val;
    }
    return super::change(0); // parent must handle it .. just invalidation and check of limits
}
