#pragma once
#include "gui.hpp"
#include "IDialog.hpp"
#include "window_header.hpp"
#include "screen.hpp"
#include "display.h"
#include "window_text.hpp"
#include "window_numb.hpp"
#include "window_menu.hpp"
#include "WinMenuContainer.hpp"
#include "window_arrows.hpp"

class DialogMoveZ : public AddSuperWindow<IDialog> {
private:
    constexpr static const char *const headerLabel = N_("Z AXIS MOVE");
    constexpr static const char *const axisLabel = N_("Z-axis");
    constexpr static const char *const infoTextContent = N_("Turn the knob to change Z-axis");
    constexpr static const char *const closeTextContent = N_("Click knob to end");
    constexpr static const char *const upTextContent = N_("UP");
    constexpr static const char *const downTextContent = N_("DOWN");

    int32_t value;
    int32_t lastQueuedPos;

    window_text_t axisText;
    window_text_t infoText;
    window_text_t closeText;
    window_text_t upText;
    window_text_t downText;
    WindowArrows arrows;

    window_numb_t numb;
    window_header_t header;
    window_icon_t icon;

    static constexpr Rect16 infoText_rc { 0, 45, 240, 60 };
    static constexpr Rect16 closeText_rc { 0, 90, 240, 60 };
    static constexpr Rect16 icon_rc { 80, 154, 81, 55 };
    static constexpr Rect16 upText_rc { 167, 171, 30, 21 };
    static constexpr Rect16 downText_rc { 20, 171, 56, 21 };
    static constexpr Rect16 text_rc { 0, 223, 114, 21 };
    static constexpr Rect16 numb_rc { 126, 223, 114, 21 };
    static constexpr Rect16 arrows_rc { 113, 223, 14, 21 };

    void change(int diff);
    Rect16 getNumbRect(Rect16 rect) const;

    DialogMoveZ();

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;

public:
    static void Show();
};
