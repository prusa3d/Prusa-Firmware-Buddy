#pragma once

#include "IDialog.hpp"
#include <common/fsm_base_types.hpp>

/**
 * Parent class for all dialogs usable from DialogHandler.
 * Provides a hook for subscribing to FSM changes.
 */
class IDialogMarlin : public IDialog {
public:
    IDialogMarlin(Rect16 rect)
        : IDialog(rect) {}

    virtual void Change(fsm::BaseData) {}
};
