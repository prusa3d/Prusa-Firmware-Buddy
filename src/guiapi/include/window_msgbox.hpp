// window_msgbox.hpp
#pragma once

#include "IDialog.hpp"
#include "DialogRadioButton.hpp"
#include "window_text.hpp"
#include "window_icon.hpp"
#include "../../lang/i18n.h"
#include "client_response.hpp"

/*****************************************************************************/
// clang-format off
static constexpr PhaseResponses Responses_NONE                 = { Response::_none,    Response::_none,  Response::_none,  Response::_none };
static constexpr PhaseResponses Responses_Next                 = { Response::Next,     Response::_none,  Response::_none,  Response::_none };
static constexpr PhaseResponses Responses_Ok                   = { Response::Ok,       Response::_none,  Response::_none,  Response::_none };
static constexpr PhaseResponses Responses_OkCancel             = { Response::Ok,       Response::Cancel, Response::_none,  Response::_none };
static constexpr PhaseResponses Responses_AbortRetryIgnore     = { Response::Abort,    Response::Retry,  Response::Ignore, Response::_none };
static constexpr PhaseResponses Responses_RetryAbort           = { Response::Retry,    Response::Abort,  Response::_none,  Response::_none };
static constexpr PhaseResponses Responses_YesNo                = { Response::Yes,      Response::No,     Response::_none,  Response::_none };
static constexpr PhaseResponses Responses_YesNoCancel          = { Response::Yes,      Response::No,     Response::Cancel, Response::_none };
static constexpr PhaseResponses Responses_YesNoIgnore          = { Response::Yes,      Response::No,     Response::Ignore, Response::_none };
static constexpr PhaseResponses Responses_YesAbort             = { Response::Yes,      Response::Abort,  Response::_none,  Response::_none };
static constexpr PhaseResponses Responses_YesCancel            = { Response::Yes,      Response::Cancel, Response::_none,  Response::_none };
static constexpr PhaseResponses Responses_YesRetry             = { Response::Yes,      Response::Retry,  Response::_none,  Response::_none };
static constexpr PhaseResponses Responses_RetryCancel          = { Response::Retry,    Response::Cancel, Response::_none,  Response::_none };
static constexpr PhaseResponses Responses_ChangeIgnoreCancel   = { Response::Change,   Response::Ignore, Response::Cancel, Response::_none };
static constexpr PhaseResponses Responses_ChangeIgnoreAbort    = { Response::Change,   Response::Ignore, Response::Abort,  Response::_none };
static constexpr PhaseResponses Responses_IgnoreAbort          = { Response::Ignore,   Response::Abort,  Response::_none,  Response::_none };

// clang-format on
/*****************************************************************************/

/*****************************************************************************/
//MsgBoxBase
class MsgBoxBase : public AddSuperWindow<IDialog> {
protected:
    window_text_t text;
    RadioButton buttons;
    Response result; //return value
public:
    MsgBoxBase(Rect16 rect, const PhaseResponses &resp, size_t def_btn, const PhaseTexts *labels,
        string_view_utf8 txt, is_multiline multiline = is_multiline::yes);
    Response GetResult();

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
    Rect16 getTextRect();
};

/*****************************************************************************/
//MsgBoxIconned
class MsgBoxIconned : public AddSuperWindow<MsgBoxBase> {

public:
    MsgBoxIconned(Rect16 rect, const PhaseResponses &resp, size_t def_btn, const PhaseTexts *labels,
        string_view_utf8 txt, is_multiline multiline, uint16_t icon_id_res);

protected:
    window_icon_t icon;
    // some methods to help with construction, so they can't be virtual
    // some derived classes use them too, don't change visibility
    Rect16 getIconRect();
    Rect16 getTextRect();
};

/*****************************************************************************/
//MsgBoxTitled
class MsgBoxTitled : public AddSuperWindow<MsgBoxIconned> {
public:
    MsgBoxTitled(Rect16 rect, const PhaseResponses &resp, size_t def_btn, const PhaseTexts *labels,
        string_view_utf8 txt, is_multiline multiline, string_view_utf8 tit, uint16_t title_icon_id_res);

protected:
    window_text_t title;
    virtual void unconditionalDraw() override;

private:
    //some methods to help with construction
    Rect16 getTextRect();
    Rect16 getLineRect();
    Rect16 getIconRect();
    Rect16 getTitleRect(); // icon must be initialized
    font_t *getTitleFont();
    padding_ui8_t getTitlePadding();
};

//todo enum default button
//todo enum for size?
Response MsgBox(string_view_utf8 txt, const PhaseResponses &resp = Responses_NONE, size_t def_btn = 0, Rect16 rect = GuiDefaults::DialogFrameRect, is_multiline multiline = is_multiline::yes);
Response MsgBoxError(string_view_utf8 txt, const PhaseResponses &resp = Responses_NONE, size_t def_btn = 0, Rect16 rect = GuiDefaults::DialogFrameRect, is_multiline multiline = is_multiline::yes);
Response MsgBoxQuestion(string_view_utf8 txt, const PhaseResponses &resp = Responses_NONE, size_t def_btn = 0, Rect16 rect = GuiDefaults::DialogFrameRect, is_multiline multiline = is_multiline::yes);
Response MsgBoxWarning(string_view_utf8 txt, const PhaseResponses &resp = Responses_NONE, size_t def_btn = 0, Rect16 rect = GuiDefaults::DialogFrameRect, is_multiline multiline = is_multiline::yes);
Response MsgBoxInfo(string_view_utf8 txt, const PhaseResponses &resp = Responses_NONE, size_t def_btn = 0, Rect16 rect = GuiDefaults::DialogFrameRect, is_multiline multiline = is_multiline::yes);
Response MsgBoxTitle(string_view_utf8 title, string_view_utf8 txt, const PhaseResponses &resp = Responses_NONE, size_t def_btn = 0, Rect16 rect = GuiDefaults::DialogFrameRect, uint16_t icon_id = 0, is_multiline multiline = is_multiline::yes);
Response MsgBoxIcon(string_view_utf8 txt, uint16_t icon_id, const PhaseResponses &resp = Responses_NONE, size_t def_btn = 0, Rect16 rect = GuiDefaults::DialogFrameRect, is_multiline multiline = is_multiline::yes);
Response MsgBoxPepa(string_view_utf8 txt, const PhaseResponses &resp = Responses_NONE, size_t def_btn = 0, Rect16 rect = GuiDefaults::DialogFrameRect, is_multiline multiline = is_multiline::yes);
