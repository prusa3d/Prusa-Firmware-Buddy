// wizard_config.hpp
#pragma once
#include "eeprom.h" // SelftestResult_Passed, SelftestResult_Failed

enum class SelftestSubtestState_t : uint8_t { //it is passed as uint8_t between threads
    undef,
    ok,
    not_good,
    running
};

constexpr SelftestSubtestState_t SelftestStateFromEeprom(uint8_t state) {
    switch (state) {
    case SelftestResult_Passed:
        return SelftestSubtestState_t::ok;
    case SelftestResult_Failed:
        return SelftestSubtestState_t::not_good;
    default:
        return SelftestSubtestState_t::undef;
    }
}

struct WizardDefaults {
    static constexpr size_t MarginLeft = 6;
    static constexpr size_t MarginRight = 6;
    static constexpr size_t X_space = 240 - (WizardDefaults::MarginLeft + WizardDefaults::MarginRight);
    static constexpr size_t txt_h = 22;
    static constexpr size_t row_h = 24;
    static constexpr size_t status_icon_X_pos = 210; // todo calculate
    static constexpr size_t row_0 = 40;
    static constexpr size_t separator_width = 1;
    static constexpr size_t separator_padding_bottom = 3;
    static constexpr size_t progress_h = 3;
    static constexpr size_t progress_row_h = progress_h + 3;
    static constexpr size_t progress_LR_margin = 10;
    static constexpr size_t progress_width = 240 - 2 * progress_LR_margin;
};
