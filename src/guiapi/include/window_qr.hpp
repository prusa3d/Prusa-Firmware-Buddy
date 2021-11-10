// window_qr.hpp

#pragma once

#include "window.hpp"
#include "qrcodegen.h"
#include "support_utils.h"

class window_qr_t : public AddSuperWindow<window_t> {
    char text[MAX_LEN_4QR + 1];
    // 8 bit
    uint8_t border;        /// border size in pixels; same for all sides
    uint8_t px_per_module; /// width/height of module (single colored square)
    Align_t align;         /// alignment of QR code in the window
    // other
    bool scale; /// changes px_per_module so the QR code is the biggest that fits in the window

    /// Defines maximal size of QR code and buffers needed for generating. Keep it low.
    static constexpr uint8_t qr_version_max = 9;

public:
    window_qr_t(window_t *parent, Rect16 rect, uint16_t err_num);

protected:
    virtual void unconditionalDraw() override;
};
