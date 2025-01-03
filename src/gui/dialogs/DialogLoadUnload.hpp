#pragma once

#include "client_response.hpp"
#include "error_codes_mmu.hpp"
#include "i18n.h"
#include "IDialogMarlin.hpp"
#include "radio_button_fsm.hpp"
#include "status_footer.hpp"
#include "window_colored_rect.hpp"
#include "window_icon.hpp"
#include "window_numb.hpp"
#include "window_progress.hpp"
#include "window_text.hpp"
#include <gui/text_error_url.hpp>
#include <gui/qr.hpp>
#include <optional>

/**
 * @brief radio button for red screens
 * workaround - DialogLoadUnload already has an automatic radio button
 * but MMU red screens are many states masked as single state
 * automatic radio button cannot handle that
 */
class RadioButtonNotice : public RadioButton {
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
    void windowEvent(window_t *sender, GUI_event_t event, void *param) override;
};

/**
 * @brief load unload and change filament dialog
 * with MMU support
 * MMU error are handled extra and are red
 */
class DialogLoadUnload final : public IDialogMarlin {
private:
    window_frame_t progress_frame;
    window_text_t title;
    window_numberless_progress_t progress_bar;
    window_numb_t progress_number;
    window_text_t label;
    std::optional<PhasesLoadUnload> current_phase = std::nullopt;
    RadioButtonFSM radio;

    void set_progress_percent(uint8_t val);

public:
    void Change(fsm::BaseData data) override final;

    static constexpr uint8_t MaxErrorCodeDigits = 10;

    DialogLoadUnload(fsm::BaseData data);
    ~DialogLoadUnload();

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
    void notice_update(uint16_t errCode, const char *errTitle, const char *errDesc, ErrType type);
    float deserialize_progress(fsm::PhaseData data) const;
    void phaseEnter();

private:
    StatusFooter footer;

    window_frame_t notice_frame;

    window_text_t notice_title;
    window_text_t notice_text;
    TextErrorUrlWindow notice_link;
    window_icon_t notice_icon_hand;
    window_icon_t notice_icon_type;
    QRErrorUrlWindow notice_qr;
    RadioButtonNotice notice_radio_button; // workaround, see RadioButtonNotice comment

    window_text_t filament_type_text;
    window_colored_rect filament_color_icon;

    LoadUnloadMode mode;

    // Needs to be held in memory because we're rendering the name from it
    FilamentTypeParameters filament_type_parameters;

    static DialogLoadUnload *instance; // needed for sounds
};
