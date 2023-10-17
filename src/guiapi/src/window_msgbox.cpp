// window_msgbox.cpp
#include "window_msgbox.hpp"
#include "sound.hpp"
#include <algorithm>
#include "ScreenHandler.hpp"
#include "client_response_texts.hpp"
#include "GuiDefaults.hpp"
#include "img_resources.hpp"

/*****************************************************************************/
// Icon + Text layout adjusting tool

void AdjustLayout(window_text_t &text, window_icon_t &icon) {
    icon.SetAlignment(Align_t::LeftTop());
    Rect16 new_rect = text.GetRect();

    /// @todo Cannot calculate number of lines of text in advance as it gets wrapped by words.
    ///       Centering unknown text vertically is not possible now.
    if (true /*text.text.computeNumLinesAndRewind() > 3*/) { // If there are more than 3 rows, icon will align to the top
        new_rect -= Rect16::Top_t(text.padding.top); // Add Text's padding
        text.SetAlignment(Align_t::LeftTop());
    } else {
        new_rect = Rect16::Height_t(text.get_font()->h * 3); // Set height to 3 rows
        text.SetAlignment(Align_t::LeftCenter());
    }
    text.SetRect(new_rect);
}

/*****************************************************************************/
// MsgBoxBase
MsgBoxBase::MsgBoxBase(Rect16 rect, const PhaseResponses &resp, size_t def_btn, const PhaseTexts *labels, string_view_utf8 txt,
    is_multiline multiline, is_closed_on_click_t close)
    : AddSuperWindow<IDialog>(rect)
    , text(this, getTextRect(), multiline, is_closed_on_click_t::no, txt)
    , pButtons(new(&radio_mem_space) RadioButton(this, GuiDefaults::GetButtonRect(rect), resp, labels))
    , result(Response::_none) {
    flags.close_on_click = close;
    pButtons->SetBtnIndex(def_btn);
    CaptureNormalWindow(*pButtons);
}

Rect16 MsgBoxBase::getTextRect() {
    if (GuiDefaults::EnableDialogBigLayout) {
        return GuiDefaults::MsgBoxLayoutRect;
    } else {
        return GetRect() - GuiDefaults::GetButtonRect(GetRect()).Height();
    }
}

void MsgBoxBase::set_text_alignment(Align_t alignment) {
    text.SetAlignment(alignment);
}

void MsgBoxBase::set_text_font(font_t *font) {
    text.set_font(font);
}

void MsgBoxBase::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    event_conversion_union un;
    un.pvoid = param;

    switch (event) {
    case GUI_event_t::CHILD_CLICK:
        result = un.response;
        if (flags.close_on_click == is_closed_on_click_t::yes) {
            Screens::Access()->Close();
        } else if (GetParent())
            GetParent()->WindowEvent(this, GUI_event_t::CHILD_CLICK, un.pvoid);
        break;
    default:
        SuperWindowEvent(sender, event, param);
    }
}

/*****************************************************************************/
// MsgBoxTitled
MsgBoxTitled::MsgBoxTitled(Rect16 rect, const PhaseResponses &resp, size_t def_btn, const PhaseTexts *labels,
    string_view_utf8 txt, is_multiline multiline, string_view_utf8 tit, const img::Resource *title_icon, is_closed_on_click_t close, dense_t dense)
    : AddSuperWindow<MsgBoxIconned>(rect, resp, def_btn, labels, txt, multiline, title_icon, close)
    , title(this, GetRect(), is_multiline::no, is_closed_on_click_t::no, tit) {
    title.set_font(getTitleFont());
    title.SetRect(getTitleRect());
    icon.SetRect(getIconRect());
    text.SetRect(getTextRect());
    if (dense == dense_t::yes) {
        set_text_font(GuiDefaults::FontMenuSpecial);
    }
}

void MsgBoxTitled::set_title_alignment(Align_t alignment) {
    title.SetAlignment(alignment);
}

Rect16 MsgBoxTitled::getTitleRect() {
    const auto shared_top = GetRect().Top() + 1; /* Visual delimeter */
    const auto shared_height = Rect16::Height_t(getTitleFont()->h);
    if (icon.IsIconValid()) {
        return Rect16(Rect16::Left_t(MsgBoxTitled::TextPadding.left + GuiDefaults::FooterIconSize.w + MsgBoxTitled::IconTitleDelimeter),
            shared_top,
            Rect16::Width_t(Width() - (MsgBoxTitled::TextPadding.left + MsgBoxTitled::TextPadding.right + GuiDefaults::FooterIconSize.w + MsgBoxTitled::IconTitleDelimeter)),
            shared_height);
    } else {
        return Rect16(
            Rect16::Left_t(MsgBoxTitled::TextPadding.left),
            shared_top,
            Rect16::Width_t(Width() - (MsgBoxTitled::TextPadding.left + MsgBoxTitled::TextPadding.right)),
            shared_height);
    }
}

