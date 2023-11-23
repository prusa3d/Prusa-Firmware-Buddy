// screen_reset_perror.hpp
#pragma once
#include "gui.hpp"
#include "window_text.hpp"
#include "screen.hpp"

/**
 * @brief Generic screen shown after reset caused by error.
 * Reads error details from dump.
 */
class ScreenResetError : public AddSuperWindow<screen_t> {
    window_text_t fw_version_txt;
    window_text_t signature_txt;
    window_text_t appendix_txt;
    bool sound_started;

public:
    ScreenResetError();

    static const constexpr Rect16 title_rect = GuiDefaults::EnableDialogBigLayout ? Rect16(30, 44, display::GetW() - 60, 20) : Rect16(10, 24, display::GetW() - 26, 20);
    static const constexpr Rect16 fw_version_rect = GuiDefaults::EnableDialogBigLayout ? Rect16(30, 265, 150, 20) : Rect16(6, 295, 80, 13);
    static const constexpr Rect16 signature_rect = GuiDefaults::EnableDialogBigLayout ? Rect16(320, 265, 40, 20) : Rect16(160, 295, 40, 13);
    static const constexpr Rect16 appendix_rect = GuiDefaults::EnableDialogBigLayout ? Rect16(410, 265, 40, 20) : Rect16(195, 295, 40, 13);

protected:
    /// starts sound and avoids repetitive starting
    void start_sound();
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};
