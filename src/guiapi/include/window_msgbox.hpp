// window_msgbox.hpp
#pragma once

#include "IDialog.hpp"
#include "DialogRadioButton.hpp"
#include "window_text.hpp"
#include "window_icon.hpp"
#include "../../lang/i18n.h"
#include "client_response.hpp"

extern const PhaseResponses Responses_NONE;
extern const PhaseResponses Responses_Ok;
extern const PhaseResponses Responses_OkCancel;
extern const PhaseResponses Responses_AbortRetryIgnore;
extern const PhaseResponses Responses_YesNo;
extern const PhaseResponses Responses_YesNoCancel;
extern const PhaseResponses Responses_RetryCancel;

/*****************************************************************************/
//MsgBoxBase
class MsgBoxBase : public IDialog {
protected:
    window_text_t text;
    RadioButton buttons;
    Response result; //return value
public:
    MsgBoxBase(Rect16 rect, const PhaseResponses *resp, const PhaseTexts *labels, string_view_utf8 txt);
    Response GetResult();

protected:
    Rect16 getTextRect();
    virtual void windowEvent(window_t *sender, uint8_t event, void *param) override;
};

/*****************************************************************************/
//MsgBoxTitled
class MsgBoxTitled : public MsgBoxBase {
    window_icon_t title_icon;
    window_text_t title;

public:
    MsgBoxTitled(Rect16 rect, const PhaseResponses *resp, const PhaseTexts *labels, string_view_utf8 txt, string_view_utf8 tit, uint16_t title_icon_id_res);

protected:
    virtual void unconditionalDraw() override;

    //some methods to help with construction
    font_t *getTitleFont();
    padding_ui8_t getTitlePadding();
    Rect16 getTitleRect();      // icon must be initialized
    Rect16 getTitledTextRect(); // icon and title must be initialized
};

/*****************************************************************************/
//MsgBoxIconned
class MsgBoxIconned : public MsgBoxBase {
    window_icon_t icon;

public:
    MsgBoxIconned(Rect16 rect, const PhaseResponses *resp, const PhaseTexts *labels, string_view_utf8 txt, uint16_t icon_id_res);

protected:
    //some methods to help with construction
    Rect16 getIconnedTextRect(); // icon and title must be initialized
};

//todo enum default button
//todo enum for size?
Response MsgBox(string_view_utf8 txt, const PhaseResponses &resp = Responses_NONE, size_t def_btn = 0, Rect16 rect = GuiDefaults::RectScreenBody);
Response MsgBoxError(string_view_utf8 txt, const PhaseResponses &resp = Responses_NONE, size_t def_btn = 0, Rect16 rect = GuiDefaults::RectScreenBody);
Response MsgBoxQuestion(string_view_utf8 txt, const PhaseResponses &resp = Responses_NONE, size_t def_btn = 0, Rect16 rect = GuiDefaults::RectScreenBody);
Response MsgBoxWarning(string_view_utf8 txt, const PhaseResponses &resp = Responses_NONE, size_t def_btn = 0, Rect16 rect = GuiDefaults::RectScreenBody);
Response MsgBoxInfo(string_view_utf8 txt, const PhaseResponses &resp = Responses_NONE, size_t def_btn = 0, Rect16 rect = GuiDefaults::RectScreenBody);
Response MsgBoxTitle(string_view_utf8 title, string_view_utf8 txt, const PhaseResponses &resp, size_t def_btn, Rect16 rect, uint16_t icon_id = 0);
Response MsgBoxIcon(string_view_utf8 txt, uint16_t icon_id, const PhaseResponses &resp = Responses_NONE, size_t def_btn = 0, Rect16 rect = GuiDefaults::RectScreenBody);
Response MsgBoxPepa(string_view_utf8 txt, const PhaseResponses &resp = Responses_NONE, size_t def_btn = 0, Rect16 rect = GuiDefaults::RectScreenBody);
