#include "DialogHandler.hpp"

#include "DialogLoadUnload.hpp"
#include "IScreenPrinting.hpp"
#include "ScreenHandler.hpp"
#include "ScreenESP.hpp"
#include "screen_printing.hpp"
#include "config_features.h"
#include "screen_print_preview.hpp"
#include "window_dlg_preheat.hpp"
#include "window_dlg_quickpause.hpp"
#include "window_dlg_warning.hpp"
#include <option/has_phase_stepping.h>

#if HAS_COLDPULL()
    #include "screen_cold_pull.hpp"
#endif

#if HAS_SELFTEST()
    #include "ScreenSelftest.hpp"
#endif

#if ENABLED(CRASH_RECOVERY)
    #include "screen_crash_recovery.hpp"
using CrashRecovery = ScreenCrashRecovery;
#else
    #include "screen_dialog_does_not_exist.hpp"
using CrashRecovery = ScreenDialogDoesNotExist;
#endif

#include <option/has_serial_print.h>
#if HAS_SERIAL_PRINT()
    #include "screen_printing_serial.hpp"
using SerialPrint = screen_printing_serial_data_t;
#else
    #include "screen_dialog_does_not_exist.hpp"
using SerialPrint = ScreenDialogDoesNotExist;
#endif

#if HAS_PHASE_STEPPING()
    #include "screen_phase_stepping.hpp"
#endif

using mem_space = std::aligned_union_t<0, DialogQuickPause, DialogLoadUnload, DialogMenuPreheat, DialogWarning
#if HAS_COLDPULL()
    ,
    ScreenColdPull
#endif
#if HAS_PHASE_STEPPING()
    ,
    ScreenPhaseStepping
#endif
    >;
static mem_space all_dialogs;

// safer than make_static_unique_ptr, checks storage size
template <class T, class... Args>
static static_unique_ptr<IDialogMarlin> make_dialog_ptr(Args &&...args) {
    static_assert(sizeof(T) <= sizeof(all_dialogs), "Error dialog does not fit");
    return make_static_unique_ptr<T>(&all_dialogs, std::forward<Args>(args)...);
}

static void OpenPrintScreen(ClientFSM dialog) {
    switch (dialog) {
    case ClientFSM::Serial_printing:
        Screens::Access()->ClosePrinting();
        Screens::Access()->Open(ScreenFactory::Screen<SerialPrint>);
        return;
    case ClientFSM::Printing:
        Screens::Access()->CloseAll();
        Screens::Access()->Open(ScreenFactory::Screen<screen_printing_data_t>);
        return;
    default:
        return;
    }
}

//*****************************************************************************
// method definitions
void DialogHandler::open(ClientFSM fsm_type, fsm::BaseData data) {
    if (ptr) {
        if (dialog_cache.has_value()) {
            // TODO: Make all dialogs screens and use Screens state stack
            bsod("Can't open more then 2 dialogs at a time.");
        }

        dialog_cache = last_fsm_change;
        ptr = nullptr;
    }

    last_fsm_change = std::make_pair(fsm_type, data);

    // todo get_scr_printing_serial() is no dialog but screen ... change to dialog?
    //  only ptr = dialog_creators[dialog](data); should remain
    switch (fsm_type) {
    case ClientFSM::Serial_printing:
    case ClientFSM::Printing:
        if (IScreenPrinting::GetInstance() == nullptr) {
            OpenPrintScreen(fsm_type);
        } else {
            // opened, notify it
            IScreenPrinting::NotifyMarlinStart();
        }
        break;
    case ClientFSM::PrintPreview:
        if (!ScreenPrintPreview::GetInstance()) {
            Screens::Access()->Open(ScreenFactory::Screen<ScreenPrintPreview>);
        }
        break;
    case ClientFSM::CrashRecovery:
        if (!CrashRecovery::GetInstance()) {
            Screens::Access()->Open(ScreenFactory::Screen<CrashRecovery>);
        }
        break;
    case ClientFSM::Selftest:
#if HAS_SELFTEST()
        if (!ScreenSelftest::GetInstance()) {
            // data contain screen caption type
            // ScreenSelftest::SetHeaderMode(...);
            Screens::Access()->Open(ScreenFactory::Screen<ScreenSelftest>);
        }
#endif // HAS_SELFTEST
        break;
    case ClientFSM::ESP:
        if (!ScreenESP::GetInstance()) {
            Screens::Access()->Open(ScreenFactory::Screen<ScreenESP>);
        }
        break;
    case ClientFSM::ColdPull:
#if HAS_COLDPULL()
        if (!ScreenColdPull::GetInstance()) {
            Screens::Access()->Open(ScreenFactory::Screen<ScreenColdPull>);
        }
#endif
        break;
#if HAS_PHASE_STEPPING()
    case ClientFSM::PhaseStepping:
        if (!ScreenPhaseStepping::GetInstance()) {
            Screens::Access()->Open(ScreenFactory::Screen<ScreenPhaseStepping>);
        }
        break;
#endif
    case ClientFSM::QuickPause:
        ptr = make_dialog_ptr<DialogQuickPause>(data);
        break;
    case ClientFSM::Warning:
        ptr = make_dialog_ptr<DialogWarning>(data);
        break;
    case ClientFSM::Load_unload:
        ptr = make_dialog_ptr<DialogLoadUnload>(data);
        break;
    case ClientFSM::Preheat:
        ptr = make_dialog_ptr<DialogMenuPreheat>(data);
        break;
    case ClientFSM::_none:
        break;
    }
}

