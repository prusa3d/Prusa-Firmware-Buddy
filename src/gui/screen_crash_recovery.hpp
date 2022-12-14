#pragma once

#include "screen.hpp"
#include "window_header.hpp"
#include "status_footer.hpp"
#include "fsm_base_types.hpp"
#include "window_icon.hpp"
#include "window_wizard_progress.hpp"
#include "radio_button.hpp"

class ScreenCrashRecovery;

namespace crash_recovery {

struct WinsCheckAxis {
    window_text_t text_long;
    window_icon_t icon_nozzle_crash;
    window_icon_t icon_nozzle;
    window_text_t text_checking_axis;
    window_progress_t line;
    window_text_t text_x_axis;
    WindowIcon_OkNg icon_x_axis;
    window_text_t text_y_axis;
    WindowIcon_OkNg icon_y_axis;

    WinsCheckAxis(ScreenCrashRecovery &screen);
};

struct WinsHome {
    window_text_t text_long;
    window_icon_t icon_nozzle_crash;
    window_icon_t icon_nozzle;
    window_progress_t line;
    window_text_t text_home_axes;
    WindowIcon_OkNg icon_home_axes;

    WinsHome(ScreenCrashRecovery &screen);
};

struct WinsAxisNok {
    window_text_t text_long;
    window_progress_t line;
    window_text_t text_x_axis;
    WindowIcon_OkNg icon_x_axis;
    window_text_t text_y_axis;
    WindowIcon_OkNg icon_y_axis;
    RadioButton radio;
    static constexpr PhaseTexts texts = { { "Retry", "Pause", "Resume" } };
    WinsAxisNok(ScreenCrashRecovery &screen);
};

struct WinsRepeatedCrash {
    window_text_t text_long;
    window_icon_t icon_nozzle_crash;
    window_icon_t icon_nozzle;
    RadioButton radio;
    static constexpr PhaseTexts texts = { { "Resume", "Pause" } };
    WinsRepeatedCrash(ScreenCrashRecovery &screen);
};

struct WinUnion {
    union {
        WinsCheckAxis *checkAxis;
        WinsHome *home;
        WinsAxisNok *axisNok;
        WinsRepeatedCrash *repeatedCrash;
    };

    enum screen_type {
        CheckAxis,
        Home,
        AxisNok,
        RepeatedCrash
    };

    using MemSpace = std::aligned_union<0, WinsCheckAxis, WinsHome, WinsAxisNok, WinsRepeatedCrash>::type;

    PhasesCrashRecovery phase;

    WinUnion(ScreenCrashRecovery &screen);
    void ChangePhase(PhasesCrashRecovery ph);
    void Destroy();                   // just to call destructor - to unregister windows from screen
    void New(PhasesCrashRecovery ph); // place new screen
    void ButtonEvent(GUI_event_t event);

private:
    MemSpace mem_space;
    ScreenCrashRecovery &parent_screen;
    screen_type ScreenType(PhasesCrashRecovery ph);
};

}

class ScreenCrashRecovery : public AddSuperWindow<screen_t> {
protected:
    window_header_t header;
    StatusFooter footer;

    crash_recovery::WinUnion win_union;

    static ScreenCrashRecovery *ths; // to be accessible in dialog handler

    virtual void windowEvent(EventLock /*has private ctor*/, window_t * /*sender*/, GUI_event_t event, void *param) override;

public:
    ScreenCrashRecovery();
    virtual ~ScreenCrashRecovery() override;
    static ScreenCrashRecovery *GetInstance();
    bool Change(fsm::BaseData data);
};
