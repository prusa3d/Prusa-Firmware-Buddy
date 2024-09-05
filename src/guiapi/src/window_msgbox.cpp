// window_msgbox.cpp
#include "window_msgbox.hpp"
#include "sound.hpp"
#include <algorithm>
#include "ScreenHandler.hpp"
#include "client_response_texts.hpp"
#include <guiconfig/GuiDefaults.hpp>
#include "img_resources.hpp"

/*****************************************************************************/
// Icon + Text layout adjusting tool

void AdjustLayout(window_text_t &text, window_icon_t &icon) {
    icon.SetAlignment(Align_t::LeftTop());
    Rect16 new_rect = text.GetRect();
    new_rect -= Rect16::Top_t(text.padding.top);
    text.SetAlignment(Align_t::LeftTop());
    text.SetRect(new_rect);
}

/*****************************************************************************/
// MsgBoxBase
MsgBoxBase::MsgBoxBase(Rect16 rect, const PhaseResponses &resp, size_t def_btn, const PhaseTexts *labels, const string_view_utf8 &txt,
    is_multiline multiline, is_closed_on_click_t close)
    : IDialog(rect)
    , text(this, getTextRect(), multiline, is_closed_on_click_t::no, txt)
    , result(Response::_none) {
    flags.close_on_click = close;
    static_assert(sizeof(RadioButton) <= std::tuple_size_v<RadioMemSpace>);
    pButtons = make_static_unique_ptr<RadioButton>(&radio_mem_space, this, GuiDefaults::GetButtonRect(rect), resp, labels);
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

void MsgBoxBase::BindToFSM(FSMAndPhase phase) {
    using T = RadioButtonFSM;
    static_assert(sizeof(T) <= mem_space_size, "RadioMemSpace is too small");

    if (!pButtons) { // pButtons can never be null
        assert("unassigned msgbox");
        return;
    }

    Rect16 rc = pButtons->GetRect();
    bool has_icon = pButtons->HasIcon();
    Color back = pButtons->GetBackColor();

    ReleaseCaptureOfNormalWindow();

    // First reset, then create new class; we cannot afford constructing and then destructing because it's the same memory
    pButtons.reset();
    static_assert(sizeof(T) <= std::tuple_size_v<RadioMemSpace>);
    pButtons = make_static_unique_ptr<T>(radio_mem_space.data(), this, rc, phase);

    has_icon ? pButtons->SetHasIcon() : pButtons->ClrHasIcon();
    pButtons->SetBackColor(back);

    CaptureNormalWindow(*pButtons);
}

void MsgBoxBase::generate_response(Response r) {
    result = r;

    if (flags.close_on_click == is_closed_on_click_t::yes) {
        Screens::Access()->Close();
    } else if (GetParent()) {
        GetParent()->WindowEvent(this, GUI_event_t::CHILD_CLICK, event_conversion_union { .response = r }.pvoid);
    }
}

void MsgBoxBase::set_text_alignment(Align_t alignment) {
    text.SetAlignment(alignment);
}

void MsgBoxBase::set_text_font(Font font) {
    text.set_font(font);
}

void MsgBoxBase::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    switch (event) {

    case GUI_event_t::CHILD_CLICK: {
        generate_response(event_conversion_union { .pvoid = param }.response);
        return;
    }

    default:
        break;
    }

    IDialog::windowEvent(sender, event, param);
}

static constexpr Font TitleFont = GuiDefaults::FontBig;

