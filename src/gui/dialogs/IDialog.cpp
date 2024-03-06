#include "IDialog.hpp"
#include <stdint.h>
#include "ScreenHandler.hpp"

IDialog::IDialog(Rect16 rc)
    : AddSuperWindow<window_frame_t>(Screens::Access()->Get(), rc, win_type_t::dialog) {
    Enable();
}

IDialog::IDialog(window_t *parent, Rect16 rc)
    : AddSuperWindow<window_frame_t>(parent, rc, win_type_t::dialog) {
    Enable();
}

void IDialog::MakeBlocking(std::function<void()> loopCallback) const {
    auto screen = Screens::Access()->Get();
    assert(screen);
    auto underlying_screen_state = screen->GetCurrentState();

    while (!Screens::Access()->ConsumeClose()) {
        gui::TickLoop();
        gui_loop();
        if (loopCallback) {
            loopCallback();
        }
    }

    screen->InitState(underlying_screen_state);
}
