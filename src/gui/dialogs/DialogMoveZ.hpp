#pragma once
#include "gui.hpp"
#include "IDialog.hpp"
#include "window_header.hpp"
#include "screen.hpp"
#include "display.h"
#include "window_text.hpp"
#include "window_numb.hpp"
#include "window_menu_adv.hpp"
#include "WinMenuContainer.hpp"
#include "window_arrows.hpp"

class DialogMoveZ : public AddSuperWindow<IDialog> {
private:
    static bool DialogShown;
    constexpr static const char *const headerLabel = N_("Z AXIS MOVE");
    constexpr static const char *const axisLabel = N_("Z-axis");
#if (PRINTER_IS_PRUSA_XL || PRINTER_IS_PRUSA_iX) // XL moves bed down while Z goes up
    constexpr static const char *const infoTextContent = N_("Turn the knob to move Heatbed");
#else /*PRINTER_TYPE*/
    constexpr static const char *const infoTextContent = N_("Turn the knob to move Z-axis");
#endif /*PRINTER_TYPE*/
    constexpr static const char *const closeTextContent = N_("Click the knob to close");
    constexpr static const char *const upTextContent = N_("UP");
    constexpr static const char *const downTextContent = N_("DOWN");
    float prev_accel;

    int32_t value;
    float lastQueuedPos;

    window_text_t axisText;
    window_text_t infoText;
    window_text_t closeText;
    window_text_t rightText;
    window_text_t leftText;
    WindowArrows arrows;

    window_numb_t numb;
    window_header_t header;
    window_icon_t icon;

#ifdef USE_ST7789
    static constexpr Rect16 infoText_rc { 0, 45, 240, 60 };
    static constexpr Rect16 closeText_rc { 0, 90, 240, 60 };
    static constexpr Rect16 icon_rc { 80, 154, 81, 55 };
    static constexpr Rect16 rightText_rc { 120 + 47, 171, 56, 21 };
    static constexpr Rect16 leftText_rc { 120 - 47 - 56, 171, 56, 21 };
    static constexpr Rect16 text_rc { 0, 223, 114, 21 };
    static constexpr Rect16 numb_rc { 126, 223, 114, 21 };
    static constexpr Rect16 arrows_rc { 113, 223, 14, 21 };
#endif
#ifdef USE_ILI9488
    static constexpr Rect16 infoText_rc { 0, 45, 480, 60 };
    static constexpr Rect16 closeText_rc { 0, 90, 480, 60 };
    static constexpr Rect16 icon_rc { 200, 171, 81, 55 };
    static constexpr Rect16 rightText_rc { 240 + 51, 187, 64, 22 };
    static constexpr Rect16 leftText_rc { 240 - 51 - 64, 187, 64, 22 };
    static constexpr Rect16 text_rc { 0, 240, 237, 22 };
    static constexpr Rect16 numb_rc { 243, 240, 237, 22 };
    static constexpr Rect16 arrows_rc { 236, 240, 8, 22 };
#endif
    void change(int diff);
    Rect16 getNumbRect(Rect16 rect) const;

    DialogMoveZ();

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;

public:
    static void Show();
    ~DialogMoveZ();
};
