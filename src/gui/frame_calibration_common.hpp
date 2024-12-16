/// @file
#pragma once

#include <common/fsm_base_types.hpp>
#include <guiconfig/wizard_config.hpp>
#include <option/has_attachable_accelerometer.h>
#include "window_text.hpp"

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
