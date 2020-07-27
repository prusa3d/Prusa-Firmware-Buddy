// window_msgbox.hpp
#pragma once

#include "IDialog.hpp"
#include "DialogRadioButton.hpp"
#include "window_text.hpp"
#include "window_icon.hpp"
#include "../../lang/i18n.h"
#include "client_response.hpp"

//messagebox flags bitmasks
#define MSGBOX_MSK_ICO 0x0070 // icon mask
#define MSGBOX_MSK_BTN 0x000f // button config mask
#define MSGBOX_MSK_IDX 0x0300 // selected button index mask
#define MSGBOX_MSK_CHG 0x7000 // changed buttons mask

//messagebox flags bitshift
#define MSGBOX_SHI_ICO 4  // icon shift
#define MSGBOX_SHI_BTN 0  // button config shift
#define MSGBOX_SHI_IDX 8  // selected button index shift
#define MSGBOX_SHI_CHG 12 // changed buttons shift

//messagebox button config
#define MSGBOX_BTN_OK               0x0000 // 1 button  "OK"
#define MSGBOX_BTN_OKCANCEL         0x0001 // 2 buttons "OK" - "Cancel"
#define MSGBOX_BTN_ABORTRETRYIGNORE 0x0002 // 3 buttons "Abort" - "Retry" - "Ignore"
#define MSGBOX_BTN_YESNOCANCEL      0x0003 // 3 buttons "Yes" - "No" - "Cancel"
#define MSGBOX_BTN_YESNO            0x0004 // 2 buttons "Yes" - "No"
#define MSGBOX_BTN_RETRYCANCEL      0x0005 // 2 buttons "Retry" - "Cancel"
#define MSGBOX_BTN_CUSTOM1          0x0006 // 1 custom button
#define MSGBOX_BTN_CUSTOM2          0x0007 // 2 custom buttons
#define MSGBOX_BTN_CUSTOM3          0x0008 // 3 custom buttons
#define MSGBOX_BTN_MAX              MSGBOX_BTN_CUSTOM3

//messagebox icons
#define MSGBOX_ICO_CUSTOM   0x0000
#define MSGBOX_ICO_ERROR    0x0010
#define MSGBOX_ICO_QUESTION 0x0020
#define MSGBOX_ICO_WARNING  0x0030
#define MSGBOX_ICO_INFO     0x0040
#define MSGBOX_ICO_MAX      (MSGBOX_BTN_RETRYCANCEL >> MSGBOX_SHI_ICO)

//messagebox results (selected button)
#define MSGBOX_RES_CLOSED   0  // message box window was closed
#define MSGBOX_RES_CANCEL   2  // the Cancel button was selected
#define MSGBOX_RES_ABORT    3  // the Abort button was selected
#define MSGBOX_RES_RETRY    4  // the Retry button was selected
#define MSGBOX_RES_IGNORE   5  // the Ignore button was selected
#define MSGBOX_RES_YES      6  // the Yes button was selected
#define MSGBOX_RES_NO       7  // the No button was selected
#define MSGBOX_RES_OK       8  // the OK button was selected
#define MSGBOX_RES_TRYAGAIN 10 // the Try Again button was selected
#define MSGBOX_RES_CONTINUE 11 // the Continue button was selected
#define MSGBOX_RES_CUSTOM0  12 // custom button0 was selected
#define MSGBOX_RES_CUSTOM1  13 // custom button1 was selected
#define MSGBOX_RES_CUSTOM2  14 // custom button2 was selected

//messagebox default selected buttons
#define MSGBOX_DEF_BUTTON0 0x0000 // selected button 0
#define MSGBOX_DEF_BUTTON1 0x0100 // selected button 1
#define MSGBOX_DEF_BUTTON2 0x0200 // selected button 2

#define MSGBOX_GREY_FRAME 0x8000 // draw grey frame

struct window_msgbox_t : public IDialog {
    color_t color_text;
    font_t *font;
    font_t *font_title;
    padding_ui8_t padding;
    uint8_t alignment;
    string_view_utf8 title;
    uint16_t id_icon;
    string_view_utf8 text;
    const char *buttons[3];
    uint16_t flags;
    int res;
    window_msgbox_t(rect_ui16_t rect);

protected:
    void drawButtons();
    void step(int step);
    void click();

    virtual void windowEvent(window_t *sender, uint8_t event, void *param) override;
    virtual void unconditionalDraw() override;
};

extern uint16_t window_msgbox_id_icon[5];

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
    MsgBoxBase(rect_ui16_t rect, const PhaseResponses *resp, const PhaseTexts *labels, string_view_utf8 txt);
    Response GetResult();

protected:
    rect_ui16_t getTextRect();
    virtual void windowEvent(window_t *sender, uint8_t event, void *param) override;
};

/*****************************************************************************/
//MsgBoxTitled
class MsgBoxTitled : public MsgBoxBase {
    window_icon_t title_icon;
    window_text_t title;

public:
    MsgBoxTitled(rect_ui16_t rect, const PhaseResponses *resp, const PhaseTexts *labels, string_view_utf8 txt, string_view_utf8 tit, uint16_t title_icon_id_res);

protected:
    virtual void unconditionalDraw() override;

    //some methods to help with construction
    font_t *getTitleFont();
    padding_ui8_t getTitlePadding();
    rect_ui16_t getTitleRect();      // icon must be initialized
    rect_ui16_t getTitledTextRect(); // icon and title must be initialized
};

Response MsgBox(const PhaseResponses &resp, string_view_utf8 txt);
Response MsgBoxError(const PhaseResponses &resp, string_view_utf8 txt);
Response MsgBoxQuestion(const PhaseResponses &resp, string_view_utf8 txt);
Response MsgBoxWarning(const PhaseResponses &resp, string_view_utf8 txt);
Response MsgBoxInfo(const PhaseResponses &resp, string_view_utf8 txt);
Response MsgBoxIcon(const PhaseResponses &resp, string_view_utf8 txt);
