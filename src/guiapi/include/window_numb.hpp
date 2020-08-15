// window_numb.hpp

#pragma once

#include "window.hpp"

struct window_numb_t : public window_t {
    color_t color_text;
    font_t *font;
    float value;
    const char *format;
    padding_ui8_t padding;
    uint8_t alignment;

    void SetFormat(const char *frmt);
    const char *GetFormat() { return format; }
    void SetValue(float val);
    void SetFont(font_t *val);
    float GetValue() const { return value; }
    void SetColor(color_t clr);
    window_numb_t(window_t *parent, Rect16 rect, float value = 0);

    void PrintAsFloat();
    void PrintAsInt();
    bool IsPrintingAsInt() const;

protected:
    virtual void unconditionalDraw() override;
    virtual void setValue(float val);
};
