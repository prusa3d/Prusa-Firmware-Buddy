#include "DialogFactory.hpp"
#include "i18n.h"
#include "bsod.h"
#include "DialogLoadUnload.hpp"
#include "window_dlg_preheat.hpp"
#include "window_dlg_quickpause.hpp"

DialogFactory::mem_space DialogFactory::all_dialogs;

// screens .. not used, return nullptr (to pass check in GetAll)
static_unique_ptr<IDialogMarlin> DialogFactory::screen_not_dialog(fsm::BaseData /*data*/) {
    return nullptr;
}

static_unique_ptr<IDialogMarlin> DialogFactory::load_unload(fsm::BaseData data) {
    return makePtr<DialogLoadUnload>(data);
}

static_unique_ptr<IDialogMarlin> DialogFactory::Preheat(fsm::BaseData data) {
    return makePtr<DialogMenuPreheat>(data);
}

static_unique_ptr<IDialogMarlin> DialogFactory::QuickPause(fsm::BaseData data) {
    return makePtr<DialogQuickPause>(data);
}

DialogFactory::Ctors DialogFactory::GetAll() {
    // todo check if all fields are set
    std::array<fnc, size_t(ClientFSM::_count)> ret = { nullptr };
    ret[size_t(ClientFSM::Serial_printing)] = screen_not_dialog;
    ret[size_t(ClientFSM::Printing)] = screen_not_dialog;
    ret[size_t(ClientFSM::CrashRecovery)] = screen_not_dialog;
    ret[size_t(ClientFSM::QuickPause)] = QuickPause;
    ret[size_t(ClientFSM::PrintPreview)] = screen_not_dialog;
    ret[size_t(ClientFSM::Load_unload)] = load_unload;
    ret[size_t(ClientFSM::Preheat)] = Preheat;
    ret[size_t(ClientFSM::Selftest)] = screen_not_dialog;
    ret[size_t(ClientFSM::ESP)] = screen_not_dialog;

    if (std::find(std::begin(ret), std::end(ret), nullptr) != std::end(ret)) {
        bsod("Error missing dialog Ctor"); // GUI init will throw this
    }

    return ret;
}
