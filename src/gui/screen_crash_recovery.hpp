#pragma once

#include "screen.hpp"
#include "window_header.hpp"
#include "status_footer.hpp"
#include <common/fsm_base_types.hpp>
#include "window_wizard_icon.hpp"
#include "radio_button.hpp"
#include <option/has_toolchanger.h>
#include <option/has_tool_crash_recovery.h>
#include <option/has_dwarf.h>
#include <gui/standard_frame/frame_wait.hpp>

class ScreenCrashRecovery;

namespace crash_recovery {

class RepeatedBeep {
public:
    RepeatedBeep();
    ~RepeatedBeep();
};

class SingleBeep {
public:
    SingleBeep();
    ~SingleBeep();
};

struct WinsCheckAxis {
    window_text_t text_long;
    window_icon_t icon_nozzle_crash;
    window_icon_t icon_nozzle;
    window_text_t text_checking_axis;
    BasicWindow line;
    window_text_t text_x_axis;
    WindowIcon_OkNg icon_x_axis;
    window_text_t text_y_axis;
    WindowIcon_OkNg icon_y_axis;
    SingleBeep beep;

    WinsCheckAxis(ScreenCrashRecovery &screen);
};

struct WinsHome {
    window_text_t text_long;
    window_icon_t icon_nozzle_crash;
    window_icon_t icon_nozzle;
    BasicWindow line;
    window_text_t text_home_axes;
    WindowIcon_OkNg icon_home_axes;
    SingleBeep beep;

    WinsHome(ScreenCrashRecovery &screen);
};

struct WinsAxisNok {
    window_text_t text_long;
    BasicWindow line;
    window_text_t text_x_axis;
    WindowIcon_OkNg icon_x_axis;
    window_text_t text_y_axis;
    WindowIcon_OkNg icon_y_axis;
    RadioButton radio;
    RepeatedBeep beep;

    WinsAxisNok(ScreenCrashRecovery &screen);
};

struct WinsRepeatedCrash {
    window_text_t text_long;
    window_icon_t icon_nozzle_crash;
    window_icon_t icon_nozzle;
    window_text_t text_info;
    RadioButton radio;
    RepeatedBeep beep;

    WinsRepeatedCrash(ScreenCrashRecovery &screen);
};

struct WinsHomeFail {
    window_text_t text_long;
    window_icon_t icon_nozzle_crash;
    window_icon_t icon_nozzle;
    window_text_t text_info;
    RadioButton radio;
    RepeatedBeep beep;

    WinsHomeFail(ScreenCrashRecovery &screen);
};

class WinsGCodeInterrupt : public window_frame_t {

public:
    WinsGCodeInterrupt(ScreenCrashRecovery &screen);

private:
    FrameWait frame_;
};

#if HAS_TOOL_CRASH_RECOVERY() && HAS_DWARF()
struct WinsToolRecovery {
    window_text_t text_long;
    window_text_t text_careful;
    window_text_t text_tool[EXTRUDERS];
    WindowIcon_OkNg icon_tool[EXTRUDERS];
    RadioButton radio;
    RepeatedBeep beep;

    WinsToolRecovery(ScreenCrashRecovery &screen);
};
#endif

using WinVariant = std::variant<std::nullopt_t,
#if HAS_TOOL_CRASH_RECOVERY() && HAS_DWARF()
    WinsToolRecovery,
#endif
    WinsCheckAxis, WinsHome, WinsAxisNok, WinsRepeatedCrash, WinsHomeFail, WinsGCodeInterrupt>;

} // namespace crash_recovery

class ScreenCrashRecovery : public screen_t {
protected:
    window_header_t header;
    StatusFooter footer;
    crash_recovery::WinVariant window;

    virtual void windowEvent(window_t * /*sender*/, GUI_event_t event, void *param) override;

public:
    ScreenCrashRecovery();
    ~ScreenCrashRecovery();
    bool Change(fsm::BaseData data);

private:
    void change_phase(PhasesCrashRecovery ph);

    std::optional<PhasesCrashRecovery> current_phase;
};