Rect16 MsgBoxTitled::getLineRect() {
    const auto title_rect { getTitleRect() };
    return Rect16(GetRect().Left(), title_rect.EndPoint().y + 2 /* Visual delimeter */, GetRect().Width(), 1);
}

Rect16 MsgBoxTitled::getTextRect() {
    Rect16 text_rect = GetRect();
    uint16_t y = getLineRect().EndPoint().y;

    text_rect -= Rect16::Height_t(y - text_rect.Top());
    text_rect -= GuiDefaults::GetButtonRect(GetRect()).Height();

    text_rect = Rect16::Top_t(y);

    text_rect.CutPadding(MsgBoxTitled::TextPadding);

    return text_rect;
}

Rect16 MsgBoxTitled::getIconRect() {
    return Rect16(GetRect().Left() + MsgBoxTitled::TextPadding.left, GetRect().Top() + 3 /* Visual delimeter */, GuiDefaults::FooterIconSize.w, GuiDefaults::FooterIconSize.h);
}

font_t *MsgBoxTitled::getTitleFont() {
    return GuiDefaults::FontBig;
}

/*****************************************************************************/
// MsgBoxIconned
MsgBoxIconned::MsgBoxIconned(Rect16 rect, const PhaseResponses &resp, size_t def_btn, const PhaseTexts *labels,
    string_view_utf8 txt, is_multiline multiline, const img::Resource *icon_res, is_closed_on_click_t close)
    : AddSuperWindow<MsgBoxBase>(rect, resp, def_btn, labels, txt, multiline, close)
    , icon(this, icon_res, { int16_t(rect.Left()), int16_t(rect.Top()) }, GuiDefaults::Padding) {
    text.SetRect(getTextRect()); // reinit text, icon and title must be initialized
    if (GuiDefaults::EnableDialogBigLayout) {
        text.SetAlignment(Align_t::LeftCenter());
        if (icon_res) {
            icon.SetRect(getIconRect());
        }
        AdjustLayout(text, icon);
    } else {
        icon -= Rect16::Width_t(GuiDefaults::Padding.left + GuiDefaults::Padding.right);
        icon += Rect16::Left_t((Width() / 2) - (icon.Width() / 2)); // center icon
    }
}

Rect16 MsgBoxIconned::getIconRect() {
    return GuiDefaults::MessageIconRect;
}

Rect16 MsgBoxIconned::getTextRect() {
    if (GuiDefaults::EnableDialogBigLayout) {
        return GuiDefaults::MessageTextRect;
    } else {
        Rect16 text_rect = GetRect();
        text_rect -= icon.Height();
        text_rect -= GuiDefaults::GetButtonRect(GetRect()).Height();

        text_rect += Rect16::Top_t(icon.Height());
        return text_rect;
    }
}

/*****************************************************************************/
// MsgBoxIconPepa
MsgBoxIconPepa::MsgBoxIconPepa(Rect16 rect, const PhaseResponses &resp, size_t def_btn, const PhaseTexts *labels,
    string_view_utf8 txt, is_multiline multiline, const img::Resource *ic)
    : AddSuperWindow<MsgBoxIconned>(rect, resp, def_btn, labels, txt, multiline, ic) {
    icon.SetRect(getIconRect());
    icon.SetAlignment(Align_t::CenterTop());

    text.SetRect(getTextRect());
    text.SetAlignment(Align_t::LeftBottom());
}

Rect16 MsgBoxIconPepa::getTextRect() {
    return Rect16(60, GuiDefaults::HeaderHeight + 170, 380, 23); // This is reserved for one-lined text: "All tests finished successfully!"
}

Rect16 MsgBoxIconPepa::getIconRect() {
    return Rect16(0, GuiDefaults::HeaderHeight, display::GetW(), 140);
}

/*****************************************************************************/
// MsgBoxIconPepaCentered
MsgBoxIconPepaCentered::MsgBoxIconPepaCentered(Rect16 rect, const PhaseResponses &resp, size_t def_btn, const PhaseTexts *labels,
    string_view_utf8 txt, is_multiline multiline, const img::Resource *ic)
    : AddSuperWindow<MsgBoxIconned>(rect, resp, def_btn, labels, txt, multiline, ic) {
    icon.SetRect(getIconRect());
    icon.SetAlignment(Align_t::CenterTop());

    text.SetRect(getTextRect());
    text.SetAlignment(Align_t::Center());
}

Rect16 MsgBoxIconPepaCentered::getTextRect() {
    return Rect16(10, GuiDefaults::HeaderHeight + 147, display::GetW() - 10, 23 * 4);
}

