// screen_wizard.hpp
#pragma once

#include "gui.hpp"
#include "window_header.hpp"
#include "status_footer.hpp"
#include "wizard_types.hpp"
#include "screen.hpp"

class ScreenWizard : public AddSuperWindow<screen_t> {
    window_header_t header;
    StatusFooter footer;

    using StateFnc = WizardState_t (*)();
    using StateArray = std::array<StateFnc, size_t(WizardState_t::last) + 1>;

    static StateArray states;
    static StateArray StateInitializer();

    WizardState_t state;
    bool loopInProgress;

    static uint64_t run_mask;
    static WizardState_t start_state;
    static wizard_run_type_t caption_type;

    static bool is_config_invalid;

    static string_view_utf8 WizardGetCaption(WizardState_t st, wizard_run_type_t type); //todo constexpr

protected:
    void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;

public:
    static void ChangeStartState(WizardState_t state) { start_state = state; }
    ScreenWizard();

    static void Run(wizard_run_type_t type);

    static uint64_t GetMask() { return run_mask; }
    static bool IsConfigInvalid() { return is_config_invalid; }
};