/*****************************************************************************/
// MsgBoxTitled
MsgBoxTitled::MsgBoxTitled(Rect16 rect, const PhaseResponses &resp, size_t def_btn, const PhaseTexts *labels,
    const string_view_utf8 &txt, is_multiline multiline, const string_view_utf8 &tit, const img::Resource *title_icon, is_closed_on_click_t close, dense_t dense)
    : MsgBoxIconned(rect, resp, def_btn, labels, txt, multiline, title_icon, close)
    , title(this, GetRect(), is_multiline::no, is_closed_on_click_t::no, tit) {
    title.set_font(TitleFont);
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
    const auto shared_height = Rect16::Height_t(height(TitleFont));
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

/*****************************************************************************/
// MsgBoxIconned
MsgBoxIconned::MsgBoxIconned(Rect16 rect, const PhaseResponses &resp, size_t def_btn, const PhaseTexts *labels,
    const string_view_utf8 &txt, is_multiline multiline, const img::Resource *icon_res, is_closed_on_click_t close)
    : MsgBoxBase(rect, resp, def_btn, labels, txt, multiline, close)
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
// MsgBoxIconPepaCentered
MsgBoxIconPepaCentered::MsgBoxIconPepaCentered(Rect16 rect, const PhaseResponses &resp, size_t def_btn, const PhaseTexts *labels,
    const string_view_utf8 &txt, is_multiline multiline, const img::Resource *ic)
    : MsgBoxIconned(rect, resp, def_btn, labels, txt, multiline, ic) {
    icon.SetRect(getIconRect());
    icon.SetAlignment(Align_t::CenterTop());

    text.SetRect(getTextRect());
    text.SetAlignment(Align_t::Center());
}

Rect16 MsgBoxIconPepaCentered::getTextRect() {
    return Rect16(10, GuiDefaults::HeaderHeight + 147, GuiDefaults::ScreenWidth - 10, 23 * 4);
}

Rect16 MsgBoxIconPepaCentered::getIconRect() {
    return Rect16(0, GuiDefaults::HeaderHeight, GuiDefaults::ScreenWidth, 140);
}

/*****************************************************************************/
// MsgBoxIconnedError
MsgBoxIconnedError::MsgBoxIconnedError(Rect16 rect, const PhaseResponses &resp, size_t def_btn, const PhaseTexts *labels, const string_view_utf8 &txt, is_multiline multiline, const img::Resource *icon_res)
    : MsgBoxIconned(rect, resp, def_btn, labels, txt, multiline, icon_res) {
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
    const string_view_utf8 &txt, is_multiline multiline)
    : MsgBoxIconned(rect, resp, def_btn, labels, txt, multiline, &img::hourglass_26x39) {
    icon.SetRect(Rect16(0, GuiDefaults::HeaderHeight, GuiDefaults::ScreenWidth, 140));
    icon.SetAlignment(Align_t::Center());

    text.SetRect(Rect16(10, GuiDefaults::HeaderHeight + 147, GuiDefaults::ScreenWidth - 10, 23 * 4));
    text.SetAlignment(Align_t::Center());
}

/*****************************************************************************/
namespace {

enum class MsgBoxDialogClass {
    MsgBoxBase,
    MsgBoxTitled,
    MsgBoxIconned,
    MsgBoxIconnedError,
    MsgBoxIconPepaCentered,
    _count,
};

struct MsgBoxImplicitConfig {
    MsgBoxDialogClass dialog_class;
    const img::Resource *icon = nullptr;
    const char *title = nullptr;
};

constexpr bool big_layout = GuiDefaults::EnableDialogBigLayout;

constexpr const char *nullstr = nullptr;

constexpr EnumArray<MsgBoxType, MsgBoxImplicitConfig, MsgBoxType::_count> msb_box_implicit_configs {
    {
        MsgBoxType::standard,
        MsgBoxImplicitConfig {
            .dialog_class = MsgBoxDialogClass::MsgBoxBase,
        },
    },
    {
        MsgBoxType::titled,
        MsgBoxImplicitConfig {
            .dialog_class = MsgBoxDialogClass::MsgBoxTitled,
        },
    },
    {
        MsgBoxType::error,
        MsgBoxImplicitConfig {
            .dialog_class = big_layout ? MsgBoxDialogClass::MsgBoxIconnedError : MsgBoxDialogClass::MsgBoxTitled,
            .icon = big_layout ? &img::error_white_48x48 : &img::error_16x16,
            .title = big_layout ? nullstr : N_("Error"),
        },
    },
    {
        MsgBoxType::question,
        MsgBoxImplicitConfig {
            .dialog_class = big_layout ? MsgBoxDialogClass::MsgBoxIconned : MsgBoxDialogClass::MsgBoxTitled,
            .icon = big_layout ? &img::question_48x48 : &img::question_16x16,
            .title = big_layout ? nullstr : N_("Question"),
        },
    },
    {
        MsgBoxType::warning,
        MsgBoxImplicitConfig {
            .dialog_class = big_layout ? MsgBoxDialogClass::MsgBoxIconned : MsgBoxDialogClass::MsgBoxTitled,
            .icon = big_layout ? &img::warning_48x48 : &img::warning_16x16,
            .title = big_layout ? nullstr : N_("Warning"),
        },
    },
    {
        MsgBoxType::info,
        MsgBoxImplicitConfig {
            .dialog_class = big_layout ? MsgBoxDialogClass::MsgBoxIconned : MsgBoxDialogClass::MsgBoxTitled,
            .icon = big_layout ? &img::info_48x48 : &img::info_16x16,
            .title = big_layout ? nullstr : N_("Information"),
        },
    },
    {
        MsgBoxType::pepa_centered,
        MsgBoxImplicitConfig {
            .dialog_class = MsgBoxDialogClass::MsgBoxIconPepaCentered,
            .icon = big_layout ? &img::pepa_92x140 : &img::pepa_42x64,
        },
    },
};

} // namespace

Response MsgBoxBuilder::exec() const {
    const MsgBoxImplicitConfig &implicit_config = msb_box_implicit_configs[static_cast<size_t>(type)];
    const img::Resource *used_icon = icon ?: implicit_config.icon;
    const string_view_utf8 used_title = title.isNULLSTR() ? _(implicit_config.title) : title;

    const PhaseTexts labels = {
        get_response_text(responses[0]),
        get_response_text(responses[1]),
        get_response_text(responses[2]),
        get_response_text(responses[3]),
    };

    const auto box_f = [&]<typename T, MsgBoxDialogClass CS = MsgBoxDialogClass::_count, typename... Args>(Args... args) {
        T msgbox(rect, responses, static_cast<size_t>(default_button), &labels, text, multiline, args...);
        Screens::Access()->gui_loop_until_dialog_closed(loop_callback);
        return msgbox.GetResult();
    };

    switch (implicit_config.dialog_class) {

    case MsgBoxDialogClass::MsgBoxBase:
        return box_f.operator()<MsgBoxBase>();

    case MsgBoxDialogClass::MsgBoxTitled:
        return box_f.operator()<MsgBoxTitled>(used_title, used_icon);

    case MsgBoxDialogClass::MsgBoxIconned:
        return box_f.operator()<MsgBoxIconned>(used_icon);

    case MsgBoxDialogClass::MsgBoxIconnedError:
        return box_f.operator()<MsgBoxIconnedError>(used_icon);

    case MsgBoxDialogClass::MsgBoxIconPepaCentered:
        return box_f.operator()<MsgBoxIconPepaCentered>(used_icon);

    default:
        bsod("Invalid MsgBoxDialogClass");
    }
}

Response msg_box(MsgBoxType type, const string_view_utf8 &txt, const PhaseResponses &resp, MsgBoxDefaultButton default_button) {
    return MsgBoxBuilder {
        .type = type,
        .text = txt,
        .responses = resp,
        .default_button = default_button,
    }
        .exec();
}

Response MsgBox(const string_view_utf8 &txt, const PhaseResponses &resp, size_t def_btn) {
    return msg_box(MsgBoxType::standard, txt, resp, static_cast<MsgBoxDefaultButton>(def_btn));
}

Response MsgBoxError(const string_view_utf8 &txt, const PhaseResponses &resp, size_t def_btn) {
    return msg_box(MsgBoxType::error, txt, resp, static_cast<MsgBoxDefaultButton>(def_btn));
}

Response MsgBoxQuestion(const string_view_utf8 &txt, const PhaseResponses &resp, size_t def_btn) {
    return msg_box(MsgBoxType::question, txt, resp, static_cast<MsgBoxDefaultButton>(def_btn));
}

Response MsgBoxWarning(const string_view_utf8 &txt, const PhaseResponses &resp, size_t def_btn) {
    return msg_box(MsgBoxType::warning, txt, resp, static_cast<MsgBoxDefaultButton>(def_btn));
}

Response MsgBoxInfo(const string_view_utf8 &txt, const PhaseResponses &resp, size_t def_btn) {
    return msg_box(MsgBoxType::info, txt, resp, static_cast<MsgBoxDefaultButton>(def_btn));
}

Response MsgBoxPepaCentered(const string_view_utf8 &txt, const PhaseResponses &resp, size_t def_btn) {
    return msg_box(MsgBoxType::pepa_centered, txt, resp, static_cast<MsgBoxDefaultButton>(def_btn));
}
