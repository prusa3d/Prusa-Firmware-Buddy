#pragma once

#include <stdint.h>
#include "window.h"

//window_t has pragma pack, have to use here too
#pragma pack(push)
#pragma pack(1)
//interface for dialog
class IDialog : public window_t {
public:
    IDialog(window_t win);
    virtual void Change(uint8_t phase, uint8_t progress_tot, uint8_t progress) = 0;
    virtual ~IDialog() {}
};
#pragma pack(pop)
