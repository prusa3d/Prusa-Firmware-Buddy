/**
 * @file screen_dialog_does_not_exist.hpp
 * @author Radek Vana
 * @brief Empty screen to be bound to not existing dialogs or screens
 * @date 2021-11-05
 */
#pragma once

#include "screen.hpp"
#include <common/fsm_base_types.hpp>
#include "window_text.hpp"

class ScreenDoesNotExist : public screen_t {
protected:
    window_text_t txt;

public:
    ScreenDoesNotExist();
};

class ScreenDialogDoesNotExist : public ScreenDoesNotExist {

public:
    ScreenDialogDoesNotExist();
    constexpr bool Change([[maybe_unused]] fsm::BaseData data) { return false; }
};
