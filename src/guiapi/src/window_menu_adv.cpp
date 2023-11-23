/**
 * @file window_menu_adv.cpp
 */

#include <algorithm>
#include <cstdlib>
#include "window_menu_adv.hpp"
#include "img_resources.hpp"
#include "marlin_client.hpp"
#include "touch_get.hpp"

static Rect16::Width_t Icon_w() {
    return touch::is_enabled() ? GuiDefaults::MenuIcon_w : Rect16::Width_t(0);
}

static Rect16::Height_t Icon_h() {
    return touch::is_enabled() ? GuiDefaults::MenuIcon_h : Rect16::Height_t(0);
}

static Rect16 rc_menu(Rect16 rc) {
    rc.CutPadding(GuiDefaults::MenuPadding);
    rc -= std::max(Icon_w(), GuiDefaults::MenuScrollbarWidth);
    return rc;
}

static Rect16 rc_arrow_up(Rect16 rc) {
    Rect16::Left_t left = rc.Left() + rc.Width() - Icon_w();
    return { left, rc.Top(), Icon_w(), Icon_h() };
}

#if (MENU_HAS_BUTTONS)
static Rect16 rc_arrow_down(Rect16 rc) {
    Rect16 ret = rc_arrow_up(rc);
    ret += Rect16::Top_t(rc.Height() - Icon_h());
    return ret;
}

static const img::Resource *arrow_up() {
    return touch::is_enabled() ? &img::arrow_up_white_24x24 : nullptr;
}

static const img::Resource *arrow_down() {
    return touch::is_enabled() ? &img::arrow_down_white_24x24 : nullptr;
}
#endif

static Rect16 rc_bar(Rect16 rc) {
    Rect16 ret = rc_arrow_up(rc);
    ret += Rect16::Top_t(Icon_h());
    ret += Rect16::Left_t(ret.Width() - GuiDefaults::MenuScrollbarWidth);
    ret = GuiDefaults::MenuScrollbarWidth;
    ret = Rect16::Height_t(rc.Height() - Icon_h() * 2);
    return ret;
}

WindowMenuAdv::WindowMenuAdv(window_t *parent, Rect16 rect, IWinMenuContainer *pContainer, uint8_t index)
    : AddSuperWindow<window_frame_t>(parent, rect)
    , menu(parent, rc_menu(rect), pContainer, index)
#if (MENU_HAS_BUTTONS)
    , up(this, rc_arrow_up(rect), arrow_up(), []() {}) // parent will handle click, do nothing
    , down(this, rc_arrow_down(rect), arrow_down(), []() {}) // parent will handle click, do nothing
#endif
    , bar(this, rc_bar(rect), menu) // event loop will handle everything
{
}

void WindowMenuAdv::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {

    switch (event) {
    // no need to resend GUI_event_t::LOOP and GUI_event_t::TEXT_ROLL (the are broadcasted)
    // and GUI_event_t::CHILD_CLICK: which is targetted to menu directly
    case GUI_event_t::CLICK:
    case GUI_event_t::ENC_DN:
    case GUI_event_t::ENC_UP:
        menu.WindowEvent(sender, event, param);
        break;
    case GUI_event_t::TOUCH: {
        event_conversion_union un;
        un.pvoid = param;
        // menu gets touch event
        if (menu.GetRect().Contain(un.point)) {
            menu.WindowEvent(sender, GUI_event_t::TOUCH, un.pvoid);
            break;
        }
#if (MENU_HAS_BUTTONS)
        // icon gets click
        if (up.GetRect().Contain(un.point)) {
            menu.RollUp();
            // we clicked on one of menus move buttons
            // unlike knob we did not know it until now
            marlin_client::notify_server_about_encoder_move();
            break;
        }
        if (down.GetRect().Contain(un.point)) {
            menu.RollDown();
            // we clicked on one of menus move buttons
            // unlike knob we did not know it until now
            marlin_client::notify_server_about_encoder_move();
            break;
        }
#endif // MENU_HAS_BUTTONS
    } break;
    default:
        // discard other events
        break;
    }
}

WindowFileBrowserAdv::WindowFileBrowserAdv(window_t *parent, Rect16 rect, const char *media_SFN_path)
    : AddSuperWindow<window_frame_t>(parent, rect, win_type_t::normal)
    , file_browser(parent, rc_menu(rect), media_SFN_path)
#if (MENU_HAS_BUTTONS)
    , up(this, rc_arrow_up(rect), arrow_up(), []() {}) // parent will handle click, do nothing
    , down(this, rc_arrow_down(rect), arrow_down(), []() {}) // parent will handle click, do nothing
#endif
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
    case GUI_event_t::TOUCH: {
        event_conversion_union un;
        un.pvoid = param;
        // file_browser gets touch event
        if (file_browser.GetRect().Contain(un.point)) {
            file_browser.WindowEvent(sender, GUI_event_t::TOUCH, un.pvoid);
            break;
        }
#if (MENU_HAS_BUTTONS)
        // icon gets click
        if (up.GetRect().Contain(un.point)) {
            file_browser.RollUp();
            // we clicked on one of file_browsers move buttons
            // unlike knob we did not know it until now
            marlin_client::notify_server_about_encoder_move();
            break;
        }
        if (down.GetRect().Contain(un.point)) {
            file_browser.RollDown();
            // we clicked on one of file_browsers move buttons
            // unlike knob we did not know it until now
            marlin_client::notify_server_about_encoder_move();
            break;
        }
#endif // MENU_HAS_BUTTONS
    } break;
    default:
        // discard other events
        break;
    }
}
