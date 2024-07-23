#pragma once
#include "gui.hpp"
#include "IDialog.hpp"
#include "window_header.hpp"
#include "screen.hpp"
#include "window_text.hpp"
#include "window_numb.hpp"
#include "window_menu_adv.hpp"
#include "WinMenuContainer.hpp"
#include "window_arrows.hpp"
#include <guiconfig/guiconfig.h>

class DialogMoveZ : public IDialog {
private:
    static bool DialogShown;
    constexpr static const char *const headerLabel = N_("Z AXIS MOVE");
    constexpr static const char *const axisLabel = N_("Z-axis");
#if (PRINTER_IS_PRUSA_XL() || PRINTER_IS_PRUSA_iX()) // XL moves bed down while Z goes up
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

    /*To avoid cropping text due to small rectangles it is necessary to size them accordingly. This count represents currently longest text applicable among all translations.

    Additionally, because XL and iX have directions switched, it is also necessary to resize both sides to the longest. Alternatively it could be done via #ifdef clause, since it causes no issues this way, the code and the code is more readable it was resized on both sides without checking printer type.

    Currently longest was Czech NAHORU (just like ARRIBA or RUNTER).*/
    static constexpr uint8_t longest_text_char_count { 6 };
    static constexpr Font font { GuiDefaults::FontBig };
    static constexpr font_size_t font_size { resource_font_size(font) };

#if HAS_MINI_DISPLAY()
    static constexpr Rect16 infoText_rc { 0, 45, 240, 60 };
    static constexpr Rect16 closeText_rc { 0, 90, 240, 60 };
    static constexpr Rect16 icon_rc { 80, 154, 81, 55 };
    static constexpr Rect16 rightText_rc { 120 + 47, 171, 70, 21 };
    static constexpr Rect16 leftText_rc { 120 - 47 - 56, 171, 56, 21 };
    static constexpr Rect16 text_rc { 0, 223, 114, 21 };
    static constexpr Rect16 numb_rc { 126, 223, 114, 21 };
    static constexpr Rect16 arrows_rc { 113, 223, 14, 21 };
#endif
#if HAS_LARGE_DISPLAY()
    static constexpr Rect16 infoText_rc { 0, 45, 480, 60 };
    static constexpr Rect16 closeText_rc { 0, 90, 480, 60 };
    static constexpr Rect16 icon_rc { 200, 171, 81, 55 };
    static constexpr Rect16 rightText_rc { 240 + 51, 187, font_size.w *longest_text_char_count, font_size.h };
    static constexpr Rect16 leftText_rc { 240 - 51 - font_size.w * longest_text_char_count, 187, font_size.w *longest_text_char_count, font_size.h };
    static constexpr Rect16 text_rc { 0, 240, 237, 22 };
    static constexpr Rect16 numb_rc { 243, 240, 237, 22 };
    static constexpr Rect16 arrows_rc { 236, 240, 8, 22 };
#endif
    void change(int diff);
    Rect16 getNumbRect(Rect16 rect) const;

    DialogMoveZ();

protected:
    virtual void windowEvent(window_t *sender, GUI_event_t event, void *param) override;

public:
    static void Show();
    ~DialogMoveZ();
};
