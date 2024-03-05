#include "IDialog.hpp"

#include "ScreenHandler.hpp"
#include "log.h"

LOG_COMPONENT_REF(GUI);

IDialog::IDialog(Rect16 rc)
    : IDialog(Screens::Access()->Get(), rc) {
}

static screen_init_variant get_underlying_screen_state() {
    if (screen_t *screen = Screens::Access()->Get()) {
        return screen->GetCurrentState();
    } else {
        // TODO investigate if this happens in the wild after we collect some telemetry
        log_info(GUI, "get_underlying_screen_state() without screen");
        return screen_init_variant {};
    }
}

static void set_underlying_screen_state(const screen_init_variant &underlying_screen_state) {
    if (screen_t *screen = Screens::Access()->Get()) {
        screen->InitState(underlying_screen_state);
    } else {
        // TODO investigate if this happens in the wild after we collect some telemetry
        log_info(GUI, "set_underlying_screen_state() without screen");
    }
}

IDialog::IDialog(window_t *parent, Rect16 rc)
    : AddSuperWindow<window_frame_t>(parent, rc, win_type_t::dialog)
    , underlying_screen_state { get_underlying_screen_state() } {
    Enable();
}

IDialog::~IDialog() {
    set_underlying_screen_state(underlying_screen_state);
}
