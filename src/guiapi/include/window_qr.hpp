// window_qr.hpp

#pragma once

#include "window.hpp"

struct DrawQROptions {
    const char *data;
    Rect16 rect;
    Align_t align;
};
void draw_qr(const DrawQROptions &);

class window_qr_t : public window_t {
public:
    window_qr_t(window_t *parent, Rect16 rect, uint16_t error_num, Align_t align = Align_t::Center());
    window_qr_t(window_t *parent, Rect16 rect, const char *txt);
    window_qr_t(window_t *parent, Rect16 rect, Align_t align = Align_t::LeftTop());

    static constexpr uint16_t MAX_LEN_4QR = 256;
    void SetQRHeader(uint16_t err_num);
    void SetText(const char *txt);
    const char *GetQRShortText();
    const char *GetQRLongText();

protected:
    // QR Header vars
    char text[MAX_LEN_4QR + 1];
    uint16_t error_num;
    Align_t align; /// alignment of QR code in the window

protected:
    virtual void unconditionalDraw() override;
};
