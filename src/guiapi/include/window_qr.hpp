#pragma once

#include "align.hpp"
#include "Rect16.h"

struct DrawQROptions {
    const char *data;
    Rect16 rect;
    Align_t align;
};
void draw_qr(const DrawQROptions &);
