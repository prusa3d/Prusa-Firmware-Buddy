// window_msgbox.hpp
#pragma once

#include "IDialog.hpp"
#include "radio_button.hpp"
#include "radio_button_fsm.hpp"
#include "window_text.hpp"
#include "window_icon.hpp"
#include "client_response.hpp"
#include "window_qr.hpp"
#include <guiconfig/guiconfig.h>

/*****************************************************************************/
// clang-format off
inline constexpr PhaseResponses Responses_NONE                 = { Response::_none,    Response::_none,  Response::_none,       Response::_none };
inline constexpr PhaseResponses Responses_Ok                   = { Response::Ok,       Response::_none,  Response::_none,       Response::_none };
inline constexpr PhaseResponses Responses_AbortRetryIgnore     = { Response::Abort,    Response::Retry,  Response::Ignore,      Response::_none };
inline constexpr PhaseResponses Responses_RetryAbort           = { Response::Retry,    Response::Abort,  Response::_none,       Response::_none };
inline constexpr PhaseResponses Responses_YesNo                = { Response::Yes,      Response::No,     Response::_none,       Response::_none };
inline constexpr PhaseResponses Responses_YesNoCancel          = { Response::Yes,      Response::No,     Response::Cancel,      Response::_none };
inline constexpr PhaseResponses Responses_YesNoIgnore          = { Response::Yes,      Response::No,     Response::Ignore,      Response::_none };
inline constexpr PhaseResponses Responses_YesRetry             = { Response::Yes,      Response::Retry,  Response::_none,       Response::_none };
inline constexpr PhaseResponses Responses_RetryCancel          = { Response::Retry,    Response::Cancel, Response::_none,       Response::_none };
inline constexpr PhaseResponses Responses_Disable              = { Response::Disable,  Response::_none,  Response::_none,       Response::_none };
inline constexpr PhaseResponses Responses_INVALID              = { Response::_last,    Response::_last,  Response::_last,       Response::_last };

// clang-format on
/*****************************************************************************/

void AdjustLayout(window_text_t &text, window_icon_t &icon);

/*****************************************************************************/
// MsgBoxBase
class MsgBoxBase : public AddSuperWindow<IDialog> {
protected:
    window_text_t text;

    // memory space to store radio buttons
    // template parameter <PhasesPrintPreview> is irrelevant - same size
    // in case it changes swap <PhasesPrintPreview> with the biggest type
    // it is checked in BindToFSM method
    using RadioMemSpace = std::aligned_union<0, RadioButton, RadioButtonFsm<PhasesPrintPreview>>::type;
    RadioMemSpace radio_mem_space;
    IRadioButton *pButtons = nullptr;
    Response result; // return value

public:
    MsgBoxBase(Rect16 rect, const PhaseResponses &resp, size_t def_btn, const PhaseTexts *labels,
        string_view_utf8 txt, is_multiline multiline = is_multiline::yes, is_closed_on_click_t close = is_closed_on_click_t::yes);

    [[nodiscard]] inline Response GetResult() const {
        return result;
    }

    template <class FSM_PHASE>
    void BindToFSM(FSM_PHASE phase) {
        static_assert(sizeof(RadioButtonFsm<FSM_PHASE>) <= sizeof(radio_mem_space), "RadioMemSpace is too small");

        if (!pButtons) { // pButtons can never be null
            assert("unassigned msgbox");
            return;
        }

        Rect16 rc = pButtons->GetRect();
        bool has_icon = pButtons->HasIcon();
        color_t back = pButtons->GetBackColor();

        ReleaseCaptureOfNormalWindow();
        pButtons->~IRadioButton();

        pButtons = new (&radio_mem_space) RadioButtonFsm<FSM_PHASE>(this, rc, phase);
        has_icon ? pButtons->SetHasIcon() : pButtons->ClrHasIcon();
        pButtons->SetBackColor(back);

        CaptureNormalWindow(*pButtons);
    }

    /// Sets response and generates appropriate events as if a button was pressed
    void generate_response(Response r);

    void set_text_alignment(Align_t alignment);
    void set_text_font(Font font);

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;

    Rect16 getTextRect();
};

/*****************************************************************************/
// MsgBoxIconned
class MsgBoxIconned : public AddSuperWindow<MsgBoxBase> {

public:
    MsgBoxIconned(Rect16 rect, const PhaseResponses &resp, size_t def_btn, const PhaseTexts *labels,
        string_view_utf8 txt, is_multiline multiline, const img::Resource *icon_res,
        is_closed_on_click_t close = is_closed_on_click_t::yes);

protected:
    window_icon_t icon;
    // some methods to help with construction, so they can't be virtual
    // some derived classes use them too, don't change visibility
    Rect16 getIconRect();
    Rect16 getTextRect();
};

/*****************************************************************************/
// MsgBoxTitled
class MsgBoxTitled : public AddSuperWindow<MsgBoxIconned> {
public:
    MsgBoxTitled(Rect16 rect, const PhaseResponses &resp, size_t def_btn, const PhaseTexts *labels,
        string_view_utf8 txt, is_multiline multiline, string_view_utf8 tit, const img::Resource *title_icon_res,
        is_closed_on_click_t close = is_closed_on_click_t::yes, dense_t dense = dense_t::no);

    void set_title_alignment(Align_t alignment);

protected:
    window_text_t title;