Rect16 MsgBoxIconPepaCentered::getIconRect() {
    return Rect16(0, GuiDefaults::HeaderHeight, display::GetW(), 140);
}

/*****************************************************************************/
// MsgBoxIconnedError
MsgBoxIconnedError::MsgBoxIconnedError(Rect16 rect, const PhaseResponses &resp, size_t def_btn, const PhaseTexts *labels, string_view_utf8 txt, is_multiline multiline, const img::Resource *icon_res)
    : AddSuperWindow<MsgBoxIconned>(rect, resp, def_btn, labels, txt, multiline, icon_res) {
    SetRoundCorners();
    text.SetRect(getTextRect()); // reinit text, icon and title must be initialized
    text.SetAlignment(Align_t::LeftCenter());
    icon.SetRect(getIconRect());
    AdjustLayout(text, icon);
    SetBackColor(COLOR_ORANGE);
    text.SetBackColor(COLOR_ORANGE);
    icon.SetBackColor(COLOR_ORANGE);

    if (!pButtons) { // pButtons can never be null
        assert("unassigned msgbox");
        return;
    }

    pButtons->SetBackColor(COLOR_WHITE);
}

/*****************************************************************************/
// MsgBoxIconnedWait
MsgBoxIconnedWait::MsgBoxIconnedWait(Rect16 rect, const PhaseResponses &resp, size_t def_btn, const PhaseTexts *labels,
    string_view_utf8 txt, is_multiline multiline)
    : AddSuperWindow<MsgBoxIconned>(rect, resp, def_btn, labels, txt, multiline, &img::hourglass_26x39) {
    icon.SetRect(Rect16(0, GuiDefaults::HeaderHeight, display::GetW(), 140));
    icon.SetAlignment(Align_t::Center());

    text.SetRect(Rect16(10, GuiDefaults::HeaderHeight + 147, display::GetW() - 10, 23 * 4));
    text.SetAlignment(Align_t::Center());
}

/*****************************************************************************/
// MsgBoxIS
MsgBoxIS::MsgBoxIS(Rect16 rect, const PhaseResponses &resp, size_t def_btn, const PhaseTexts *labels,
    string_view_utf8 txt, is_multiline multiline, const img::Resource *icon_res, is_closed_on_click_t close)
    : AddSuperWindow<MsgBoxBase>(rect, resp, def_btn, labels, txt, multiline, close)
    , icon(this, icon_res, {}, GuiDefaults::Padding)
    , qr(this, {}, QR_ADDR) {
    if (GuiDefaults::EnableDialogBigLayout) {
        uint16_t w = rect.Width();
        uint16_t h = rect.Height();

        uint16_t icon_w = w / 10 * 2;
        uint16_t text_w = w / 10 * 5;
        uint16_t qr_w = w / 10 * 3 - 8;

        icon.SetRect({ 3, 30, icon_w, icon_w });
        text.SetRect({ int16_t(icon_w + 3), 0, text_w, h });
        qr.SetRect({ int16_t(icon_w + text_w + 3), 50, qr_w, qr_w });

        text.SetAlignment(Align_t::LeftCenter());
    } else {
        text.SetRect(getTextRect());
        icon -= Rect16::Width_t(GuiDefaults::Padding.left + GuiDefaults::Padding.right);
        icon += Rect16::Left_t((Width() / 2) - (icon.Width() / 2)); // center icon
    }
}

Rect16 MsgBoxIS::getTextRect() {
    if (GuiDefaults::EnableDialogBigLayout) {
        return GuiDefaults::MessageTextRect;
    } else {
        Rect16 text_rect = GetRect();
        text_rect -= icon.Height();
        text_rect -= GuiDefaults::GetButtonRect(GetRect()).Height();

        text_rect += Rect16::Top_t(icon.Height());
        return text_rect;
    }
}

/*****************************************************************************/
// MsgBoxBase variadic template methods
// to be used as blocking functions
template <class T, typename... Args>
Response MsgBox_Custom(Rect16 rect, const PhaseResponses &resp, size_t def_btn, string_view_utf8 txt, is_multiline multiline, Args... args) {
    const PhaseTexts labels = { BtnResponse::GetText(resp[0]), BtnResponse::GetText(resp[1]), BtnResponse::GetText(resp[2]), BtnResponse::GetText(resp[3]) };
    // static_assert(labels.size() == 4, "Incorrect array size, modify number of elements");
    T msgbox(rect, resp, def_btn, &labels, txt, multiline, args...);
    msgbox.MakeBlocking();
    return msgbox.GetResult();
}

Response MsgBox(string_view_utf8 txt, const PhaseResponses &resp, size_t def_btn, Rect16 rect, is_multiline multiline) {
    return MsgBox_Custom<MsgBoxBase>(rect, resp, def_btn, txt, multiline);
}

