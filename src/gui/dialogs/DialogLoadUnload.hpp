#pragma once

#include "DialogStateful.hpp"
#include "error_codes_mmu.hpp"
#include "window_icon.hpp"
#include "window_qr.hpp"
#include "status_footer.hpp"
#include "window_colored_rect.hpp"

/**
 * @brief radio button for red screens
 * workaround - DialogStateful already has an automatic radio button
 * but MMU red screens are many states masked as single state
 * automatic radio button cannot handle that
 */
class RadioButtonNotice : public AddSuperWindow<RadioButton> {
    PhasesLoadUnload current_phase;

public:
    /**
     * @brief Construct a new Radio Button Mmu Err object
     *
     * @param parent window containing this object
     * @param rect   rectangle enclosing all buttons
     */
    RadioButtonNotice(window_t *parent, Rect16 rect);
    void ChangePhase(PhasesLoadUnload phase, PhaseResponses responses);

protected:
    void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};

/**
 * @brief load unload and change filament dialog
 * with MMU support
 * MMU error are handled extra and are red
 */
class DialogLoadUnload : public AddSuperWindow<DialogStateful<PhasesLoadUnload>> {

public:
    static constexpr uint8_t MaxErrorCodeDigits = 10;

public:
    DialogLoadUnload(fsm::BaseData data);
    virtual ~DialogLoadUnload() override;

    static void phaseAlertSound();
    static void phaseWaitSound();
    static void phaseStopSound();

    static string_view_utf8 get_name(LoadUnloadMode mode);
    LoadUnloadMode get_mode() { return mode; }

public:
#if HAS_MMU2()
    /// Returns whether there is a dialog open on an MMU error screen (that is waiting for user input)
    static inline bool is_mmu2_error_screen_running() {
        return instance && instance->current_phase == PhasesLoadUnload::MMU_ERRWaitingForUser;
    }
#endif

protected:
    virtual bool change(PhasesLoadUnload phase, fsm::PhaseData data) override;
    void notice_update(uint16_t errCode, const char *errTitle, const char *errDesc, MMU2::ErrType type);
    virtual float deserialize_progress(fsm::PhaseData data) const override;
    void phaseEnter() override;

private:
    StatusFooter footer;

    window_frame_t notice_frame;

    window_text_t notice_title;
    window_text_t notice_text;
    window_text_t notice_link;
    window_icon_t notice_icon_hand;
    window_icon_t notice_icon_type;
    window_qr_t notice_qr;
    RadioButtonNotice notice_radio_button; // workaround, see RadioButtonNotice comment

    window_text_t filament_type_text;
    window_colored_rect filament_color_icon;

    char error_code_str[32 + MaxErrorCodeDigits + 1]; // static text before error code has 32 chars
    LoadUnloadMode mode;

    static DialogLoadUnload *instance; // needed for sounds
};
