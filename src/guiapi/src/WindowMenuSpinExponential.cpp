/**
 * @file WindowMenuSpinExponential.hpp
 * @author Radek Vana
 * @date 2021-08-01
 */

#include "WindowMenuSpinExponential.hpp"

WiSpinExp::WiSpinExp(int val, const Config &cnf, string_view_utf8 label, const img::Resource *id_icon, is_enabled_t enabled, is_hidden_t hidden)
    : WiSpinInt(val, cnf, label, id_icon, enabled, hidden) {
    if (config.Min() <= 0) {
        bsod("WiSpinExp requires minimal value to be > 0, use WiSpinExpWith0 instead");
    }
}

invalidate_t WiSpinExp::change(int dif) {
    set_val(exponential_change(dif, GetVal(), config));
    return WiSpinInt::change(0); // parent must handle it .. just invalidation and check of limits
}

int WiSpinExp::exponential_change(int dif, int val, const Config &cnf) {
    if (dif != 0) {
        // I don't want to use pow(), because it works with floats
        // step will be in 99% 0, 1 or -1 .. so it is fine

        if (dif > 0) {
            while ((--dif) >= 0) {
                val *= cnf.Step();
            }
        } else {
            while ((++dif) <= 0) {
                val /= cnf.Step();
            }
        }
    }
    return val;
}

WiSpinExpWith0::WiSpinExpWith0(int val, const Config &cnf, string_view_utf8 label, const img::Resource *id_icon, is_enabled_t enabled, is_hidden_t hidden)
    : WiSpinInt(val, cnf, label, id_icon, enabled, hidden) {
    if (config.Min() != 0) {
        bsod("WiSpinExpWith0 requires minimal value to be 0, use WiSpinExp instead");
    }
}

invalidate_t WiSpinExpWith0::change(int dif) {
    int val = GetVal();

    if (dif > 0) {
        // when current value is 0, we need to increment it to x^0 == 1
        if (val == 0) {
            val = 1;
            --dif;
        }
    }

    set_val(WiSpinExp::exponential_change(dif, val, config));
    return WiSpinInt::change(0);
}
