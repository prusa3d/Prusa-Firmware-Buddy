#pragma once

#include "align.hpp"
#include "Rect16.h"
#include "window.hpp"
#include <array>
#include <common/str_utils.hpp>

/**
 * GUI widget for displaying QR codes with static string.
 */
class QRStaticStringWindow : public window_aligned_t {
private:
    const char *data;

public:
    QRStaticStringWindow(window_t *parent, Rect16 rect, Align_t align, const char *data);

    virtual void unconditionalDraw() override;
};

/**
 * GUI widget for displaying QR codes with dynamic string.
 * Buffer for the string has N bytes and is contained
 * within QRDynamicStringWindow itself.
 */
template <size_t N>
class QRDynamicStringWindow : public QRStaticStringWindow {
protected:
    std::array<char, N> buffer;

public:
    QRDynamicStringWindow(window_t *parent, Rect16 rect, Align_t align)
        : QRStaticStringWindow(parent, rect, align, buffer.data()) {}

    /**
     * Get string builder to manipulate internal buffer. Make sure the widget
     * is in 'invalidated' state after changing the buffer, either by calling
     * Invalidate() or by some other means.
     */
    StringBuilder get_string_builder() { return StringBuilder { buffer }; }

    operator const char *() const { return buffer.data(); }
};

/**
 * GUI widget for displaying QR codes for specific error code.
 */
class QRErrorUrlWindow : public QRDynamicStringWindow<44> {
public:
    QRErrorUrlWindow(window_t *parent, Rect16 rect, ErrCode ec);

    void set_error_code(ErrCode ec);
};
