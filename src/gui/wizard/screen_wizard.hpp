// screen_wizard.hpp
#pragma once

#include "gui.hpp"
#include "window_header.hpp"
#include "status_footer.h"
#include "wizard_types.hpp"

string_view_utf8 WizardGetCaption(WizardState_t st); //todo constexpr

class ScreenWizard : public window_frame_t {
    window_header_t header;
    status_footer_t footer;

    using StateFnc = StateFncData (*)(StateFncData last_run);
    using StateArray = std::array<StateFnc, size_t(WizardState_t::last) + 1>;
    using ResultArray = std::array<WizardTestState_t, size_t(WizardState_t::last) + 1>;

    static StateArray states;
    static StateArray StateInitializer();

    ResultArray results;
    static ResultArray ResultInitializer(uint64_t mask);

    WizardState_t state;
    bool loopInProgress;

    static uint64_t run_mask;

protected:
    void windowEvent(window_t *sender, uint8_t event, void *param) override;

public:
    ScreenWizard();
    ~ScreenWizard();

    static constexpr void SetMaskAll() { run_mask = WizardMaskAll(); }
    static uint64_t GetMask() { return run_mask; }
};
