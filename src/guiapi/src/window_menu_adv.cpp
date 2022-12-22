/**
 * @file window_menu_adv.cpp
 */

#include <algorithm>
#include <cstdlib>
#include "window_menu_adv.hpp"

static constexpr Rect16 rc_menu(Rect16 rc) {
    rc.CutPadding(GuiDefaults::MenuPadding);
    rc -= GuiDefaults::MenuIcon_w;
    return rc;
}

static constexpr Rect16 rc_arrow_up(Rect16 rc) {
    Rect16::Left_t left = rc.Left() + rc.Width() - GuiDefaults::MenuIcon_w;
    return { left, rc.Top(), GuiDefaults::MenuIcon_w, GuiDefaults::MenuIcon_h };
}

static constexpr Rect16 rc_arrow_down(Rect16 rc) {
    Rect16 ret = rc_arrow_up(rc);
    ret += Rect16::Top_t(rc.Height() - GuiDefaults::MenuIcon_h);
    return ret;
}

static constexpr Rect16 rc_bar(Rect16 rc) {
    Rect16 ret = rc_arrow_up(rc);
    ret += Rect16::Top_t(GuiDefaults::MenuIcon_h);
    ret += Rect16::Left_t(ret.Width() - GuiDefaults::MenuScrollbarWidth);
    ret = GuiDefaults::MenuScrollbarWidth;
    ret = Rect16::Height_t(rc.Height() - GuiDefaults::MenuIcon_h * 2);
    return ret;
}

WindowMenuAdv::WindowMenuAdv(window_t *parent, Rect16 rect, IWinMenuContainer *pContainer, uint8_t index)
    : AddSuperWindow<window_frame_t>(parent, rect)
    , menu(parent, rc_menu(rect), pContainer, index)

    , bar(this, rc_bar(rect), menu) // event loop will handle everything
{}

void WindowMenuAdv::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {

    switch (event) {
    // no need to resend GUI_event_t::LOOP and GUI_event_t::TEXT_ROLL (the are broadcasted)
    // and GUI_event_t::CHILD_CLICK: which is targetted to menu directly
    case GUI_event_t::CLICK:
    case GUI_event_t::ENC_DN:
    case GUI_event_t::ENC_UP:
        menu.WindowEvent(sender, event, param);
        break;
    default:
        // discard other events
        break;
    }
}

WindowFileBrowserAdv::WindowFileBrowserAdv(window_t *parent, Rect16 rect, const char *media_SFN_path)
    : AddSuperWindow<window_frame_t>(parent, rect, win_type_t::normal)
    , file_browser(parent, rc_menu(rect), media_SFN_path)
//, bar(this, rc_bar(rect), menu) // event loop will handle everything
{
    CaptureNormalWindow(file_browser);
    file_browser.SetFocus();
}

void WindowFileBrowserAdv::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {

    switch (event) {
    // no need to resend GUI_event_t::LOOP and GUI_event_t::TEXT_ROLL (the are broadcasted)
    // and GUI_event_t::CHILD_CLICK: which is targetted to menu directly
    case GUI_event_t::CLICK:
    case GUI_event_t::ENC_DN:
    case GUI_event_t::ENC_UP:
        file_browser.WindowEvent(sender, event, param);
        break;
    default:
        // discard other events
        break;
    }
}
