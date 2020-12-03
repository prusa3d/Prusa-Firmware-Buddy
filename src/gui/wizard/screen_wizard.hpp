// screen_wizard.hpp
#pragma once

#include "gui.hpp"
#include "window_header.hpp"
#include "status_footer.h"
#include "wizard_types.hpp"
#include "screen.hpp"

class ScreenWizard : public AddSuperWindow<screen_t> {
    enum class caption_t {
        all,
        selftest,
        xyz,
        firstlay
    };

    window_header_t header;
    status_footer_t footer;

    using StateFnc = WizardState_t (*)();
    using StateArray = std::array<StateFnc, size_t(WizardState_t::last) + 1>;

    static StateArray states;
    static StateArray StateInitializer();

    WizardState_t state;
    bool loopInProgress;

    static uint64_t run_mask;
    static WizardState_t start_state;
    static caption_t caption_type;

    static bool is_config_invalid;

    static string_view_utf8 WizardGetCaption(WizardState_t st, caption_t type); //todo constexpr

protected:
    void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;

public:
    static void ChangeStartState(WizardState_t state) { start_state = state; }
    ScreenWizard();

    static void RunAll();
    static void RunSelfTest();
    static void RunXYZCalib();
    static void RunFirstLay();
    static void RunFirstLayerStandAlone();

    static uint64_t GetMask() { return run_mask; }
    static bool IsConfigInvalid() { return is_config_invalid; }
};
