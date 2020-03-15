#pragma once

#include <stdint.h>

//interface for dialog
class IDialog {
public:
    virtual void Change(uint8_t phase, uint8_t progress_tot, uint8_t progress) = 0;
    virtual ~IDialog() {}
};
