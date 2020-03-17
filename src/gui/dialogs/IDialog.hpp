#pragma once

#include <stdint.h>
#include "window.h"

//window_t has pragma pack, have to use here too
#pragma pack(push)
#pragma pack(1)
//interface for dialog
class IDialog : protected window_t {
protected:
    int16_t WINDOW_CLS;

public:
    IDialog(int16_t WINDOW_CLS_);
    //virtual void Change(uint8_t phase, uint8_t progress_tot, uint8_t progress) = 0;
    virtual ~IDialog() {}

    static IDialog *cast(window_t *win_addr);
};
#pragma pack(pop)
