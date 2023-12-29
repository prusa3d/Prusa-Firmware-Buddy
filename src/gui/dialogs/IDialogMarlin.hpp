#pragma once

#include "IDialog.hpp"
#include "fsm_types.hpp"

/**
 * Parent class for all dialogs usable from DialogHandler.
 * Provides a hook for subscribing to FSM changes.
 */
class IDialogMarlin : public IDialog {
public:
    IDialogMarlin(Rect16 rect)
        : IDialog(rect) {}

    virtual bool Change(fsm::BaseData) { return true; }
};