void DialogHandler::close(ClientFSM fsm_type) {
    // following are screens (not dialogs)
    switch (fsm_type) {
    case ClientFSM::Serial_printing:
    case ClientFSM::Printing:
        Screens::Access()->CloseAll();
        break;
    case ClientFSM::PrintPreview:
    case ClientFSM::CrashRecovery:
    case ClientFSM::Selftest:
    case ClientFSM::ESP:
    case ClientFSM::ColdPull:
#if HAS_PHASE_STEPPING()
    case ClientFSM::PhaseStepping:
#endif
        Screens::Access()->Close();
        break;
    default:
        break;
    }

    // Attempt to restore underlying screen state
    if (ptr != nullptr) {
        if (dialog_cache.has_value()) {
            ptr = nullptr;
            const auto cache = *dialog_cache;
            dialog_cache = std::nullopt;
            open(cache.first, cache.second);
        } else {
            ptr = nullptr; // destroy current dialog
        }
    }
}

void DialogHandler::change(ClientFSM fsm_type, fsm::BaseData data) {
    last_fsm_change = std::make_pair(fsm_type, data);

    switch (fsm_type) {
    case ClientFSM::PrintPreview:
        if (ScreenPrintPreview::GetInstance()) {
            ScreenPrintPreview::GetInstance()->Change(data);
        }
        break;
    case ClientFSM::CrashRecovery:
        if (CrashRecovery::GetInstance()) {
            CrashRecovery::GetInstance()->Change(data);
        }
        break;
    case ClientFSM::Selftest:
#if HAS_SELFTEST()
        if (ScreenSelftest::GetInstance()) {
            ScreenSelftest::GetInstance()->Change(data);
        }
#endif // HAS_SELFTEST
        break;
    case ClientFSM::ESP:
        if (ScreenESP::GetInstance()) {
            ScreenESP::GetInstance()->Change(data);
        }
        break;
    case ClientFSM::ColdPull:
#if HAS_COLDPULL()
        if (ScreenColdPull::GetInstance()) {
            ScreenColdPull::GetInstance()->Change(data);
        }
#endif
        break;
#if HAS_PHASE_STEPPING()
    case ClientFSM::PhaseStepping:
        if (ScreenPhaseStepping::GetInstance()) {
            ScreenPhaseStepping::GetInstance()->Change(data);
        }
        break;
#endif
    default:
        if (ptr) {
            ptr->Change(data);
        }
    }
}

bool DialogHandler::IsOpen() const {
    return ptr != nullptr;
}

DialogHandler &DialogHandler::Access() {
    static DialogHandler instance;
    return instance;
}

void DialogHandler::Loop() {
    const auto &new_fsm_states = marlin_vars()->get_fsm_states();
    const auto &old_fsm_states = fsm_states;
    if (old_fsm_states == new_fsm_states) {
        return;
    }

    const auto &new_top = new_fsm_states.get_top();
    const auto &old_top = old_fsm_states.get_top();

    // TODO Investigate whether Screens::Access()->Loop() is really needed.
    // TODO Update open() so that we won't need to call change() afterwards.
    if (new_top && old_top) {
        if (new_top->fsm_type == old_top->fsm_type) {
            if (new_top->data != old_top->data) {
                change(new_top->fsm_type, new_top->data);
            }
        } else {
            if (new_top->fsm_type == ClientFSM::Load_unload && old_top->fsm_type == ClientFSM::PrintPreview) {
                // TODO Remove this shitcode/prasohack as soon as possible.
                //      As a special exception we do not close PrintPreview screen when the LoadUnload dialog
                //      is requested. It would destroy the ToolsMappingBody while one of its methods is still
                //      executing, leading to calling refresh_physical_tool_filament_labels() which in turn
                //      jumped to undefined memory.
            } else {
                close(old_top->fsm_type);
                Screens::Access()->Loop();
            }
            open(new_top->fsm_type, new_top->data);
            Screens::Access()->Loop();
            change(new_top->fsm_type, new_top->data);
        }
    } else if (new_top && !old_top) {
        open(new_top->fsm_type, new_top->data);
        Screens::Access()->Loop();
        change(new_top->fsm_type, new_top->data);
    } else if (!new_top && old_top) {
        close(old_top->fsm_type);
        Screens::Access()->Loop();
    } else {
        // Having neither new_top nor old_top is perfectly valid,
        // since the FSM state may only differ in generation number.
        // In such case, we just don't do anything.
    }

    fsm_states = new_fsm_states;
}

bool DialogHandler::IsOpen(ClientFSM fsm) const {
    return fsm_states.is_active(fsm);
}

bool DialogHandler::IsAnyOpen() const {
    return fsm_states.get_top().has_value();
}
