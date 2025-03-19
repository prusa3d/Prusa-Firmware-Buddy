/// @file
#pragma once

#include <common/fsm_base_types.hpp>
#include <guiconfig/wizard_config.hpp>
#include <option/has_attachable_accelerometer.h>
#include "window_text.hpp"
#include <gui/qr.hpp>
#include <window_icon.hpp>

inline constexpr auto rect_screen = WizardDefaults::RectSelftestFrame;
inline constexpr auto rect_radio = WizardDefaults::RectRadioButton(0);
inline constexpr auto rect_frame = Rect16 {
    rect_screen.Left() + WizardDefaults::MarginLeft,
    rect_screen.Top() + WizardDefaults::row_0,
    rect_screen.Width() - (WizardDefaults::MarginLeft + WizardDefaults::MarginRight),
    rect_radio.Top() - rect_screen.Top() - WizardDefaults::row_0
};
inline constexpr auto rect_frame_top = Rect16 {
    rect_frame.Left(),
    rect_frame.Top(),
    rect_frame.Width(),
    WizardDefaults::row_h,
};
inline constexpr auto rect_frame_bottom = Rect16 {
    rect_frame.Left(),
    rect_frame.Top() + WizardDefaults::progress_row_h + WizardDefaults::row_h,
    rect_frame.Width(),
    WizardDefaults::row_h,
};

class FrameText {
protected:
    FrameText(window_t *parent, string_view_utf8 txt, const Rect16::Top_t top)
        : text(parent, Rect16(WizardDefaults::col_0, top, WizardDefaults::RectSelftestFrame.Width() - WizardDefaults::MarginRight - WizardDefaults::MarginLeft, WizardDefaults::row_h * 8), is_multiline::yes, is_closed_on_click_t::no, txt) {}

private:
    window_text_t text;
};

class FrameTextWithQR {
protected:
    FrameTextWithQR(window_t *parent, string_view_utf8 txt, Rect16::Top_t top, const char *qr_link)
        : text(parent, Rect16(WizardDefaults::col_0, top, WizardDefaults::RectSelftestFrame.Width() - WizardDefaults::MarginLeft - WizardDefaults::MarginRight - GuiDefaults::QRSize, WizardDefaults::row_h * 8), is_multiline::yes, is_closed_on_click_t::no, txt)
        , qr(parent, Rect16(WizardDefaults::RectSelftestFrame.Width() - WizardDefaults::MarginRight - GuiDefaults::QRSize, WizardDefaults::row_1, GuiDefaults::QRSize, GuiDefaults::QRSize), Align_t::CenterTop(), qr_link) {}

private:
    window_text_t text;
    QRStaticStringWindow qr;
};

class FrameTextWithImage {
protected:
    FrameTextWithImage(window_t *parent, string_view_utf8 txt, Rect16::Top_t top, const img::Resource *icon_res, uint16_t icon_width)
        : text(parent, Rect16(WizardDefaults::col_0, top, WizardDefaults::RectSelftestFrame.Width() - WizardDefaults::MarginLeft - WizardDefaults::MarginRight - icon_width, WizardDefaults::row_h * 8), is_multiline::yes, is_closed_on_click_t::no, txt)
        , icon(parent, icon_res, point_i16_t(WizardDefaults::RectSelftestFrame.Width() - WizardDefaults::MarginRight - icon_width, top)) {}

private:
    window_text_t text;
    window_icon_t icon;
};

class FrameInstructions {
private:
    window_text_t text;

protected:
    FrameInstructions(window_t *parent, const string_view_utf8 &txt);

public:
    void update(fsm::PhaseData);
};

#if HAS_ATTACHABLE_ACCELEROMETER()

class FrameConnectToBoard final : public FrameInstructions {
public:
    explicit FrameConnectToBoard(window_t *parent);
};

class FrameWaitForExtruderTemperature final {
    window_text_t text_above;
    WindowBlinkingText text_below;
    std::array<char, sizeof("NNN °C")> text_below_buffer;

public:
    explicit FrameWaitForExtruderTemperature(window_t *parent);

    void update(fsm::PhaseData data);
};

class FrameAttachToExtruder final : public FrameInstructions {
public:
    explicit FrameAttachToExtruder(window_t *parent);
};

class FrameAttachToBed final : public FrameInstructions {
public:
    explicit FrameAttachToBed(window_t *parent);
};

#endif
