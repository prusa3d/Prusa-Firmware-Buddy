#pragma once

#include <stdint.h>
#include "window.hpp"

//window_t has pragma pack, have to use here too
//interface for dialog
class IDialog : protected window_t {
    window_t *id_capture;

public:
    IDialog();
    virtual ~IDialog();
};
