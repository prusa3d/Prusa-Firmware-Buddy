#pragma once

#include "DialogStateful.hpp"
#include "error_codes_mmu.hpp"
#include "window_icon.hpp"
#include "window_qr.hpp"
#include "status_footer.hpp"

/**
 * @brief radio button for red screens
 * workaround - DialogStateful already has an automatic radio button
 * but MMU red screens are many states masked as single state
 * automatic radio button cannot handle that
 */
class RadioButtonMmuErr : public AddSuperWindow<RadioButton> {
public:
    /**
     * @brief Construct a new Radio Button Mmu Err object
     *
     * @param parent window containing this object
     * @param rect   rectangle enclosing all buttons
     */
    RadioButtonMmuErr(window_t *parent, Rect16 rect);

protected:
    void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param);
};

/**
 * @brief load unload and change filament dialog
 * with MMU support
 * MMU error are handled extra and are red
 */
class DialogLoadUnload : public AddSuperWindow<DialogStateful<PhasesLoadUnload>> {
public:
    static constexpr uint8_t MaxErrorCodeDigits = 10;

private:
    StatusFooter footer;

    RadioButtonMmuErr radio_for_red_screen; // workaround, see RadioButtonMmuErr comment

    window_text_t text_link;
    window_icon_t icon_hand;
    window_qr_t qr;
    char error_code_str[32 + MaxErrorCodeDigits + 1]; // static text before error code has 32 chars
    PhaseResponses responses;
    LoadUnloadMode mode;

    static DialogLoadUnload *instance; // needed for sounds

protected:
    virtual bool change(uint8_t phase, fsm::PhaseData data) override;
    void red_screen_update(const MMU2::MMUErrDesc &err);
    virtual float deserialize_progress(fsm::PhaseData data) const override;

public:
    DialogLoadUnload(fsm::BaseData data);
    virtual ~DialogLoadUnload() override;

    static void phaseAlertSound();
    static void phaseWaitSound();
    static void phaseStopSound();

    static string_view_utf8 get_name(LoadUnloadMode mode);
    LoadUnloadMode get_mode() { return mode; }
};
