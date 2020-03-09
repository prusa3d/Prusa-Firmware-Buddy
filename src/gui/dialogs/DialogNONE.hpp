#pragma once

#include "IDialog.hpp"

//no dialog is needed as initial value to static union
class DialogNONE : public IDialog {
public:
    virtual void Change(uint8_t phase, uint8_t progress_tot, uint8_t progress) {}
};