    // some methods to help with construction
    Rect16 getTextRect();
    Rect16 getLineRect();
    Rect16 getIconRect();
    Rect16 getTitleRect(); // icon must be initialized
    padding_ui8_t getTitlePadding();
    static constexpr uint8_t IconTitleDelimeter = 5;
    static constexpr padding_ui8_t TextPadding =
#if defined(USE_ST7789) || defined(USE_MOCK_DISPLAY)
        { 5, 0, 5, 0 };
#elif defined(USE_ILI9488)
        { 24, 24, 24, 24 };
#endif
};

/*****************************************************************************/
// MsgBoxPepa
class MsgBoxIconPepa : public AddSuperWindow<MsgBoxIconned> {
public:
    MsgBoxIconPepa(Rect16 rect, const PhaseResponses &resp, size_t def_btn, const PhaseTexts *labels,
        string_view_utf8 txt, is_multiline multiline, const img::Resource *icon);

protected:
    Rect16 getTextRect();
    Rect16 getIconRect();
};

/*****************************************************************************/
// MsgBoxPepaCentered
class MsgBoxIconPepaCentered : public AddSuperWindow<MsgBoxIconned> {
public:
    MsgBoxIconPepaCentered(Rect16 rect, const PhaseResponses &resp, size_t def_btn, const PhaseTexts *labels,
        string_view_utf8 txt, is_multiline multiline, const img::Resource *icon);

protected:
    Rect16 getTextRect();
    Rect16 getIconRect();
};

/*****************************************************************************/
// MsgBoxError
class MsgBoxIconnedError : public AddSuperWindow<MsgBoxIconned> {
public:
    MsgBoxIconnedError(Rect16 rect, const PhaseResponses &resp, size_t def_btn, const PhaseTexts *labels,
        string_view_utf8 txt, is_multiline multiline, const img::Resource *icon);
};

// MsgBoxWait
class MsgBoxIconnedWait : public AddSuperWindow<MsgBoxIconned> {
public:
    MsgBoxIconnedWait(Rect16 rect, const PhaseResponses &resp, size_t def_btn, const PhaseTexts *labels,
        string_view_utf8 txt, is_multiline multiline);
};

/*****************************************************************************/
// MsgBoxIS
class MsgBoxIS : public AddSuperWindow<MsgBoxBase> {

public:
    MsgBoxIS(Rect16 rect, const PhaseResponses &resp, size_t def_btn, const PhaseTexts *labels,
        string_view_utf8 txt, is_multiline multiline, const img::Resource *icon_res,
        is_closed_on_click_t close = is_closed_on_click_t::yes);

protected:
    window_icon_t icon;
    window_qr_t qr;

    // some methods to help with construction, so they can't be virtual
    // some derived classes use them too, don't change visibility
    Rect16 getIconRect();
    Rect16 getTextRect();

private:
    static auto constexpr QR_ADDR = "https://prusa.io/input-shaper";
};

enum class MsgBoxType : uint8_t {
    standard,
    titled,
    error, ///< Has default icon & title
    question, ///< Has default icon & title
    warning, ///< Has default icon & title
    info, ///< Has default icon & title
    pepa, ///< Has default icon
    pepa_centered, ///< Has default icon
    input_shaper_warning, ///< Has default icon
    _count,
};

enum class MsgBoxDefaultButton : uint8_t {
    b0,
    b1,
    b2,
    b3
};

struct MsgBoxBuilder {

public:
    /// Type/shape of the message box
    MsgBoxType type = MsgBoxType::standard;

    string_view_utf8 text;

    /// Title override. Effective only in some dialog types.
    /// Some dialog types provide a default icon.
    string_view_utf8 title = {};

    /// Icon override. Effective only in some dialog types.
    /// Some dialog types provide a default icon.
    const img::Resource *icon = nullptr;

    /// Responses override. Some dialog types have default responses.
    PhaseResponses responses = Responses_Ok;

    /// Index of the button that is to be selected as default
    MsgBoxDefaultButton default_button = MsgBoxDefaultButton::b0;

    Rect16 rect = GuiDefaults::DialogFrameRect;

    is_multiline multiline = is_multiline::yes;

public:
    /// Callback that is called after every loop tick during the dialog blocking execution
    std::function<void()> loop_callback = {};

public:
    /// Blocking displays the msg box, returns the response
    Response exec() const;
};

Response msg_box(MsgBoxType type, string_view_utf8 txt, const PhaseResponses &resp = Responses_Ok, MsgBoxDefaultButton default_button = MsgBoxDefaultButton::b0);

Response MsgBox(string_view_utf8 txt, const PhaseResponses &resp = Responses_NONE, size_t def_btn = 0);
Response MsgBoxError(string_view_utf8 txt, const PhaseResponses &resp = Responses_NONE, size_t def_btn = 0);
Response MsgBoxQuestion(string_view_utf8 txt, const PhaseResponses &resp = Responses_NONE, size_t def_btn = 0);
Response MsgBoxWarning(string_view_utf8 txt, const PhaseResponses &resp = Responses_NONE, size_t def_btn = 0);
Response MsgBoxInfo(string_view_utf8 txt, const PhaseResponses &resp = Responses_NONE, size_t def_btn = 0);
Response MsgBoxPepa(string_view_utf8 txt, const PhaseResponses &resp = Responses_NONE, size_t def_btn = 0);
Response MsgBoxPepaCentered(string_view_utf8 txt, const PhaseResponses &resp = Responses_NONE, size_t def_btn = 0);
Response MsgBoxISWarning(string_view_utf8 txt, const PhaseResponses &resp = Responses_NONE, size_t def_btn = 0);
