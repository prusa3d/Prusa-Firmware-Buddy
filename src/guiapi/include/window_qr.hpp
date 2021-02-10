// window_qr.hpp

#pragma once

#include "window.hpp"
#include "qrcodegen.h"

struct window_qr_t : public window_t {
    // 32 bit
    color_t bg_color = color_t::White; /// background color
    color_t px_color = color_t::Black; /// foreground color
    char *text = nullptr;              /// border size in pixels; same for all sides
    // 8 bit
    uint8_t border = 4;                /// border size in pixels; same for all sides
    uint8_t px_per_module = 2;         /// width/height of module (single colored square)
    Align_t align = Align_t::Center(); /// alignment of QR code in the window
    // other
    bool scale = true; /// changes px_per_module so the QR code is the biggest that fits in the window
    window_qr_t(window_t *parent, Rect16 rect);
};

/// Defines maximal size of QR code and buffers needed for generating. Keep it low.
constexpr uint8_t qr_version_max = 9;

/// Takes \param text and writes QR code into \param qrcode.
/// \returns true if QR code was generated.
/// Use this to allocate \param qrcode buffer:
/// uint8_t qrcode[qrcodegen_BUFFER_LEN_FOR_VERSION(qr_version_max)];
/// \param buffer you can provide your buffer of the size of \param qrcode
bool generate_qr(const char *text, uint8_t qrcode[]);
bool generate_qr(const char *text, uint8_t qrcode[], uint8_t buffer[]);

/// \returns number of modules of QR code (\param qrcode).
/// Does not include size of border.
/// Size is always the same for X and Y dimensions.
int get_qr_size(uint8_t *qrcode);

/// \returns size of QR code (\param qrcode) in pixels.
/// Includes size of borders.
/// Size is always the same for X and Y dimensions.
int get_qr_px_size(uint8_t qrcode[], const window_qr_t *const window);

/// \returns size of QR code (\param qrcode) in pixels.
/// Uses \param px_per_module. Size includes borders.
/// Size is always the same for X and Y dimensions.
int get_qr_px_size(uint8_t qrcode[], const window_qr_t *const window, uint16_t px_per_module);

void draw_qr(uint8_t qrcode[], const window_qr_t *const window);
