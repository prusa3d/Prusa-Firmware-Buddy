#pragma once

#include "DialogStateful.hpp"
#include "../../../lib/Prusa-Error-Codes/04_MMU/errors_list.h"
#include "window_icon.hpp"
#include "window_qr.hpp"
#include "status_footer.hpp"

//load unload and change filament dialog
class DialogLoadUnload : public AddSuperWindow<DialogStateful<PhasesLoadUnload>> {
public:
    static constexpr uint8_t MaxErrorCodeDigits = 10;

private:
    StatusFooter footer;

    /**
     * @brief radio button for red screens
     * workaround - DialogStateful already has an automatic radio button
     * but MMU red screens are many states masked as single state
     * automatic radio button cannot handle that
     */
    RadioButton radio_for_red_screen;

    window_text_t text_link;
    window_icon_t icon_hand;
    window_qr_t qr;
    char error_code_str[32 + MaxErrorCodeDigits + 1]; // static text before error code has 32 chars
    PhaseResponses responses;

protected:
    virtual bool change(uint8_t phase, fsm::PhaseData data) override;
    void red_screen_update(const MMU2::MMUErrorDesc &err);

public:
    DialogLoadUnload(string_view_utf8 name);
    virtual ~DialogLoadUnload() override;

    static bool is_M600_phase;

    static void phaseAlertSound();
    static void phaseWaitSound();
    static void phaseStopSound();
};
