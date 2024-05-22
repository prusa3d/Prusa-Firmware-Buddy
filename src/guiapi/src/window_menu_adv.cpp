/**
 * @file window_menu_adv.cpp
 */

#include <algorithm>
#include <cstdlib>
#include "window_menu_adv.hpp"
#include "img_resources.hpp"
#include "marlin_client.hpp"

static Rect16 rc_menu(Rect16 rc) {
    rc.CutPadding(GuiDefaults::MenuPadding);
    rc -= GuiDefaults::MenuScrollbarWidth;
    return rc;
}

static Rect16 rc_bar(Rect16 rc) {
    Rect16 ret = rc;
    ret += Rect16::Left_t(ret.Width() - GuiDefaults::MenuScrollbarWidth);
    ret = GuiDefaults::MenuScrollbarWidth;
    ret = Rect16::Height_t(rc.Height());
    return ret;
}

WindowMenuAdv::WindowMenuAdv(window_t *parent, Rect16 rect, IWinMenuContainer *pContainer)
    : AddSuperWindow<window_frame_t>(parent, rect)
    , menu(parent, rc_menu(rect), pContainer)
    , bar(this, rc_bar(rect), menu) // event loop will handle everything
{
}

void WindowMenuAdv::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (GUI_event_IsCaptureEv(event)) {
        menu.WindowEvent(sender, event, param);
    }
}

WindowFileBrowserAdv::WindowFileBrowserAdv(window_t *parent, Rect16 rect, const char *media_SFN_path)
    : AddSuperWindow<window_frame_t>(parent, rect, win_type_t::normal)
    , file_browser(parent, rc_menu(rect), media_SFN_path)
    , bar(this, rc_bar(rect), file_browser) // event loop will handle everything
{
    CaptureNormalWindow(file_browser);
    file_browser.SetFocus();
}

void WindowFileBrowserAdv::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (GUI_event_IsCaptureEv(event)) {
        file_browser.WindowEvent(sender, event, param);
    }
}
