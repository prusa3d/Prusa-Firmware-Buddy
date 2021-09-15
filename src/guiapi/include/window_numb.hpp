// window_numb.hpp

#pragma once

#include "window.hpp"

enum class printType {
    asInt32,
    asFloat,
    asUint32,
    asTime
};

struct window_numb_t : public window_aligned_t {
    color_t color_text;
    font_t *font;
    float value;
    const char *format;
    padding_ui8_t padding;
    printType printAs;

    void SetFormat(const char *frmt);
    const char *GetFormat() { return format; }
    void SetValue(float val);
    void SetFont(font_t *val);
    float GetValue() const { return value; }
    void SetColor(color_t clr);
    window_numb_t(window_t *parent, Rect16 rect, float value = 0, const char *frmt = nullptr);
    void PrintTime(char *buffer);

    void PrintAsFloat();
    void PrintAsInt32();
    void PrintAsUint32();
    void PrintAsTime();
    bool IsPrintingAsInt() const;

protected:
    virtual void unconditionalDraw() override;
    virtual void setValue(float val);
};
