// window_msgbox.hpp
#pragma once

#include "IDialog.hpp"
#include "DialogRadioButton.hpp"
#include "window_text.hpp"
#include "window_icon.hpp"
#include "../../lang/i18n.h"
#include "client_response.hpp"

extern const PhaseResponses Responses_NONE;
extern const PhaseResponses Responses_Next;
extern const PhaseResponses Responses_Ok;
extern const PhaseResponses Responses_OkCancel;
extern const PhaseResponses Responses_AbortRetryIgnore;
extern const PhaseResponses Responses_YesNo;
extern const PhaseResponses Responses_YesNoCancel;
extern const PhaseResponses Responses_RetryCancel;
extern const PhaseResponses Responses_ChangeIgnoreCancel;

/*****************************************************************************/
//MsgBoxBase
class MsgBoxBase : public AddSuperWindow<IDialog> {
protected:
    window_text_t text;
    RadioButton buttons;
    Response result; //return value
public:
    MsgBoxBase(Rect16 rect, const PhaseResponses *resp, size_t def_btn, const PhaseTexts *labels,
        string_view_utf8 txt, is_multiline multiline = is_multiline::yes);
    Response GetResult();

protected:
    virtual Rect16 getTextRect();
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};

/*****************************************************************************/
//MsgBoxIconned
class MsgBoxIconned : public AddSuperWindow<MsgBoxBase> {

public:
    MsgBoxIconned(Rect16 rect, const PhaseResponses *resp, size_t def_btn, const PhaseTexts *labels,
        string_view_utf8 txt, is_multiline multiline, uint16_t icon_id_res);

protected:
    window_icon_t icon;
    //some methods to help with construction
    virtual Rect16 getIconRect(); // compute icon rect
    virtual Rect16 getTextRect() override;
};

/*****************************************************************************/
//MsgBoxTitled
class MsgBoxTitled : public AddSuperWindow<MsgBoxIconned> {
public:
    MsgBoxTitled(Rect16 rect, const PhaseResponses *resp, size_t def_btn, const PhaseTexts *labels,
        string_view_utf8 txt, is_multiline multiline, string_view_utf8 tit, uint16_t title_icon_id_res);

protected:
    window_text_t title;

    virtual void unconditionalDraw() override;
    virtual Rect16 getTextRect() override;
    virtual Rect16 getIconRect() override;
    virtual Rect16 getTitleRect(); // icon must be initialized
private:
    //some methods to help with construction
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
Response MsgBoxTitle(string_view_utf8 title, string_view_utf8 txt, const PhaseResponses &resp, size_t def_btn, Rect16 rect, uint16_t icon_id = 0, is_multiline multiline = is_multiline::yes);
Response MsgBoxIcon(string_view_utf8 txt, uint16_t icon_id, const PhaseResponses &resp = Responses_NONE, size_t def_btn = 0, Rect16 rect = GuiDefaults::DialogFrameRect, is_multiline multiline = is_multiline::yes);
Response MsgBoxPepa(string_view_utf8 txt, const PhaseResponses &resp = Responses_NONE, size_t def_btn = 0, Rect16 rect = GuiDefaults::DialogFrameRect, is_multiline multiline = is_multiline::yes);
