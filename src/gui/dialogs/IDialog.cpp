#include "IDialog.hpp"
#include <stdint.h>
#include "ScreenHandler.hpp"

IDialog::IDialog(Rect16 rc, IsStrong strong)
    : AddSuperWindow<window_frame_t>(strong == IsStrong::yes ? nullptr : Screens::Access()->Get(), rc, strong == IsStrong::yes ? win_type_t::strong_dialog : win_type_t::dialog) { // use dialog ctor
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

    while (!consumeCloseFlag()) {
        guiLoop();
        if (loopCallback) {
            loopCallback();
        }
    }

    screen->InitState(underlying_screen_state);
}

bool IDialog::consumeCloseFlag() const {
    return Screens::Access()->ConsumeClose();
}

void IDialog::guiLoop() const {
    gui::TickLoop();
    gui_loop();
}

void create_blocking_dialog_from_normal_window([[maybe_unused]] window_t &dlg) {
    while (!Screens::Access()->ConsumeClose()) {
        gui::TickLoop();
        gui_loop();
    }
}
