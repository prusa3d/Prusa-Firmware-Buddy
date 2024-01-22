// window_dlg_strong_warning.cpp

#include "window_dlg_strong_warning.hpp"
#include "display_helper.h"
#include "ScreenHandler.hpp"
#include "sound.hpp"
#include "client_response_texts.hpp"
#include "window_msgbox.hpp" // due AdjustLayout function
#include "config_features.h"

std::bitset<window_dlg_strong_warning_t::types::count_> window_dlg_strong_warning_t::shown;
window_dlg_strong_warning_t::types window_dlg_strong_warning_t::on_top = window_dlg_strong_warning_t::types::count_;

const PhaseResponses dlg_responses = { Response::Continue, Response::_none, Response::_none, Response::_none };

window_dlg_strong_warning_t::window_dlg_strong_warning_t()
    : AddSuperWindow<IDialog>(GuiDefaults::DialogFrameRect, IsStrong::yes)
    , header(this, _(Title))
    , footer(this)
    , icon(this, { 0, 0, 0, 0 }, &img::exposure_times_48x48)
    , text(this, { 0, 0, 0, 0 }, is_multiline::yes)
    , button(this, GuiDefaults::GetButtonRect(GetRect()) - Rect16::Top_t(GuiDefaults::EnableDialogBigLayout ? 0 : 64), dlg_responses, &ph_txt_continue) //
{
    if constexpr (GuiDefaults::EnableDialogBigLayout) {
        footer.Hide();
        header.Hide();
    }
}

void window_dlg_strong_warning_t::adjustLayout() {
    icon.SetRect(GuiDefaults::MessageIconRect);
    text.SetRect(GuiDefaults::MessageTextRect);
    AdjustLayout(text, icon);
}

void window_dlg_strong_warning_t::setWarningText(types type) {
    header.SetText(_(icon_title_text[type].title));

    icon.SetRes(icon_title_text[type].icon);
    text.SetText(_(icon_title_text[type].text));
    adjustLayout(); // this could cause invalidation issue
}

void window_dlg_strong_warning_t::show(types type) {
    if (on_top == type) {
        return;
    }
    shown[type] = true;
    on_top = type;

    setWarningText(type);

    Invalidate(); // redraw window (mainly because of messed up button graphics...)

    if (!GetParent()) {
        window_t *parent = Screens::Access()->Get();
        if (parent) {
            Sound_Play(eSOUND_TYPE::StandardAlert);
            parent->RegisterSubWin(*this);
        }
    }
}

// Relaunch correct window
void window_dlg_strong_warning_t::screenJump() {
    for (types t = types(0); t < types::count_; t = types(t + 1)) {

        if (shown[t]) {
            setWarningText(t);
            Screens::Access()->Get()->RegisterSubWin(*this);
            return;
        }
    }
}

void window_dlg_strong_warning_t::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (!GetParent()) {
        return;
    }

    switch (event) {

    case GUI_event_t::CLICK:
    case GUI_event_t::TOUCH_CLICK:
    case GUI_event_t::TOUCH_SWIPE_LEFT:
    case GUI_event_t::TOUCH_SWIPE_RIGHT:
        // todo use timer
        shown[on_top] = false; // remove from mask
        on_top = types::count_; // erase on_top

        if (shown.any()) {
            for (types t = types(0); t < types::count_; t = types(t + 1)) {
                if (shown[t]) {
                    show(t);
                    break;
                }
            }
        } else {
            GetParent()->UnregisterSubWin(*this);
        }
        break;

    default:
        SuperWindowEvent(sender, event, param);
        break;
    }
}

void window_dlg_strong_warning_t::ScreenJumpCheck() {
    // Check warning flag and if current screen has no warning window, create one
    if (shown.any() && !Screens::Access()->Get()->GetFirstStrongDialog()) { //&& !Instance().IsVisible()){
        Instance().screenJump();
    }
}
