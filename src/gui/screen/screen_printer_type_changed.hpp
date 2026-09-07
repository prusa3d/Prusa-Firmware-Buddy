/// @file
#pragma once

#include <gui.hpp>
#include <radio_button.hpp>
#include <screen.hpp>
#include <window_icon.hpp>
#include <window_text.hpp>

/// Informs the user that the printer type has changed since the last boot
/// and that a factory reset has to be performed because of it.
class ScreenPrinterTypeChanged final : public screen_t {
public:
    ScreenPrinterTypeChanged();

    /// \returns whether the printer type changed since the last boot.
    /// Initializes the stored printer type when it is not set up yet (first boot).
    [[nodiscard]] static bool should_show();

protected:
    void windowEvent(window_t *sender, GUI_event_t event, void *param) override;

private:
    window_icon_t icon;
    window_text_t title;

    window_text_t model_from;
    window_icon_t arrow;
    window_text_t model_to;

    window_text_t description;
    RadioButton radio;
};
