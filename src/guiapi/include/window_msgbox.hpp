// window_msgbox.hpp
#pragma once

#include "IDialog.hpp"
#include "radio_button.hpp"
#include "radio_button_fsm.hpp"
#include "window_text.hpp"
#include "window_icon.hpp"
#include "client_response.hpp"
#include <guiconfig/guiconfig.h>
#include <static_alocation_ptr.hpp>
#include <inplace_function.hpp>

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
class MsgBoxBase : public IDialog {
protected:
    window_text_t text;

    // memory space to store radio buttons
    // template parameter <PhasesPrintPreview> is irrelevant - same size
    // in case it changes swap <PhasesPrintPreview> with the biggest type
    // it is checked in BindToFSM method
    static constexpr size_t mem_space_size = std::max({ sizeof(RadioButtonFsm<PhasesPrintPreview>), sizeof(RadioButton) });
    using RadioMemSpace = std::array<uint8_t, mem_space_size>;
    alignas(std::max_align_t) RadioMemSpace radio_mem_space;
    static_unique_ptr<IRadioButton> pButtons;
    Response result = Response::_none; // return value

public:
    MsgBoxBase(Rect16 rect, const PhaseResponses &resp, size_t def_btn, const PhaseTexts *labels,
        const string_view_utf8 &txt, is_multiline multiline = is_multiline::yes, is_closed_on_click_t close = is_closed_on_click_t::yes);

    [[nodiscard]] inline Response GetResult() const {
        return result;
    }

    template <class FSM_PHASE>
    void BindToFSM(FSM_PHASE phase) {
        using T = RadioButtonFsm<FSM_PHASE>;
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

    /// Sets response and generates appropriate events as if a button was pressed
    void generate_response(Response r);

    void set_text_alignment(Align_t alignment);
    void set_text_font(Font font);

protected:
    virtual void windowEvent(window_t *sender, GUI_event_t event, void *param) override;

    Rect16 getTextRect();
};

/*****************************************************************************/
// MsgBoxIconned
class MsgBoxIconned : public MsgBoxBase {

public:
    MsgBoxIconned(Rect16 rect, const PhaseResponses &resp, size_t def_btn, const PhaseTexts *labels,
        const string_view_utf8 &txt, is_multiline multiline, const img::Resource *icon_res,
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
class MsgBoxTitled : public MsgBoxIconned {
public:
    MsgBoxTitled(Rect16 rect, const PhaseResponses &resp, size_t def_btn, const PhaseTexts *labels,
        const string_view_utf8 &txt, is_multiline multiline, const string_view_utf8 &tit, const img::Resource *title_icon_res,
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
#if HAS_MINI_DISPLAY() || HAS_MOCK_DISPLAY()
        { 5, 0, 5, 0 };
#elif HAS_LARGE_DISPLAY()
        { 24, 24, 24, 24 };
#endif
};

/*****************************************************************************/
// MsgBoxPepaCentered
class MsgBoxIconPepaCentered : public MsgBoxIconned {
public:
    MsgBoxIconPepaCentered(Rect16 rect, const PhaseResponses &resp, size_t def_btn, const PhaseTexts *labels,
        const string_view_utf8 &txt, is_multiline multiline, const img::Resource *icon);

protected:
    Rect16 getTextRect();
    Rect16 getIconRect();
};

/*****************************************************************************/
// MsgBoxError
class MsgBoxIconnedError : public MsgBoxIconned {
public:
    MsgBoxIconnedError(Rect16 rect, const PhaseResponses &resp, size_t def_btn, const PhaseTexts *labels,
        const string_view_utf8 &txt, is_multiline multiline, const img::Resource *icon);
};

// MsgBoxWait
class MsgBoxIconnedWait : public MsgBoxIconned {
public:
    MsgBoxIconnedWait(Rect16 rect, const PhaseResponses &resp, size_t def_btn, const PhaseTexts *labels,
        const string_view_utf8 &txt, is_multiline multiline);
};

enum class MsgBoxType : uint8_t {
    standard,
    titled,
    error, ///< Has default icon & title
    question, ///< Has default icon & title
    warning, ///< Has default icon & title
    info, ///< Has default icon & title
    pepa_centered, ///< Has default icon
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
    stdext::inplace_function<void()> loop_callback = {};

public:
    /// Blocking displays the msg box, returns the response
    Response exec() const;
};

Response msg_box(MsgBoxType type, const string_view_utf8 &txt, const PhaseResponses &resp = Responses_Ok, MsgBoxDefaultButton default_button = MsgBoxDefaultButton::b0);

Response MsgBox(const string_view_utf8 &txt, const PhaseResponses &resp = Responses_NONE, size_t def_btn = 0);
Response MsgBoxError(const string_view_utf8 &txt, const PhaseResponses &resp = Responses_NONE, size_t def_btn = 0);
Response MsgBoxQuestion(const string_view_utf8 &txt, const PhaseResponses &resp = Responses_NONE, size_t def_btn = 0);
Response MsgBoxWarning(const string_view_utf8 &txt, const PhaseResponses &resp = Responses_NONE, size_t def_btn = 0);
Response MsgBoxInfo(const string_view_utf8 &txt, const PhaseResponses &resp = Responses_NONE, size_t def_btn = 0);
Response MsgBoxPepaCentered(const string_view_utf8 &txt, const PhaseResponses &resp = Responses_NONE, size_t def_btn = 0);
