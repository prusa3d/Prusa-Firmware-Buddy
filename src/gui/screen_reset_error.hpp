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
    bool sound_started;

public:
    ScreenResetError(const Rect16 &fw_version_rect);

    static const constexpr Rect16 title_rect = GuiDefaults::EnableDialogBigLayout ? Rect16(30, 44, display::GetW() - 60, 20) : Rect16(10, 24, display::GetW() - 26, 20);

protected:
    /// starts sound and avoids repetitive starting
    void start_sound();
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
    void update_error_code(uint16_t &error_code); ///< distinguish MK3.9 from MK4 and update error_code's printer prefix
};
