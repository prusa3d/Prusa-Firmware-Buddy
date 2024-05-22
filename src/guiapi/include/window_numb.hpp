// window_numb.hpp

#pragma once

#include "i_window_text.hpp"
#include <guiconfig/GuiDefaults.hpp>

enum class printType {
    asInt32,
    asFloat,
    asUint32,
    asTime
};

class window_numb_t : public AddSuperWindow<IWindowText> {
public:
    float value; // TODO private
    const char *format; // TODO private
    printType printAs; // TODO private

    void SetFormat(const char *frmt);
    const char *GetFormat() { return format; }
    void SetValue(float val);
    float GetValue() const { return value; }
    void SetColor(color_t clr);
    window_numb_t(window_t *parent, Rect16 rect, float value = 0, const char *frmt = nullptr, Font font = GuiDefaults::DefaultFont);
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