Response MsgBoxTitle(string_view_utf8 title, string_view_utf8 txt, const PhaseResponses &resp, size_t def_btn, Rect16 rect, const img::Resource *icon_id, is_multiline multiline) {
    return MsgBox_Custom<MsgBoxTitled>(rect, resp, def_btn, txt, multiline, title, icon_id);
}

Response MsgBoxError(string_view_utf8 txt, const PhaseResponses &resp, size_t def_btn, Rect16 rect, is_multiline multiline) {
    if (GuiDefaults::EnableDialogBigLayout) {
        return MsgBox_Custom<MsgBoxIconnedError>(rect, resp, def_btn, txt, multiline, &img::error_white_48x48);
    } else {
        constexpr static const char *label = N_("Error");
        return MsgBox_Custom<MsgBoxTitled>(rect, resp, def_btn, txt, multiline, _(label), &img::error_16x16);
    }
}

Response MsgBoxQuestion(string_view_utf8 txt, const PhaseResponses &resp, size_t def_btn, Rect16 rect, is_multiline multiline) {
    if (GuiDefaults::EnableDialogBigLayout) {
        return MsgBox_Custom<MsgBoxIconned>(rect, resp, def_btn, txt, multiline, &img::question_48x48);
    } else {
        constexpr static const char *label = N_("Question");
        return MsgBox_Custom<MsgBoxTitled>(rect, resp, def_btn, txt, multiline, _(label), &img::question_16x16);
    }
}

Response MsgBoxWarning(string_view_utf8 txt, const PhaseResponses &resp, size_t def_btn, Rect16 rect, is_multiline multiline) {
    if (GuiDefaults::EnableDialogBigLayout) {
        return MsgBox_Custom<MsgBoxIconned>(rect, resp, def_btn, txt, multiline, &img::warning_48x48);
    } else {
        constexpr static const char *label = N_("Warning");
        return MsgBox_Custom<MsgBoxTitled>(rect, resp, def_btn, txt, multiline, _(label), &img::warning_16x16);
    }
}

Response MsgBoxInfo(string_view_utf8 txt, const PhaseResponses &resp, size_t def_btn, Rect16 rect, is_multiline multiline) {
    if (GuiDefaults::EnableDialogBigLayout) {
        return MsgBox_Custom<MsgBoxIconned>(rect, resp, def_btn, txt, multiline, &img::info_48x48);
    } else {
        constexpr static const char *label = N_("Information");
        return MsgBox_Custom<MsgBoxTitled>(rect, resp, def_btn, txt, multiline, _(label), &img::info_16x16);
    }
}

Response MsgBoxPepa(string_view_utf8 txt, const PhaseResponses &resp, size_t def_btn, Rect16 rect, is_multiline multiline) {
    if (GuiDefaults::EnableDialogBigLayout) {
        return MsgBox_Custom<MsgBoxIconPepa>(rect, resp, def_btn, txt, multiline, &img::pepa_92x140);
    } else {
        return MsgBox_Custom<MsgBoxIconned>(rect, resp, def_btn, txt, multiline, &img::pepa_42x64);
    }
}

Response MsgBoxPepaCentered(string_view_utf8 txt, const PhaseResponses &resp, size_t def_btn, Rect16 rect, is_multiline multiline) {
    if (GuiDefaults::EnableDialogBigLayout) {
        return MsgBox_Custom<MsgBoxIconPepaCentered>(rect, resp, def_btn, txt, multiline, &img::pepa_92x140);
    } else {
        return MsgBox_Custom<MsgBoxIconned>(rect, resp, def_btn, txt, multiline, &img::pepa_42x64);
    }
}

Response MsgBoxISWarning(string_view_utf8 txt, const PhaseResponses &resp, size_t def_btn, Rect16 rect, is_multiline multiline) {
    if (GuiDefaults::EnableDialogBigLayout) {
        return MsgBox_Custom<MsgBoxIS>(rect, resp, def_btn, txt, multiline, &img::error_white_48x48);
    } else {
        constexpr static const char *label = N_("Feature Preview");

        const PhaseTexts labels = { BtnResponse::GetText(resp[0]), BtnResponse::GetText(resp[1]), BtnResponse::GetText(resp[2]), BtnResponse::GetText(resp[3]) };
        MsgBoxTitled msgbox(rect, resp, def_btn, &labels, txt, multiline, _(label), nullptr, is_closed_on_click_t::yes, dense_t::yes);
        msgbox.set_text_alignment(Align_t::Center());
        msgbox.set_title_alignment(Align_t::Center());
        msgbox.MakeBlocking();
        return msgbox.GetResult();
    }
}
